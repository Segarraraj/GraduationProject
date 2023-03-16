#include "renderer/renderer.h"

#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <stdlib.h>
#include <time.h>
#include <windowsx.h>

#include <chrono>
#include <string>

#include "OpenFBX/ofbx.h"
#include "Minitrace/minitrace.h"

#include "renderer/common.hpp"
#include "renderer/window.h"
#include "renderer/logger.h"
#include "renderer/entity.h"
#include "renderer/input.h"
#include "renderer/graphics/texture.h"
#include "renderer/graphics/pipeline.h"
#include "renderer/graphics/geometry.h"
#include "renderer/components/camera_component.h"
#include "renderer/components/local_transform_component.h"
#include "renderer/components/world_transform_component.h"
#include "renderer/components/renderer_component.h"


static long long CALLBACK WindowProc(void* window, unsigned int message,
                              unsigned long long wParam, long long lParam) {
  LRESULT result = 0;

  RR::Renderer* renderer = (RR::Renderer*) GetWindowLongPtr((HWND)window, GWLP_USERDATA);

  switch (message) {
    case WM_CLOSE:
      renderer->Stop();
      break;
    case WM_SIZE:
      renderer->Resize();
      break;
    case WM_KEYDOWN:
      renderer->OverrideKey(wParam, 1);
      break;
    case WM_KEYUP:
      renderer->OverrideKey(wParam, 0);
      break;
    case WM_LBUTTONDOWN:
      renderer->CaptureMouse();
      break;
    case WM_MOUSEMOVE: {
      int mouse_x = GET_X_LPARAM(lParam);
      int mouse_y = GET_Y_LPARAM(lParam);

      renderer->OverrideMouse(mouse_x, mouse_y);
      break;
    }
    default: {
      result = DefWindowProc((HWND)window, message, wParam, lParam);
      break;
    }
  }
  return result;
}

RR::Renderer::Renderer() {}

RR::Renderer::~Renderer() {}

int RR::Renderer::Init(void* user_data, void (*update)(void*)) {
  LOG_DEBUG("RR", "Initializing renderer");
  mtr_init("trace.json");

  srand(time(NULL));

  MTR_META_PROCESS_NAME("Graduation Project");
  MTR_META_THREAD_NAME("Main Thread");

  MTR_BEGIN("Renderer", "Init");
  
  // Set client update logic callback
  _update = update;
  _user_data = user_data;

  // Initialize window
  _window = std::make_unique<RR::Window>();
  _input = std::make_unique<RR::Input>();

  _window->Init(GetModuleHandle(NULL), "winclass", "DX12 Graduation Project",
                WindowProc, this);

  _window->Show(); 

  _main_camera = RegisterEntity(ComponentTypes::kComponentType_LocalTransform |
                                ComponentTypes::kComponentType_WorldTransform |
                                ComponentTypes::kComponentType_Camera);

  _geometries = std::vector<GFX::Geometry>(2000);
  _textures = std::vector<GFX::Texture>(700);

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
  _command_queue->SetName(L"Graphics command queue");

  // Create swapchain
  LOG_DEBUG("RR", "Creating swapchain");

  DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
  swapchain_desc.Width = _window->width();
  swapchain_desc.Height = _window->height();
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
    Cleanup();
    return 1;
  }

  unsigned int descriptor_size =
      _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptor_handle(
      _rt_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

  for (uint16_t i = 0; i < kSwapchainBufferCount; i++) {
    _swap_chain->GetBuffer(i, IID_PPV_ARGS(&_render_targets[i]));
    _device->CreateRenderTargetView(_render_targets[i], nullptr, rt_descriptor_handle);
    _render_targets[i]->SetName(L"Render Target");
    rt_descriptor_handle.ptr += (1 * descriptor_size);
  }

  // Create command allocators
  LOG_DEBUG("RR", "Creating command allocators");

  for (uint16_t i = 0; i < kSwapchainBufferCount; i++) {
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
    Cleanup();
    return 1;
  }

  _command_list->Close();

  // Create fences
  LOG_DEBUG("RR", "Creating fences");

  for (uint16_t i = 0; i < kSwapchainBufferCount; i++) {
    result = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fences[i]));
    if (FAILED(result)) {
      LOG_DEBUG("RR", "Couldn't create fence");
      Cleanup();
      return 1;
    }
    _fences[i]->Signal(1);
  }

  _fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  LOG_DEBUG("RR", "Creating depth stencil buffer");
  // Create depth stencil descriptor heap
  D3D12_DESCRIPTOR_HEAP_DESC depth_stencil_descriptor_heap_desc = {};
  depth_stencil_descriptor_heap_desc.NumDescriptors = 1;
  depth_stencil_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  depth_stencil_descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  result = _device->CreateDescriptorHeap(&depth_stencil_descriptor_heap_desc,
      IID_PPV_ARGS(&_depth_stencil_descriptor_heap));

  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create depth stencil descriptor heap");
    Cleanup();
    return 1;
  }

  D3D12_HEAP_PROPERTIES default_heap_properties = {};
  default_heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
  default_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  default_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_desc = {};
  depth_stencil_desc.Format = DXGI_FORMAT_D32_FLOAT;
  depth_stencil_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  depth_stencil_desc.Flags = D3D12_DSV_FLAG_NONE;

  D3D12_CLEAR_VALUE depth_stencil_clear_values = {};
  depth_stencil_clear_values.Format = DXGI_FORMAT_D32_FLOAT;
  depth_stencil_clear_values.DepthStencil.Depth = 1.0f;
  depth_stencil_clear_values.DepthStencil.Stencil = 0;

  D3D12_RESOURCE_DESC depth_stencil_buffer_desc = {};
  depth_stencil_buffer_desc.Format = DXGI_FORMAT_D32_FLOAT;
  depth_stencil_buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  depth_stencil_buffer_desc.Alignment = 0;
  depth_stencil_buffer_desc.Width = _window->width();
  depth_stencil_buffer_desc.Height = _window->height();
  depth_stencil_buffer_desc.DepthOrArraySize = 1;
  depth_stencil_buffer_desc.MipLevels = 0;
  depth_stencil_buffer_desc.SampleDesc.Count = 1;
  depth_stencil_buffer_desc.SampleDesc.Quality = 0;
  depth_stencil_buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  depth_stencil_buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

  result = _device->CreateCommittedResource(
      &default_heap_properties, D3D12_HEAP_FLAG_NONE,
      &depth_stencil_buffer_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, 
      &depth_stencil_clear_values, IID_PPV_ARGS(&_depth_scentil_buffer));
  if (FAILED(result)) {
    LOG_ERROR("RR", "Couldn't create depth stencil buffer");
    Cleanup();
  }

#ifdef DEBUG
  _depth_scentil_buffer->SetName(L"Depth/Stencil buffer");
  _depth_stencil_descriptor_heap->SetName(L"Depth/Stencil descriptor heap");
#endif  // DEBUG

  _device->CreateDepthStencilView(
      _depth_scentil_buffer, &depth_stencil_desc,
      _depth_stencil_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

  printf("\n");
  LOG_DEBUG("RR", "Initializing pipelines");
  _pipelines[RR::PipelineTypes::kPipelineType_PBR] = RR::GFX::Pipeline();
  _pipelines[RR::PipelineTypes::kPipelineType_PBR].Init(
      _device, kPipelineType_PBR, kGeometryType_Positions_Normals_UV);

  _pipelines[RR::PipelineTypes::kPipelineType_Phong] = RR::GFX::Pipeline();
  _pipelines[RR::PipelineTypes::kPipelineType_Phong].Init(
        _device, kPipelineType_Phong, kGeometryType_Positions_Normals_UV);

  printf("\n");
  LOG_DEBUG("RR", "Renderer initialized");
  LOG_DEBUG("RR", "    Available geometries: %i", _geometries.size());
  LOG_DEBUG("RR", "    Available textures: %i", _textures.size());

  MTR_END("Renderer", "Init");
  return 0;
}

void RR::Renderer::Start() {
  std::chrono::time_point<std::chrono::steady_clock> renderer_start, frame_start, frame_end;
  renderer_start = std::chrono::high_resolution_clock::now();
  
  while (_running) {
    MTR_BEGIN("Renderer", "Frame CPU wait");
    do {
      frame_end = std::chrono::high_resolution_clock::now();
      delta_time = std::chrono::duration<float, std::milli>(frame_end - frame_start).count();
    } while (delta_time < (1000.0f / 60.0f));
    MTR_END("Renderer", "Frame CPU wait");

    MTR_BEGIN("Renderer", "Frame");

    frame_start = std::chrono::high_resolution_clock::now();
    _current_frame = _swap_chain->GetCurrentBackBufferIndex();
    elapsed_time = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - renderer_start).count();


    if (_window->isCaptureMouse()) {
      _input->Flush(_window->width(), _window->height(), _window->width() / 2,
                    _window->height() / 2);
    } else {
      _input->Flush(_window->width(), _window->height(), -1, -1);
    }

    MTR_BEGIN("Renderer", "Win 32 API message dispatch");
    MSG message;
    while (PeekMessage(&message, (HWND)_window->window(), 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessage(&message);
    }
    MTR_END("Renderer", "Win 32 API message dispatch");

    MTR_BEGIN("Renderer", "Update graphic resources");
    UpdateGraphicResources();
    MTR_END("Renderer", "Update graphic resources");

    MTR_BEGIN("Renderer", "Wait for GPU");
    WaitForPreviousFrame();
    MTR_END("Renderer", "Wait for GPU");
    
    MTR_BEGIN("Renderer", "Client update");
    _update(_user_data);
    MTR_END("Renderer", "Client update");

    MTR_BEGIN("Renderer", "Internal update");
    InternalUpdate();
    MTR_END("Renderer", "Internal update");

    MTR_BEGIN("Renderer", "Update pipeline");
    UpdatePipeline();
    MTR_END("Renderer", "Update pipeline");

    MTR_BEGIN("Renderer", "Render");
    Render();
    MTR_END("Renderer", "Render");

    MTR_END("Renderer", "Frame");
  }

  Cleanup();
}

void RR::Renderer::Stop() { 
  _running = false;
}

void RR::Renderer::Resize() {
  if (_swap_chain == nullptr) {
    return;
  }

  for (_current_frame = 0; _current_frame < kSwapchainBufferCount; _current_frame++) {
    WaitForPreviousFrame();
  }

  for (uint16_t i = 0; i < kSwapchainBufferCount; i++) {
    _render_targets[i]->Release();
  }

  // todo resize depth stencil buffer
  _swap_chain->ResizeBuffers(kSwapchainBufferCount, _window->width(),
                              _window->height(), DXGI_FORMAT_UNKNOWN, 0);
}

std::shared_ptr<RR::Entity> RR::Renderer::MainCamera() const {
  return _main_camera;
}

std::shared_ptr<RR::Entity> RR::Renderer::RegisterEntity(
    uint32_t component_types) {
  if (component_types == RR::ComponentTypes::kComponentType_None) {
    LOG_WARNING("RR", "Trying to register empty entity");
    return nullptr;
  }

  std::shared_ptr<Entity> new_entity = std::make_shared<Entity>(component_types);
  _entities.push_back(new_entity);
  return new_entity;
}

int32_t RR::Renderer::CreateGeometry(uint32_t geometry_type, std::unique_ptr<GeometryData>&& data) {
  for (size_t i = 0; i < _geometries.size(); i++) {
    if (_geometries[i].Initialized()) {
      continue;
    }

    _geometries[i].Init(_device, geometry_type, std::move(data));
    return i;
  }

  return -1;
}

int32_t RR::Renderer::LoadTexture(const wchar_t* file_name) {
  for (size_t i = 0; i < _textures.size(); i++) {
    if (_textures[i].Initialized()) {
      continue;
    }

    _textures[i].Init(_device, file_name);
    return i;
  }

  return -1;
}

std::vector<std::shared_ptr<RR::Entity>> RR::Renderer::LoadFBXScene(const char* filename) {
  std::vector<std::shared_ptr<Entity>> entities(0);
  
  LOG_DEBUG("RR", "Loading FBX: %s", filename);

  FILE* file = fopen(filename, "rb");

  if (!file) {
    LOG_ERROR("RR", "Couldn't open file: %s", filename);
    return entities;
  }

  MTR_BEGIN("Renderer", "Load FBX scene");
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  auto* content = new ofbx::u8[file_size];
  fread(content, 1, file_size, file);
  ofbx::IScene* scene = ofbx::load((ofbx::u8*)content, file_size,
                       (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);

  char texture_name[128] = {0};
  wchar_t real_texture_name[256] = {0};
  std::map<std::string, int> textures;
  
  delete[] content;
  fclose(file);

  if (scene == nullptr) {
    return entities;
  }
  
  int mesh_count = scene->getMeshCount();
  const ofbx::GlobalSettings* settings = scene->getGlobalSettings();
  for (int i = 0; i < mesh_count; i++) {
    const ofbx::Mesh& mesh = *scene->getMesh(i);
    const ofbx::Geometry& geom = *mesh.getGeometry();
    LOG_DEBUG("RR", "Mesh %i/%i: %s", i, mesh_count, mesh.name);

    const ofbx::Vec3* vertices = geom.getVertices();
    const ofbx::Vec3* normals = geom.getNormals();
    const ofbx::Vec2* uvs = geom.getUVs();
    const int* material_indices = geom.getMaterials();

    const char* mesh_name = mesh.name;
    const char* geo_name = geom.name;

    int index_count = geom.getIndexCount();
    int vertex_count = geom.getVertexCount();
    int material_count = mesh.getMaterialCount();

    bool has_normals = normals != nullptr;
    bool has_uvs = uvs != nullptr;

    ofbx::Matrix world = mesh.getGlobalTransform();

    std::map<int, std::list<std::unique_ptr<GeometryData>>> material_data;

    int vertex_offset = 3 + (has_normals ? 3 : 0) + (has_uvs ? 2 : 0);

    if (material_count != 1) {
      int material_index = 0;
      int previous_count = 0;
      for (int j = 0; j <= index_count / 3; j++) {
        if (material_indices[j] == material_index && j != index_count / 3) {
          continue;
        }

        std::unique_ptr<GeometryData> data = std::make_unique<GeometryData>();
        data->index_data.resize((j - previous_count) * 3);
        data->vertex_data.resize(((j - previous_count) * 3) * vertex_offset);

        material_data[material_index].push_back(std::move(data));

        material_index = material_indices[j];
        previous_count = j;
      }
    } else {
      std::unique_ptr<GeometryData> data = std::make_unique<GeometryData>();
      data->index_data.resize(index_count);
      data->vertex_data.resize(vertex_count * vertex_offset);

      material_data[0].push_back(std::move(data));
    }

    int previous_submesh_count = 0;
    for (std::map<int, std::list<std::unique_ptr<GeometryData>>>::iterator i =
             material_data.begin();
         i != material_data.end(); i++) {
      for (std::list<std::unique_ptr<GeometryData>>::iterator data =
               i->second.begin();
           data != i->second.end(); data++) {
        for (int j = 0; j < data->get()->index_data.size(); j++) {
          data->get()->index_data[j] = j;
          data->get()->vertex_data[j * vertex_offset + 0] =
              (float)vertices[previous_submesh_count + j].x;
          data->get()->vertex_data[j * vertex_offset + 1] =
              (float)vertices[previous_submesh_count + j].y;
          data->get()->vertex_data[j * vertex_offset + 2] =
              (float)vertices[previous_submesh_count + j].z;
          if (has_normals) {
            data->get()->vertex_data[j * vertex_offset + 3] =
                (float)normals[previous_submesh_count + j].x;
            data->get()->vertex_data[j * vertex_offset + 4] =
                (float)normals[previous_submesh_count + j].y;
            data->get()->vertex_data[j * vertex_offset + 5] =
                (float)normals[previous_submesh_count + j].z;
          }
          if (has_uvs) {
            data->get()->vertex_data[j * vertex_offset + 6] =
                (float)uvs[previous_submesh_count + j].x;
            data->get()->vertex_data[j * vertex_offset + 7] =
                1.0f - (float)uvs[previous_submesh_count + j].y;
          }
        }

        previous_submesh_count += data->get()->index_data.size();

        std::shared_ptr<Entity> entity =
            RegisterEntity(ComponentTypes::kComponentType_WorldTransform |
                           ComponentTypes::kComponentType_Renderer);
        entities.push_back(entity);

        std::shared_ptr<WorldTransform> transform =
            std::static_pointer_cast<WorldTransform>(entity->GetComponent(
                ComponentTypes::kComponentType_WorldTransform));

        DirectX::XMMATRIX matrix((float)world.m[0], (float)world.m[1],
                                 (float)world.m[2], (float)world.m[3],

                                 (float)world.m[4], ((float)world.m[5]),
                                 (float)world.m[6], (float)world.m[7],

                                 (float)world.m[8], (float)world.m[9],
                                 (float)world.m[10], (float)world.m[11],

                                 (float)world.m[12], (float)world.m[13],
                                 (float)world.m[14], (float)world.m[15]);

        DirectX::XMStoreFloat4x4(&transform->world, matrix);

        uint32_t geometry_type = GeometryTypes::kGeometryType_None;

        if (has_normals && has_uvs) {
          geometry_type = GeometryTypes::kGeometryType_Positions_Normals_UV;
        } else if (has_normals) {
          geometry_type = GeometryTypes::kGeometryType_Positions_Normals;
        }

        int geometry_handle = CreateGeometry(geometry_type, std::move(*data));

        std::shared_ptr<RendererComponent> renderer_component =
            std::static_pointer_cast<RendererComponent>(
                entity->GetComponent(ComponentTypes::kComponentType_Renderer));

        const ofbx::Material* material = mesh.getMaterial(i->first);
        if (material != nullptr) {
          const ofbx::Texture* texture =
              material->getTexture(ofbx::Texture::DIFFUSE);
          if (texture != nullptr) {
            texture->getRelativeFileName().toString(texture_name);

            int texture_handle = -1;
            std::string texture_key = texture_name;
            if (textures.count(texture_key) == 1) {
              texture_handle = textures[texture_name];
            } else {
              char resources[256] = "../../resources/\0";
              char* appended_name = strcat(resources, texture_name);
              mbstowcs(real_texture_name, appended_name, 256);
              texture_handle = LoadTexture(real_texture_name);
              ZeroMemory(real_texture_name, 256);
              textures[texture_key] = texture_handle;
            }

            renderer_component->settings.pbr_settings.texture = texture_handle;
          }
        }

        //          float color_component = 1.0f - (1.0f / 8.0f) * i->first;
        // float color[] = {color_component, color_component, color_component};
        // memcpy(renderer_component->settings.phong_settings.color, color,
        //       sizeof(color));
        renderer_component->Init(this, PipelineTypes::kPipelineType_PBR,
                                 geometry_handle);
      }
    }

    UpdateGraphicResources();
  }

  scene->destroy();  

  MTR_END("Renderer", "Load FBX scene");

  return entities;
}

void RR::Renderer::CaptureMouse() { 
  _window->CaptureMouse();

  bool capture = _window->isCaptureMouse();
  ShowCursor(capture);

  if (!capture) {
    return;
  }

  SetCursorPos(_window->screenCenterX(), _window->screenCenterY());
}

int RR::Renderer::IsKeyDown(char key) { return _input->keys[key]; }

void RR::Renderer::OverrideKey(char key, int value) {
  _input->keys[key] = value;
}

void RR::Renderer::OverrideMouse(int mouse_x, int mouse_y) {
  POINT pos = {};
  GetCursorPos(&pos);

  int center_x = _window->screenCenterX();
  int center_y = _window->screenCenterY();

  if (center_x == pos.x && center_y == pos.y) {
    return;
  }

  _input->SetMouse(mouse_x, mouse_y);


  if (!_window->isCaptureMouse()) {
    return;  
  }

  SetCursorPos(_window->screenCenterX(), _window->screenCenterY());
}

float RR::Renderer::MouseXAxis() {
  return _input->MouseXAxis();
  ;
}

float RR::Renderer::MouseYAxis() { 
  return _input->MouseYAxis(); 
}

void RR::Renderer::UpdateGraphicResources() {
  size_t i = 0;
  size_t j = 0;
  bool update_geometries = false;
  bool update_textures = false;
  for (; i < _geometries.size() && !update_geometries; i++) {
    if (!_geometries[i].Updated() && _geometries[i].Initialized()) {
      update_geometries = true;
    }
  }

  for (; j < _textures.size() && !update_textures; j++) {
    if (!_textures[j].Updated() && _textures[j].Initialized()) {
      update_textures = true;
    }
  }

  if (!update_geometries && !update_textures) {
    return;
  }

  WaitForAllFrames();

  _command_allocators[_current_frame]->Reset();
  _command_list->Reset(_command_allocators[_current_frame], nullptr);

  MTR_BEGIN("Renderer", "Update geometries");
  for (i = i - 1; i < _geometries.size(); i++) {
    if (!_geometries[i].Updated() && _geometries[i].Initialized()) {
      _geometries[i].Update(_command_list);
    }
  }
  MTR_END("Renderer", "Update geometries");

  MTR_BEGIN("Renderer", "Update textures");
  for (j = j - 1; j < _textures.size(); j++) {
    if (!_textures[j].Updated() && _textures[j].Initialized()) {
      _textures[j].Update(_device, _command_list);
    }
  }
  MTR_END("Renderer", "Update textures");

  _command_list->Close();

  ID3D12CommandList* lists[] = {_command_list};

  _fences[_current_frame]->Signal(0);
  _command_queue->ExecuteCommandLists(1, lists);
  _command_queue->Signal(_fences[_current_frame], 1);
}

void RR::Renderer::InternalUpdate() {
  std::map<uint32_t, std::list<std::pair<std::shared_ptr<LocalTransform>, std::shared_ptr<WorldTransform>>>> components;

  MTR_BEGIN("Renderer", "Populate local transform list");
  // Prepare parent and child components
  // 💀💀💀
  uint32_t max_parent_level = 0;
  for (uint32_t level = 0; level <= max_parent_level; level++) {
    for (std::list<std::shared_ptr<Entity>>::iterator i = _entities.begin(); i != _entities.end(); i++) {
      std::shared_ptr<LocalTransform> local_transform =
          std::static_pointer_cast<LocalTransform>(
              i->get()->GetComponent(ComponentTypes::kComponentType_LocalTransform));
    
      if (local_transform == nullptr) {
        continue;
      }

      if (local_transform->level > max_parent_level) {
        max_parent_level = local_transform->level;
      }

      std::shared_ptr<WorldTransform> world_transform =
          std::static_pointer_cast<WorldTransform>(
              i->get()->GetComponent(ComponentTypes::kComponentType_WorldTransform));

      std::pair<std::shared_ptr<LocalTransform>, std::shared_ptr<WorldTransform>> pair;

      pair.first = local_transform;
      pair.second = world_transform;
      components[local_transform->level].push_back(pair);
    }
  }
  MTR_END("Renderer", "Populate local transform list");

  MTR_BEGIN("Renderer", "Update world transforms");
  // Calculate world positions
  for (uint32_t level = 0; level <= max_parent_level; level++) {
    for (std::list<std::pair<std::shared_ptr<LocalTransform>, std::shared_ptr<WorldTransform>>>::iterator i = components[level].begin();
         i != components[level].end(); i++) {

       DirectX::XMMATRIX world = DirectX::XMMatrixIdentity() * 
         DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&i->first->scale))
         * DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(i->first->rotation.x)) *
               DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(i->first->rotation.y)) *
               DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(i->first->rotation.z)) * 
         DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&i->first->position));


       if (level != 0) {
         std::shared_ptr<WorldTransform> parent_world =
             std::static_pointer_cast<WorldTransform>(
                 i->first->parent->GetComponent(
                     ComponentTypes::kComponentType_WorldTransform));

         world = world * DirectX::XMLoadFloat4x4(&parent_world->world);
       }

       DirectX::XMStoreFloat4x4(&i->second->world, world);
    }
  }
  MTR_END("Renderer", "Update world transforms");
}

void RR::Renderer::UpdatePipeline() { 
  HRESULT result = _command_allocators[_current_frame]->Reset();
  if (FAILED(result)) {
    _running = false;
    return;
  }

  result = _command_list->Reset(_command_allocators[_current_frame], nullptr);
  if (FAILED(result)) {
    _running = false;
    return;
  }

  MTR_BEGIN("Renderer", "Update main camera");
  std::shared_ptr<WorldTransform> camera_world =
      std::static_pointer_cast<WorldTransform>(_main_camera->GetComponent(
          ComponentTypes::kComponentType_WorldTransform));

  std::shared_ptr<Camera> camera = std::static_pointer_cast<Camera>(
      _main_camera->GetComponent(ComponentTypes::kComponentType_Camera));

  DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(
      DirectX::XMVectorSet(camera_world->world._41, camera_world->world._42,
                           camera_world->world._43, 0.0f),
      DirectX::XMVectorSet(camera_world->world._31, camera_world->world._32,
                           camera_world->world._33, 0.0f),
      DirectX::XMVectorSet(camera_world->world._21, camera_world->world._22,
                           camera_world->world._23, 0.0f));

  DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(
      camera->fov * (3.14f / 180.0f), _window->aspectRatio(), camera->nearZ,
      camera->farZ);
  MTR_END("Renderer", "Update main camera");

  MTR_BEGIN("Renderer", "Populate render list");
  std::map<uint32_t, std::list<std::shared_ptr<RendererComponent>>> render_list;
  for (std::list<std::shared_ptr<Entity>>::iterator i = _entities.begin();
       i != _entities.end(); i++) {
    std::shared_ptr<RendererComponent> renderer =
        std::static_pointer_cast<RendererComponent>(
            i->get()->GetComponent(ComponentTypes::kComponentType_Renderer));

    std::shared_ptr<WorldTransform> world_transform =
        std::static_pointer_cast<WorldTransform>(i->get()->GetComponent(
            ComponentTypes::kComponentType_WorldTransform));

    if (renderer == nullptr || world_transform == nullptr) {
      continue;
    }

    if (!renderer->_initialized) {
      continue;
    }

    RendererSettings settings = {};
    switch (renderer->_pipeline_type) {
      case RR::PipelineTypes::kPipelineType_PBR:
        DirectX::XMStoreFloat4x4(&settings.pbr_settings.mvp.view,
                                 DirectX::XMMatrixTranspose(view));
        DirectX::XMStoreFloat4x4(&settings.pbr_settings.mvp.projection,
                                 DirectX::XMMatrixTranspose(projection));
        DirectX::XMStoreFloat4x4(
            &settings.pbr_settings.mvp.model,
            DirectX::XMMatrixTranspose(
                DirectX::XMLoadFloat4x4(&world_transform->world)));
        renderer->Update(settings, _current_frame);
        break;
      case RR::PipelineTypes::kPipelineType_Phong:
        DirectX::XMStoreFloat4x4(&settings.phong_settings.mvp.view,
                                 DirectX::XMMatrixTranspose(view));
        DirectX::XMStoreFloat4x4(&settings.phong_settings.mvp.projection,
                                 DirectX::XMMatrixTranspose(projection));
        DirectX::XMStoreFloat4x4(&settings.phong_settings.mvp.model,
            DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&world_transform->world)));

        renderer->Update(settings, _current_frame);
        break;
    }
    render_list[renderer->_pipeline_type].push_back(renderer);
  }
  MTR_END("Renderer", "Populate render list");

  D3D12_RESOURCE_BARRIER rt_render_barrier = {};
  rt_render_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  rt_render_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  rt_render_barrier.Transition.pResource = _render_targets[_current_frame];
  rt_render_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  rt_render_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  rt_render_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  _command_list->ResourceBarrier(1, &rt_render_barrier);

  unsigned int rt_descriptor_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  unsigned int depth_descriptor_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  
  D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptor_handle(
      _rt_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

  D3D12_CPU_DESCRIPTOR_HANDLE depth_descriptor_handle(
      _depth_stencil_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
  
  rt_descriptor_handle.ptr += _current_frame * rt_descriptor_size;

  _command_list->OMSetRenderTargets(1, 
      &rt_descriptor_handle, FALSE, 
      &depth_descriptor_handle);

  _command_list->ClearRenderTargetView(
      rt_descriptor_handle, camera->clear_color, 0, nullptr);

  _command_list->ClearDepthStencilView(
      depth_descriptor_handle, D3D12_CLEAR_FLAG_DEPTH, 
      1.0f, 0, 0, nullptr);

  D3D12_VIEWPORT viewport = {};
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = _window->width();
  viewport.Height = _window->height();
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;

  D3D12_RECT scissor_rect = {};
  scissor_rect.left = 0;
  scissor_rect.top = 0;
  scissor_rect.right = _window->width();
  scissor_rect.bottom = _window->height();

  _command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _command_list->RSSetViewports(1, &viewport);
  _command_list->RSSetScissorRects(1, &scissor_rect);

  MTR_BEGIN("Renderer", "Populate command list");
  // CHANGE PipelineTypes values to change sorting and render order
  for (std::map<uint32_t, std::list<std::shared_ptr<RendererComponent>>>::iterator i = render_list.begin();
       i != render_list.end(); i++) {
    GFX::Pipeline pipeline = _pipelines[i->first];
    _command_list->SetPipelineState(pipeline.PipelineState());
    _command_list->SetGraphicsRootSignature(pipeline.RootSignature());
    
    for (std::list<std::shared_ptr<RendererComponent>>::iterator j = i->second.begin();
         j != i->second.end(); j++) {

      if (j->get()->geometry < 0 || j->get()->geometry > _geometries.size()) {
        LOG_WARNING("RR", "Renderer has invalid geometry");
        continue;
      }

      if (pipeline.GeometryType() != _geometries[j->get()->geometry].Type()) {
        LOG_WARNING("RR", "Traying to draw geometry with incompatible pipeline");
        continue;
      }

      if (!j->get()->_resource_views_created) {
        j->get()->CreateResourceViews(_device, _textures);
      }

      _command_list->IASetVertexBuffers( 0, 1, _geometries[j->get()->geometry].VertexView());
      _command_list->IASetIndexBuffer(_geometries[j->get()->geometry].IndexView());
      _command_list->SetGraphicsRootConstantBufferView(0, j->get()->ConstantBufferView(_current_frame));
      
      
      switch (i->first) { 
        case RR::PipelineTypes::kPipelineType_PBR: {
          ID3D12DescriptorHeap* descriptor_heaps[] = {j->get()->_descriptor_heap};
          _command_list->SetDescriptorHeaps(1, descriptor_heaps);
          _command_list->SetGraphicsRootDescriptorTable(1, descriptor_heaps[0]->GetGPUDescriptorHandleForHeapStart());
          break;
        }          
        case RR::PipelineTypes::kPipelineType_Phong: {
          break;        
        }
      }
      
      _command_list->DrawIndexedInstanced(_geometries[j->get()->geometry].Indices(), 1, 0, 0, 0);
    }
  }
  MTR_END("Renderer", "Populate command list");

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

  _fences[_current_frame]->Signal(0);
  _command_queue->ExecuteCommandLists(sizeof(command_lists) / sizeof(ID3D12CommandList), command_lists);
  _command_queue->Signal(_fences[_current_frame], 1);
  _swap_chain->Present(0, 0);
}

void RR::Renderer::Cleanup() {
  WaitForAllFrames();

  mtr_flush();
  mtr_shutdown();

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

  if (_depth_scentil_buffer != nullptr) {
    _depth_scentil_buffer->Release();
    _depth_scentil_buffer = nullptr;
  } 
  
  if (_depth_stencil_descriptor_heap != nullptr) {
    _depth_stencil_descriptor_heap->Release();
    _depth_stencil_descriptor_heap = nullptr;
  }

  for (std::map<uint32_t, GFX::Pipeline>::iterator i = _pipelines.begin(); i != _pipelines.end(); i++) {
    i->second.Release();
  }

  for (size_t i = 0; i < _geometries.size(); i++) {
    _geometries[i].Release();
  }

  for (uint16_t i = 0; i < kSwapchainBufferCount; ++i) {
    if (_command_allocators[i] != nullptr) {
      _command_allocators[i]->Release();
      _command_allocators[i] = nullptr;
    }

    if (_fences[i] != nullptr) {
      _fences[i]->Release();
      _fences[i] = nullptr;
    }

    if (_render_targets[i] != nullptr) {
      _render_targets[i]->Release();
      _render_targets[i] = nullptr;
    }
  }
}

void RR::Renderer::WaitForPreviousFrame() {
  if (_fences[_current_frame]->GetCompletedValue() != 1) {
    _fences[_current_frame]->SetEventOnCompletion(1, _fence_event);
    WaitForSingleObject(_fence_event, INFINITE);
  }
}

void RR::Renderer::WaitForAllFrames() { 
  uint16_t old_frame = _current_frame;
  for (_current_frame = 0; _current_frame < kSwapchainBufferCount; _current_frame++) {
    WaitForPreviousFrame();
  }
  _current_frame = old_frame;
}
