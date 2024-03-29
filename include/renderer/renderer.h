#ifndef __RENDERER_H__
#define __RENDERER_H__ 1

#include <DirectXMath.h>

#include <map>
#include <list>
#include <memory>
#include <vector>

#include "common.hpp"

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
struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;

#ifdef DEBUG
struct ID3D12Debug1;
struct ID3D12DebugDevice;
#endif

namespace RR {
class Window;
class Entity;
class Camera;
class Input;
struct GeometryData;
class Editor;
namespace GFX {
class Texture;
class Pipeline;
class Geometry;
}

class Renderer {
 public:
  float delta_time = 0.0f;
  float elapsed_time = 0.0f;

  Renderer();

  Renderer(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;

  void operator=(const Renderer&) = delete;
  void operator=(Renderer&&) = delete;

  ~Renderer();

  int Init(void* user_data, void (*update)(void*));
  void Start();
  void Stop();
  void Resize();

  bool initialized() const;

  std::shared_ptr<Entity> MainCamera() const;
  std::shared_ptr<Entity> RegisterEntity(uint32_t component_types);
  int32_t CreateGeometry(uint32_t geometry_type, std::unique_ptr<GeometryData>&& data);
  int32_t LoadTexture(const wchar_t* file_name);
  std::shared_ptr<std::vector<MeshData>> LoadFBXScene(const char* filename);

  // input
  void CaptureMouse();
  int IsKeyDown(char key);
  void OverrideKey(char key, int value);
  void OverrideMouse(int mouse_x, int mouse_y);
  float MouseXAxis();
  float MouseYAxis();

  // This should be private but windowproc needs acces to it
 private:
  static const uint16_t kSwapchainBufferCount = 3;

  std::unique_ptr<RR::Window> _window = nullptr;
  std::unique_ptr<RR::Editor> _editor = nullptr;
  std::shared_ptr<Entity> _main_camera = nullptr;
  std::unique_ptr<RR::Input> _input = nullptr;

  std::vector<GFX::Geometry> _geometries;
  std::vector<GFX::Texture> _textures;
  std::map<uint32_t, GFX::Pipeline> _pipelines;

  // This is memory acces mayhem, 0 cache hits
  // specially when accessing entity components,
  // See: BadBay game engine ECS
  std::list<std::shared_ptr<Entity>> _entities;

  uint16_t _current_frame = 0;
  bool _running = true;
  bool _initialized = false;
  void (*_update)(void* user_data) = nullptr;
  void* _user_data = nullptr;

  ID3D12Device* _device = nullptr;
  IDXGISwapChain3* _swap_chain = nullptr;
  ID3D12CommandQueue* _command_queue = nullptr;
  ID3D12DescriptorHeap* _rt_descriptor_heap = nullptr;
  ID3D12DescriptorHeap* _imgui_descriptor_heap = nullptr;
  ID3D12Resource* _render_targets[kSwapchainBufferCount] = {0};
  ID3D12CommandAllocator* _command_allocators[kSwapchainBufferCount] = {0};
  ID3D12GraphicsCommandList* _command_list = nullptr;
  ID3D12Fence* _fences[kSwapchainBufferCount] = {0};  
  void* _fence_event = nullptr;
  
  ID3D12Resource* _depth_scentil_buffer = nullptr;
  ID3D12DescriptorHeap* _depth_stencil_descriptor_heap = nullptr;

 #ifdef DEBUG
  ID3D12Debug1* _debug_controller = nullptr;
  ID3D12DebugDevice* _debug_device = nullptr;
 #endif

  void UpdateGraphicResources();
  void InternalUpdate();
  void UpdatePipeline();
  void Render();
  void Cleanup();
  void WaitForPreviousFrame();
  void WaitForAllFrames();

  friend class RendererComponent;
};
}

#endif  // !__RENDERER_H__
