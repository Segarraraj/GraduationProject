#include "renderer/renderer.h"

#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>

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
    
    _fence_values[i] = 0;
  }

  _fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  D3D12_FEATURE_DATA_ROOT_SIGNATURE root_signature_feature_data = {};
  root_signature_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

  if (FAILED(_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,
                                          &root_signature_feature_data,
                                          sizeof(root_signature_feature_data)))) {
    root_signature_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
  }

  D3D12_DESCRIPTOR_RANGE1 ranges[1];
  ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
  ranges[0].NumDescriptors = 1;
  ranges[0].BaseShaderRegister = 0;
  ranges[0].RegisterSpace = 0;
  ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
  ranges[0].OffsetInDescriptorsFromTableStart = 0;

  D3D12_ROOT_PARAMETER1 root_signature_parameters[1];
  root_signature_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  root_signature_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  root_signature_parameters[0].DescriptorTable.NumDescriptorRanges =
      sizeof(ranges) / sizeof(D3D12_DESCRIPTOR_RANGE1);
  root_signature_parameters[0].DescriptorTable.pDescriptorRanges = ranges;

  D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
  root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  root_signature_desc.Desc_1_1.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  root_signature_desc.Desc_1_1.NumParameters =
      sizeof(root_signature_parameters) / sizeof(D3D12_ROOT_PARAMETER1);;
  root_signature_desc.Desc_1_1.pParameters = root_signature_parameters;      
  root_signature_desc.Desc_1_1.NumStaticSamplers = 0;
  root_signature_desc.Desc_1_1.pStaticSamplers = nullptr;

  ID3DBlob* signature = nullptr;
  ID3DBlob* error = nullptr;

  result = D3D12SerializeVersionedRootSignature(&root_signature_desc, &signature, &error);
  result = _device->CreateRootSignature(0, signature->GetBufferPointer(),
                               signature->GetBufferSize(),
                               IID_PPV_ARGS(&_root_signature));

  if (signature != nullptr) {
    signature->Release();
  }

  if (error != nullptr) {
    error->Release();
  }

  float vertex_buffer_data[18] = { 
       1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
       0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f
  };

  D3D12_HEAP_PROPERTIES vertex_buffer_heap_properties = {};
  vertex_buffer_heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
  vertex_buffer_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  vertex_buffer_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  vertex_buffer_heap_properties.CreationNodeMask = 1;
  vertex_buffer_heap_properties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC vertex_buffer_resource_desc;
  vertex_buffer_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  vertex_buffer_resource_desc.Alignment = 0;
  vertex_buffer_resource_desc.Width = sizeof(vertex_buffer_data);
  vertex_buffer_resource_desc.Height = 1;
  vertex_buffer_resource_desc.DepthOrArraySize = 1;
  vertex_buffer_resource_desc.MipLevels = 1;
  vertex_buffer_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
  vertex_buffer_resource_desc.SampleDesc.Count = 1;
  vertex_buffer_resource_desc.SampleDesc.Quality = 0;
  vertex_buffer_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  vertex_buffer_resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  result = _device->CreateCommittedResource(
      &vertex_buffer_heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_buffer_resource_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_vertex_buffer));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create vertex buffer");
    return 1;
  }

  UINT8* vertex_data_buffer_begin = nullptr;
  D3D12_RANGE read_range = {};

  result = _vertex_buffer->Map(0, &read_range, reinterpret_cast<void**>(&vertex_data_buffer_begin));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't map vertex buffer memory");
    return 1;
  }

  memcpy(vertex_data_buffer_begin, vertex_buffer_data, sizeof(vertex_buffer_data));
  _vertex_buffer->Unmap(0, nullptr);

  _vertex_buffer_view = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();

  _vertex_buffer_view->BufferLocation = _vertex_buffer->GetGPUVirtualAddress();
  _vertex_buffer_view->StrideInBytes = sizeof(float) * 6;
  _vertex_buffer_view->SizeInBytes = sizeof(vertex_buffer_data);

  uint32_t index_buffer_data[3] = {0, 1, 2};

  vertex_buffer_resource_desc.Width = sizeof(index_buffer_data);

  result = _device->CreateCommittedResource(
      &vertex_buffer_heap_properties, D3D12_HEAP_FLAG_NONE,
      &vertex_buffer_resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&_index_buffer));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create index buffer");
    return 1;
  }

  result = _index_buffer->Map(0, &read_range, reinterpret_cast<void**>(&vertex_data_buffer_begin));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't map index buffer memory");
    return 1;
  }

  memcpy(vertex_data_buffer_begin, index_buffer_data, sizeof(index_buffer_data));
  _index_buffer->Unmap(0, nullptr);

  _index_buffer_view = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();

  _index_buffer_view->BufferLocation = _index_buffer->GetGPUVirtualAddress();
  _index_buffer_view->Format = DXGI_FORMAT_R32_UINT;
  _index_buffer_view->SizeInBytes = sizeof(vertex_buffer_data);

  D3D12_HEAP_PROPERTIES uniform_heap_props = {};
  uniform_heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
  uniform_heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  uniform_heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  uniform_heap_props.CreationNodeMask = 1;
  uniform_heap_props.VisibleNodeMask = 1;

  D3D12_DESCRIPTOR_HEAP_DESC uniform_heap_desc = {};
  uniform_heap_desc.NumDescriptors = 1;
  uniform_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  uniform_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

  result = _device->CreateDescriptorHeap(&uniform_heap_desc, IID_PPV_ARGS(&_uniform_buffer_heap));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create uniform heap descriptor");
    return 1;
  }

  D3D12_RESOURCE_DESC uniform_buffer_desc = {};
  uniform_buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  uniform_buffer_desc.Alignment = 0;
  uniform_buffer_desc.Width = (sizeof(UniformStruct) + 255) & ~255;
  uniform_buffer_desc.Height = 1;
  uniform_buffer_desc.DepthOrArraySize = 1;
  uniform_buffer_desc.MipLevels = 1;
  uniform_buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
  uniform_buffer_desc.SampleDesc.Count = 1;
  uniform_buffer_desc.SampleDesc.Quality = 0;
  uniform_buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  uniform_buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  result = _device->CreateCommittedResource(
      &uniform_heap_props, D3D12_HEAP_FLAG_NONE, &uniform_buffer_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&_uniform_buffer));

  D3D12_CONSTANT_BUFFER_VIEW_DESC uniform_buffer_view_desc = {};
  uniform_buffer_view_desc.BufferLocation = _uniform_buffer->GetGPUVirtualAddress();
  uniform_buffer_view_desc.SizeInBytes = (sizeof(UniformStruct) + 255) & ~255;

  D3D12_CPU_DESCRIPTOR_HANDLE uniform_buffer_handle(
      _uniform_buffer_heap->GetCPUDescriptorHandleForHeapStart());

  uniform_buffer_handle.ptr =
      uniform_buffer_handle.ptr +
      _device->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

  _device->CreateConstantBufferView(&uniform_buffer_view_desc,
                                    uniform_buffer_handle);

  read_range = {};
  UniformStruct uniform = {};

  DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();
  XMStoreFloat4x4(&uniform.model, matrix);

  matrix =
      DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0.0f, 0.0f, -2.0f, 1.0f),
                                DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
                                DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));
  XMStoreFloat4x4(&uniform.view, matrix);
  
  matrix = DirectX::XMMatrixPerspectiveLH(
      window_screen_bounds.right - window_screen_bounds.left,
      window_screen_bounds.bottom - window_screen_bounds.top, .1f, 10.0f);
  XMStoreFloat4x4(&uniform.projection, matrix);

  _uniform_buffer->Map(0, &read_range, reinterpret_cast<void**>(&vertex_data_buffer_begin));
  memcpy(vertex_data_buffer_begin, &uniform, sizeof(UniformStruct));
  _uniform_buffer->Unmap(0, nullptr);

  std::vector<char> bytecode = Utils::ReadFile("../../shaders/triangle_vert.dxil\0");
  if (bytecode.size() == 0) {
    return 1;
  }

  D3D12_SHADER_BYTECODE vertex_shader_byte_code;
  vertex_shader_byte_code.pShaderBytecode = bytecode.data();
  vertex_shader_byte_code.BytecodeLength = bytecode.size();

  bytecode = Utils::ReadFile("../../shaders/triangle_frag.dxil\0");
  if (bytecode.size() == 0) {
    return 1;
  }

  D3D12_SHADER_BYTECODE fragment_shader_byte_code;
  fragment_shader_byte_code.pShaderBytecode = bytecode.data();
  fragment_shader_byte_code.BytecodeLength = bytecode.size();

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc = {};

  D3D12_INPUT_ELEMENT_DESC pipeline_input_descs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  pipeline_desc.InputLayout = {pipeline_input_descs,
      sizeof(pipeline_input_descs) / sizeof(D3D12_INPUT_ELEMENT_DESC)};
  pipeline_desc.pRootSignature = _root_signature;

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

  pipeline_desc.RasterizerState = rasterizer_desc;
  pipeline_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  D3D12_BLEND_DESC blend_desc;
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

  pipeline_desc.BlendState = blend_desc;

  pipeline_desc.DepthStencilState.DepthEnable = FALSE;
  pipeline_desc.DepthStencilState.StencilEnable = FALSE;
  pipeline_desc.SampleMask = UINT_MAX;

  pipeline_desc.NumRenderTargets = 1;
  pipeline_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  pipeline_desc.SampleDesc.Count = 1;

  //result = _device->CreateGraphicsPipelineState(&pipeline_desc, IID_PPV_ARGS(&_pipeline_state));
  //if (FAILED(result)) {
  //  LOG_ERROR("RR", "Couldn't create pipeline state");
  //  return 1;
  //}

  printf("\n");
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
  WaitForFrame();

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

  // Command list resource barrier
  D3D12_RESOURCE_BARRIER rt_render_barrier = {};
  rt_render_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  rt_render_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  rt_render_barrier.Transition.pResource = _render_targets[_current_frame];
  rt_render_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  rt_render_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  rt_render_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  _command_list->ResourceBarrier(1, &rt_render_barrier);

  // Command list render target
  unsigned int descriptor_size =
      _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptor_handle(
      _rt_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
  rt_descriptor_handle.ptr += _current_frame * descriptor_size;

  _command_list->OMSetRenderTargets(1, &rt_descriptor_handle, FALSE, nullptr);

  // Clear render target
  const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
  _command_list->ClearRenderTargetView(rt_descriptor_handle, clearColor, 0,
                                       nullptr);

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

  _command_queue->ExecuteCommandLists(1, command_lists);
  _command_queue->Signal(_fences[_current_frame], _fence_values[_current_frame]);
  _swap_chain->Present(0, 0);
}

void RR::Renderer::Cleanup() { 
  WaitForFrame();

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
    if (_render_targets[i] != nullptr) {
      _render_targets[i]->Release();
      _render_targets[i] = nullptr;
    }

    if (_command_allocators[i] != nullptr) {
      _command_allocators[i]->Release();
      _command_allocators[i] = nullptr;
    }

    if (_fences[i] != nullptr) {
      _fences[i]->Release();
      _fences[i] = nullptr;
    }
  };
}

void RR::Renderer::WaitForFrame() {
  HRESULT result;

  _current_frame = _swap_chain->GetCurrentBackBufferIndex();

  if (_fences[_current_frame]->GetCompletedValue() < _fence_values[_current_frame]) {

    result = _fences[_current_frame]->SetEventOnCompletion(
        _fence_values[_current_frame], _fence_event);

    WaitForSingleObject(_fence_event, INFINITE);
  }

  _fence_values[_current_frame]++;
}