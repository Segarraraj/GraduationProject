#ifndef __PROJECT_RENDERER_RENDERER__
#define __PROJECT_RENDERER_RENDERER__ 1

#include <memory>

struct ID3D12CommandQueue;
struct IDXGIFactory4;
struct IDXGIAdapter1;
struct ID3D12Device;
struct ID3D12CommandAllocator;
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

 private:
  std::unique_ptr<RR::Window> _window;

  ID3D12CommandAllocator* _command_allocator;
  ID3D12CommandQueue* _command_queue;
  IDXGIFactory4* _factory;
  IDXGIAdapter1* _adapter;
  ID3D12Device* _device;
 #ifdef DEBUG
  ID3D12Debug1* _debug_controller;
  ID3D12DebugDevice* _debug_device;
 #endif

  void (*update_)() = nullptr;

  bool _running = true;
};
}

#endif  // !__PROJECT_RENDERER_RENDERER__
