#include "renderer/renderer.h"

#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>

#include "utils.hpp"
#include "renderer/window.h"
#include "renderer/logger.h"

static long long CALLBACK WindowProc(void* window, unsigned int message,
                              unsigned long long wParam, long long lParam) {
  LRESULT result = 0;

  RR::Renderer* renderer = (RR::Renderer*) GetWindowLongPtr((HWND)window, GWLP_USERDATA);

  switch (message) {
    case WM_CLOSE:
      renderer->Stop();
      break;
    default: {
      result = DefWindowProc((HWND)window, message, wParam, lParam);
      break;
    }
  }
  return result;
}

RR::Renderer::Renderer() {}

RR::Renderer::~Renderer() {}

int RR::Renderer::Init(void (*update)()) {
  LOG_DEBUG("RR", "Initializing renderer");
  
  // Set client update logic callback
  update_ = update;

  // Initialize window
  _window = std::make_unique<RR::Window>();
  _window->Init(GetModuleHandle(NULL), "winclass", "DX12 Graduation Project",
                WindowProc, this);

  _window->Show(); 

  HRESULT result;

  // Create device
  unsigned int factory_flags = 0;
  LOG_DEBUG("RR", "Creating virtual device");
#ifdef DEBUG
  result = D3D12GetDebugInterface(IID_PPV_ARGS(&_debug_controller));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't get debug interface");
    Cleanup();
    return 1;
  }

  _debug_controller->EnableDebugLayer();
  _debug_controller->SetEnableGPUBasedValidation(true);

  factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

  IDXGIFactory4* factory = nullptr;
  IDXGIAdapter1* adapter = nullptr;
  result = CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create factory");
    Cleanup();
    return 1;
  }

  for (unsigned int adapter_index = 0; 
       factory->EnumAdapters1(adapter_index, &adapter) != DXGI_ERROR_NOT_FOUND;
       adapter_index++) {

    DXGI_ADAPTER_DESC1 desc = {};
    adapter->GetDesc1(&desc);

    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      continue;
    }

    result = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device));

    if (SUCCEEDED(result)) {
      printf("\n\0");
      LOG_DEBUG("RR", "Running on: %S", desc.Description);
      LOG_DEBUG("RR", "Dedicated video memory: %.2f GB", desc.DedicatedVideoMemory / 1000000000.0);
      printf("\n\0");
      break;
    }

    adapter->Release();
    _device->Release();
  }

#ifdef DEBUG
  _device->QueryInterface(&_debug_device);
#endif  // DEBUG

  adapter->Release();

  // Create command queue
  LOG_DEBUG("RR", "Creating command queue");

  D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
  command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  command_queue_desc.NodeMask = 0;

  result = _device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&_command_queue));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create command queue");
    Cleanup();
    return 1;
  }

  // Create swapchain
  LOG_DEBUG("RR", "Creating swapchain");

  RECT window_screen_bounds;
  GetClientRect((HWND)_window->window(), &window_screen_bounds);

  DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
  swapchain_desc.Width = window_screen_bounds.right - window_screen_bounds.left;
  swapchain_desc.Height = window_screen_bounds.bottom - window_screen_bounds.top;
  swapchain_desc.Stereo = FALSE;
  swapchain_desc.SampleDesc.Count = 1;
  swapchain_desc.SampleDesc.Quality = 0;
  swapchain_desc.BufferCount = kSwapchainBufferCount;
  swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
  swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
  swapchain_desc.Flags = 0 | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapchain_fs_desc = {};
  swapchain_fs_desc.Windowed = TRUE;

  IDXGISwapChain1* chain = nullptr;
  result = factory->CreateSwapChainForHwnd(
      _command_queue, (HWND)_window->window(),
      &swapchain_desc, &swapchain_fs_desc,
      nullptr, &chain);

  _swap_chain = static_cast<IDXGISwapChain3*>(chain);
  _current_frame = _swap_chain->GetCurrentBackBufferIndex(); 

  factory->Release();

  // Creating render target descriptor heap

  LOG_DEBUG("RR", "Creating render target descriptor heap");

  D3D12_DESCRIPTOR_HEAP_DESC rt_descriptor_heap_desc = {};
  rt_descriptor_heap_desc.NumDescriptors = kSwapchainBufferCount;
  rt_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rt_descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  
  result = _device->CreateDescriptorHeap(&rt_descriptor_heap_desc, IID_PPV_ARGS(&_rt_descriptor_heap));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldent create swapchain render targets descriptor heap");
    return 1;
  }

  unsigned int descriptor_size =
      _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptor_handle(
      _rt_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

  for (unsigned int i = 0; i < kSwapchainBufferCount; i++) {
    _swap_chain->GetBuffer(i, IID_PPV_ARGS(&_render_targets[i]));
    _device->CreateRenderTargetView(_render_targets[i], nullptr, rt_descriptor_handle);
    rt_descriptor_handle.ptr += (1 * descriptor_size);
  }

  // Create command allocators
  LOG_DEBUG("RR", "Creating command allocators");

  for (unsigned int i = 0; i < kSwapchainBufferCount; i++) {
    result = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                 IID_PPV_ARGS(&_command_allocators[i]));
  }

  // Create command list
  LOG_DEBUG("RR", "Creating command list");

  result = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      _command_allocators[0], NULL,
                                      IID_PPV_ARGS(&_command_list));

  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create command list");
    return 1;
  }

  _command_list->Close();

  // Create fences
  LOG_DEBUG("RR", "Creating fences");

  for (unsigned int i = 0; i < kSwapchainBufferCount; i++) {
    result = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fences[i]));
    if (FAILED(result)) {
      LOG_DEBUG("RR", "Couldn't create fence");
      return 1;
    }
    _fences[i]->Signal(1);
  }

  _fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  // Create root signature
  LOG_DEBUG("RR", "Creating root signature");

  D3D12_FEATURE_DATA_ROOT_SIGNATURE root_signature_feature_data = {};
  root_signature_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
  result = _device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,
                                        &root_signature_feature_data,
                                        sizeof(root_signature_feature_data));

  if (FAILED(result)) {
    LOG_WARNING("RR", "Current device don't support root signature v1.1, using v1.0");
    root_signature_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
  }

  D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
  root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  root_signature_desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  root_signature_desc.Desc_1_1.NumParameters = 0;
  root_signature_desc.Desc_1_1.pParameters = nullptr;      
  root_signature_desc.Desc_1_1.NumStaticSamplers = 0;
  root_signature_desc.Desc_1_1.pStaticSamplers = nullptr;

  ID3DBlob* signature = nullptr;
  ID3DBlob* error = nullptr;

  result = D3D12SerializeVersionedRootSignature(&root_signature_desc, &signature, &error);
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't serialeze root signature");
    return 1;
  }
  
  result = _device->CreateRootSignature(
      0, signature->GetBufferPointer(), signature->GetBufferSize(),
      IID_PPV_ARGS(&_root_signature));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create root signature");
    return 1;
  }

  // Read shaders
  LOG_DEBUG("RR", "Reading shaders");

  ID3DBlob* vertex_shader;
  UINT compile_flags = 0;

#ifdef DEBUG
  compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif  // DEBUG

  result = D3DCompileFromFile(L"../../shaders/triangle.vert.hlsl", nullptr,
                              nullptr, "main", "vs_5_1", compile_flags, 0,
                              &vertex_shader, &error);

  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't compile vertex shader");
    return 1;
  }

  D3D12_SHADER_BYTECODE vertex_shader_bytecode = {};
  vertex_shader_bytecode.BytecodeLength = vertex_shader->GetBufferSize();
  vertex_shader_bytecode.pShaderBytecode = vertex_shader->GetBufferPointer();

  ID3DBlob* fragment_shader;
  result = D3DCompileFromFile(L"../../shaders/triangle.frag.hlsl", nullptr,
                              nullptr, "main", "ps_5_1", compile_flags, 0,
                              &fragment_shader, &error);
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't compile fragment shader");
    return 1;
  }

  D3D12_SHADER_BYTECODE pixel_shader_bytecode = {};
  pixel_shader_bytecode.BytecodeLength = fragment_shader->GetBufferSize();
  pixel_shader_bytecode.pShaderBytecode = fragment_shader->GetBufferPointer();

  D3D12_INPUT_ELEMENT_DESC input_layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
  };

  D3D12_INPUT_LAYOUT_DESC input_layout_desc = {};
  input_layout_desc.pInputElementDescs = input_layout;
  input_layout_desc.NumElements = sizeof(input_layout) / sizeof(D3D12_INPUT_ELEMENT_DESC);

  D3D12_RASTERIZER_DESC rasterizer_desc = {};
  rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
  rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
  rasterizer_desc.FrontCounterClockwise = FALSE;
  rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  rasterizer_desc.DepthClipEnable = TRUE;
  rasterizer_desc.MultisampleEnable = FALSE;
  rasterizer_desc.AntialiasedLineEnable = FALSE;
  rasterizer_desc.ForcedSampleCount = 0;
  rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  D3D12_BLEND_DESC blend_desc = {};
  blend_desc.AlphaToCoverageEnable = FALSE;
  blend_desc.IndependentBlendEnable = FALSE;

  const D3D12_RENDER_TARGET_BLEND_DESC render_target_blend_desc = {
      FALSE,
      FALSE,
      D3D12_BLEND_ONE,
      D3D12_BLEND_ZERO,
      D3D12_BLEND_OP_ADD,
      D3D12_BLEND_ONE,
      D3D12_BLEND_ZERO,
      D3D12_BLEND_OP_ADD,
      D3D12_LOGIC_OP_NOOP,
      D3D12_COLOR_WRITE_ENABLE_ALL,
  };

  for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
    blend_desc.RenderTarget[i] = render_target_blend_desc;
  }

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc = {};
  pipeline_desc.InputLayout = input_layout_desc;
  pipeline_desc.pRootSignature = _root_signature;
  pipeline_desc.VS = vertex_shader_bytecode;
  pipeline_desc.PS = pixel_shader_bytecode;
  pipeline_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pipeline_desc.RasterizerState = rasterizer_desc;
  pipeline_desc.BlendState = blend_desc;
  pipeline_desc.NumRenderTargets = 1;
  pipeline_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  pipeline_desc.SampleDesc.Count = 1;
  pipeline_desc.DepthStencilState.DepthEnable = FALSE;
  pipeline_desc.DepthStencilState.StencilEnable = FALSE;
  pipeline_desc.SampleMask = UINT_MAX;

  result = _device->CreateGraphicsPipelineState(&pipeline_desc, 
      IID_PPV_ARGS(&_pipeline_state));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create pipeline state");
    return 1;
  }

  // Create vertex buffer
  DirectX::XMFLOAT3 vertex_list[] = {
      { 0.0f,  0.5f, 0.5f}, {1.0f, 0.0f, 0.0f},
      { 0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f},
      {-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f},
  };

   UINT indexes[] = {0, 1, 2};

  D3D12_HEAP_PROPERTIES default_heap_properties = {};
  default_heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
  default_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  default_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_RESOURCE_DESC vertex_resource_desc = {};
  vertex_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  vertex_resource_desc.Alignment = 0;
  vertex_resource_desc.Width = sizeof(vertex_list);
  vertex_resource_desc.Height = 1;
  vertex_resource_desc.DepthOrArraySize = 1;
  vertex_resource_desc.MipLevels = 1;
  vertex_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
  vertex_resource_desc.SampleDesc.Count = 1;
  vertex_resource_desc.SampleDesc.Quality = 0;
  vertex_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  vertex_resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
   
  result = _device->CreateCommittedResource(
      &default_heap_properties, D3D12_HEAP_FLAG_NONE, 
      &vertex_resource_desc, D3D12_RESOURCE_STATE_COMMON, 
      nullptr, IID_PPV_ARGS(&_vertex_default_buffer));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create vertex buffer default resource heap");
    return 1;
  }

  ID3D12Resource* vertex_buffer_upload_heap = nullptr;
  D3D12_HEAP_PROPERTIES upload_heap_properties = {};
  upload_heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
  upload_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  upload_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  result = _device->CreateCommittedResource(
      &upload_heap_properties, D3D12_HEAP_FLAG_NONE,
      &vertex_resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&vertex_buffer_upload_heap));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create vertex buffer upload resource heap");
    return 1;
  }

  vertex_resource_desc.Width = sizeof(indexes);
  result = _device->CreateCommittedResource(
      &default_heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_resource_desc,
      D3D12_RESOURCE_STATE_COMMON, nullptr,
      IID_PPV_ARGS(&_index_default_buffer));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create index buffer default resource heap");
    return 1;
  }

  ID3D12Resource* index_buffer_upload_heap = nullptr;
  result = _device->CreateCommittedResource(
      &upload_heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_resource_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&index_buffer_upload_heap));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create index buffer upload resource heap");
    return 1;
  }

  UINT8* upload_resource_heap_begin;
  D3D12_RANGE read_range;
  read_range.Begin = 0;
  read_range.End = 0;

  // Copy data to upload resource heap
  vertex_buffer_upload_heap->Map(0, &read_range,
      reinterpret_cast<void**>(&upload_resource_heap_begin));
  memcpy(upload_resource_heap_begin, vertex_list, sizeof(vertex_list));
  vertex_buffer_upload_heap->Unmap(0, nullptr);

  index_buffer_upload_heap->Map(0, &read_range, 
      reinterpret_cast<void**>(&upload_resource_heap_begin));
  memcpy(upload_resource_heap_begin, indexes, sizeof(indexes));
  index_buffer_upload_heap->Unmap(0, nullptr);

  D3D12_RESOURCE_BARRIER vb_upload_resource_heap_barrier = {};
  vb_upload_resource_heap_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  vb_upload_resource_heap_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  vb_upload_resource_heap_barrier.Transition.pResource = _vertex_default_buffer;
  vb_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  vb_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  vb_upload_resource_heap_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  
  D3D12_RESOURCE_BARRIER index_upload_resource_heap_barrier = {};
  index_upload_resource_heap_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  index_upload_resource_heap_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  index_upload_resource_heap_barrier.Transition.pResource = _index_default_buffer;
  index_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  index_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  index_upload_resource_heap_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  _command_list->Reset(_command_allocators[_current_frame], NULL);
  _command_list->ResourceBarrier(1, &vb_upload_resource_heap_barrier);
  _command_list->ResourceBarrier(1, &index_upload_resource_heap_barrier);
  _command_list->CopyResource(_vertex_default_buffer, vertex_buffer_upload_heap);
  _command_list->CopyResource(_index_default_buffer, index_buffer_upload_heap);
  vb_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  vb_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  index_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  index_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
  _command_list->ResourceBarrier(1, &vb_upload_resource_heap_barrier);
  _command_list->ResourceBarrier(1, &index_upload_resource_heap_barrier);
  result = _command_list->Close();

  ID3D12CommandList* command_lists[] = {_command_list};
  _command_queue->ExecuteCommandLists(sizeof(command_lists) / sizeof(ID3D12CommandList), command_lists);

  _command_queue->Signal(_fences[_current_frame], 1);

  _vertex_buffer_view = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
  _vertex_buffer_view->BufferLocation = _vertex_default_buffer->GetGPUVirtualAddress();
  _vertex_buffer_view->StrideInBytes = sizeof(DirectX::XMFLOAT3) * 2;
  _vertex_buffer_view->SizeInBytes = sizeof(vertex_list);

  _index_buffer_view = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
  _index_buffer_view->BufferLocation = _index_default_buffer->GetGPUVirtualAddress();
  _index_buffer_view->Format = DXGI_FORMAT_R32_UINT;
  _index_buffer_view->SizeInBytes = sizeof(indexes);

  LOG_DEBUG("RR", "Renderer initialized");
  return 0;
}

void RR::Renderer::Start() {
  while (_running) {
    MSG message;
    while (PeekMessage(&message, (HWND)_window->window(), 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    _current_frame = _swap_chain->GetCurrentBackBufferIndex();

    WaitForPreviousFrame();
    
    update_();

    UpdatePipeline();
    Render();
  }

  Cleanup();
}

void RR::Renderer::Stop() { 
  _running = false;
}

void RR::Renderer::Resize() {

}

void RR::Renderer::UpdatePipeline() { 
  HRESULT result = _command_allocators[_current_frame]->Reset();
  if (FAILED(result)) {
    _running = false;
    return;
  }

  result = _command_list->Reset(_command_allocators[_current_frame], NULL);
  if (FAILED(result)) {
    _running = false;
    return;
  }

  D3D12_RESOURCE_BARRIER rt_render_barrier = {};
  rt_render_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  rt_render_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  rt_render_barrier.Transition.pResource = _render_targets[_current_frame];
  rt_render_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  rt_render_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  rt_render_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  _command_list->ResourceBarrier(1, &rt_render_barrier);

  unsigned int descriptor_size =
      _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptor_handle(
      _rt_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
  rt_descriptor_handle.ptr += _current_frame * descriptor_size;

  _command_list->OMSetRenderTargets(1, &rt_descriptor_handle, FALSE, nullptr);

  const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
  _command_list->ClearRenderTargetView(rt_descriptor_handle, clearColor, 0,
                                       nullptr);

  RECT window_screen_bounds;
  GetClientRect((HWND)_window->window(), &window_screen_bounds);

  D3D12_VIEWPORT viewport = {};
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = window_screen_bounds.right - window_screen_bounds.left;
  viewport.Height = window_screen_bounds.bottom - window_screen_bounds.top;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;

  D3D12_RECT scissor_rect = {};
  scissor_rect.left = 0;
  scissor_rect.top = 0;
  scissor_rect.right = window_screen_bounds.right - window_screen_bounds.left;
  scissor_rect.bottom = window_screen_bounds.bottom - window_screen_bounds.top;

  _command_list->SetPipelineState(_pipeline_state);
  _command_list->SetGraphicsRootSignature(_root_signature);
  _command_list->RSSetViewports(1, &viewport);
  _command_list->RSSetScissorRects(1, &scissor_rect);
  _command_list->IASetVertexBuffers(0, 1, _vertex_buffer_view.get());
  _command_list->IASetIndexBuffer(_index_buffer_view.get());
  _command_list->DrawIndexedInstanced(3, 1, 0, 0, 0);

  D3D12_RESOURCE_BARRIER rt_present_barrier = {};
  rt_present_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  rt_present_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  rt_present_barrier.Transition.pResource = _render_targets[_current_frame];
  rt_present_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  rt_present_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  rt_present_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  _command_list->ResourceBarrier(1, &rt_present_barrier);

  result = _command_list->Close();
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't close command list");
    _running = false;
  }
}

void RR::Renderer::Render() {
  HRESULT result = {};

  ID3D12CommandList* command_lists[] = {_command_list};

  _command_queue->ExecuteCommandLists(sizeof(command_lists) / sizeof(ID3D12CommandList), command_lists);
  _command_queue->Signal(_fences[_current_frame], 1);
  _swap_chain->Present(0, 0);
}

void RR::Renderer::Cleanup() { 
  for (; _current_frame < kSwapchainBufferCount; _current_frame++) {
    WaitForPreviousFrame();
  }

  if (_device != nullptr) {
    _device->Release();
    _device = nullptr;
  }

  if (_swap_chain != nullptr) {
    _swap_chain->Release();
    _swap_chain = nullptr;
  }

  if (_command_queue != nullptr) {
    _command_queue->Release();
    _command_queue = nullptr;
  }

  if (_rt_descriptor_heap != nullptr) {
    _rt_descriptor_heap->Release();
    _rt_descriptor_heap = nullptr;
  }

  if (_command_list != nullptr) {
    _command_list->Release();
    _command_list = nullptr;
  }

  for (int i = 0; i < kSwapchainBufferCount; ++i) {
    if (_command_allocators[i] != nullptr) {
      _command_allocators[i]->Release();
      _command_allocators[i] = nullptr;
    }

    if (_render_targets[i] != nullptr) {
      _render_targets[i]->Release();
      _render_targets[i] = nullptr;
    }

    if (_fences[i] != nullptr) {
      _fences[i]->Release();
      _fences[i] = nullptr;
    }
  };
}

void RR::Renderer::WaitForPreviousFrame() {
  HRESULT result;

  if (_fences[_current_frame]->GetCompletedValue() != 1) {
    _fences[_current_frame]->SetEventOnCompletion(1, _fence_event);
    WaitForSingleObject(_fence_event, INFINITE);
  }

  _fences[_current_frame]->Signal(0);
}