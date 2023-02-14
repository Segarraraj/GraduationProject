#ifndef __PROJECT_RENDERER_RENDERER__
#define __PROJECT_RENDERER_RENDERER__ 1

#include <memory>

struct ID3D12CommandAllocator;
struct ID3D12DescriptorHeap;
struct ID3D12CommandQueue;
struct IDXGISwapChain3;
struct ID3D12Resource;
struct IDXGIFactory4;
struct IDXGIAdapter1;
struct ID3D12Device;

#ifdef DEBUG
struct ID3D12Debug1;
struct ID3D12DebugDevice;
#endif

namespace RR {
class Window;
class Renderer {
 public:
  Renderer();

  Renderer(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;

  void operator=(const Renderer&) = delete;
  void operator=(Renderer&&) = delete;

  ~Renderer();

  int Init(void (*update)());
  void Start() const;
  void Stop();
  void Resize();

 private:
  static const unsigned int kSwapchainBufferCount = 2;

  std::unique_ptr<RR::Window> _window;

  ID3D12CommandAllocator* _command_allocator = nullptr;
  ID3D12CommandQueue* _command_queue = nullptr;
  IDXGISwapChain3* _swapchain = nullptr;
  ID3D12DescriptorHeap* _rt_descriptor_heap = nullptr;
  ID3D12Resource* _render_targets[kSwapchainBufferCount] = {0};
  IDXGIFactory4* _factory = nullptr;
  IDXGIAdapter1* _adapter = nullptr;
  ID3D12Device* _device = nullptr;

 #ifdef DEBUG
  ID3D12Debug1* _debug_controller = nullptr;
  ID3D12DebugDevice* _debug_device = nullptr;
 #endif

  void (*update_)() = nullptr;

  bool _running = true;
};
}

#endif  // !__PROJECT_RENDERER_RENDERER__
