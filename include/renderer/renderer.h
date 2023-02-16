#ifndef __PROJECT_RENDERER_RENDERER__
#define __PROJECT_RENDERER_RENDERER__ 1

#include <DirectXMath.h>

#include <memory>

struct ID3D12Device;
struct IDXGISwapChain3;
struct ID3D12CommandQueue;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12Fence;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12PipelineState;
struct ID3D12RootSignature;

#ifdef DEBUG
struct ID3D12Debug1;
struct ID3D12DebugDevice;
#endif

namespace RR {
class Window;

struct UniformStruct {
  DirectX::XMFLOAT4X4 view;
  DirectX::XMFLOAT4X4 model;
  DirectX::XMFLOAT4X4 projection;
};

class Renderer {
 public:
  Renderer();

  Renderer(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;

  void operator=(const Renderer&) = delete;
  void operator=(Renderer&&) = delete;

  ~Renderer();

  int Init(void (*update)());
  void Start();
  void Stop();
  void Resize();

 private:
  static const unsigned int kSwapchainBufferCount = 3;

  std::unique_ptr<RR::Window> _window;

  ID3D12Device* _device = nullptr;
  IDXGISwapChain3* _swap_chain = nullptr;
  ID3D12CommandQueue* _command_queue = nullptr;
  ID3D12DescriptorHeap* _rt_descriptor_heap = nullptr;
  ID3D12Resource* _render_targets[kSwapchainBufferCount] = {0};
  ID3D12CommandAllocator* _command_allocators[kSwapchainBufferCount] = {0};
  ID3D12GraphicsCommandList* _command_list = nullptr;
  ID3D12Fence* _fences[kSwapchainBufferCount] = {0};  
  void* _fence_event = nullptr;
  unsigned int _fence_values[kSwapchainBufferCount] = {0};

  ID3D12PipelineState* _pipeline_state = nullptr;
  ID3D12RootSignature* _root_signature = nullptr;

 #ifdef DEBUG
  ID3D12Debug1* _debug_controller = nullptr;
  ID3D12DebugDevice* _debug_device = nullptr;
 #endif

  void (*update_)() = nullptr;

  bool _running = true;
  int _current_frame = 0;

  void UpdatePipeline();
  void Render();
  void Cleanup();
  void WaitForFrame();
};
}

#endif  // !__PROJECT_RENDERER_RENDERER__
