#ifndef __PROJECT_RENDERER_RENDERER__
#define __PROJECT_RENDERER_RENDERER__ 1

#include <memory>

#ifdef DEBUG
struct ID3D12Debug1;
struct ID3D12DebugDevice;
#endif
struct IDXGIFactory4;
struct IDXGIAdapter1;
struct ID3D12Device;

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
