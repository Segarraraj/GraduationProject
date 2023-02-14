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

void RR::Renderer::Resize(){};