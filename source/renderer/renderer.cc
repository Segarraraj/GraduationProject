#include "renderer/renderer.h"

#include <stdio.h>
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