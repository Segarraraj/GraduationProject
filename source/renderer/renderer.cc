#include "renderer/renderer.h"

#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>

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
  _window = std::make_unique<RR::Window>();
  update_ = update;

  _window->Init(GetModuleHandle(NULL), "winclass", "DX12 Graduation Project",
                WindowProc, this);

  _window->Show(); 

  unsigned int factory_flags = 0;

  HRESULT result;

#ifdef DEBUG
  result = D3D12GetDebugInterface(IID_PPV_ARGS(&_debug_controller));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't't get debug interface");
    return 1;
  }

  _debug_controller->EnableDebugLayer();
  _debug_controller->SetEnableGPUBasedValidation(true);

  factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

  result = CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&_factory));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create factory");
    return 1;
  }

  for (unsigned int adapter_index = 0; 
       _factory->EnumAdapters1(adapter_index, &_adapter) != DXGI_ERROR_NOT_FOUND;
       adapter_index++) {

    DXGI_ADAPTER_DESC1 desc;
    _adapter->GetDesc1(&desc);

    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      continue;
    }

    result = D3D12CreateDevice(_adapter, D3D_FEATURE_LEVEL_12_0, 
                                       IID_PPV_ARGS(&_device));
    if (SUCCEEDED(result)) {
      printf("\n\0");
      LOG_DEBUG("RR", "Running on: %S", desc.Description);
      LOG_DEBUG("RR", "Dedicated video memory: %.2f GB", desc.DedicatedVideoMemory / 1000000000.0);
      printf("\n\0");
      break;
    }

    _adapter->Release();
    _device->Release();
  }

#ifdef DEBUG
  _device->QueryInterface(&_debug_device);
#endif  // DEBUG

  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  result = _device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&_command_queue));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create command queue");
    return 1;
  }

  result = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           IID_PPV_ARGS(&_command_allocator));

  RECT window_screen_bounds;
  GetClientRect((HWND)_window->window(), &window_screen_bounds);

  DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
  swapchain_desc.BufferCount = kSwapchainBufferCount;
  swapchain_desc.Width = window_screen_bounds.right - window_screen_bounds.left;
  swapchain_desc.Height = window_screen_bounds.bottom - window_screen_bounds.top;
  swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
  swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
  swapchain_desc.Flags = 0 | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  swapchain_desc.SampleDesc.Count = 1;
  swapchain_desc.SampleDesc.Quality = 0;

  DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapchain_fs_desc = {};
  swapchain_fs_desc.Windowed = TRUE;

  IDXGISwapChain1* chain = nullptr;
  result = _factory->CreateSwapChainForHwnd(
      _command_queue, (HWND)_window->window(), &swapchain_desc, &swapchain_fs_desc,
      nullptr, &chain);

  result = chain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&chain);
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create swapchain");
    return 1;
  }
  _swapchain = (IDXGISwapChain3*) chain;

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

  D3D12_CPU_DESCRIPTOR_HANDLE 
    rt_handle(_rt_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

  for (unsigned int i = 0; i < kSwapchainBufferCount; i++) {
    _swapchain->GetBuffer(i, IID_PPV_ARGS(&_render_targets[i]));
    _device->CreateRenderTargetView(_render_targets[i], nullptr, rt_handle);
    rt_handle.ptr += (1 * descriptor_size);
  }

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

  LOG_DEBUG("RR", "Renderer initialized");
  return 0;
}

void RR::Renderer::Start() const {
  while (_running) {
    MSG message;
    while (PeekMessage(&message, (HWND)_window->window(), 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    update_();
  }
}

void RR::Renderer::Stop() { _running = false; }

void RR::Renderer::Resize() { }