#ifndef __RENDERER_COMPONENT_H__
#define __RENDERER_COMPONENT_H__ 1

#include <vector>

#include "renderer/common.hpp"
#include "renderer/components/entity_component.h"

struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct ID3D12Device;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

namespace RR {
namespace GFX {
class Texture;
}

struct MVPStruct {
  DirectX::XMFLOAT4X4 model;
  DirectX::XMFLOAT4X4 view;
  DirectX::XMFLOAT4X4 projection;
};

class Renderer;
class RendererComponent : public EntityComponent {
 public:
  RendererComponent() = default;
  ~RendererComponent() = default;

  void Init(const Renderer* renderer, uint32_t pipeline_type, uint32_t geometries);

  // This is dangerous, client can resize
  std::vector<int32_t> geometries;
  std::vector<MaterialSettings> settings;
  std::vector<TextureSettings> textureSettings;
 private:
  uint32_t _pipeline_type = 0U;
  bool _initialized = false;

  ID3D12Resource* _mvp_constant_buffers;
  ID3D12Resource* _material_constant_buffers;
  std::vector<ID3D12DescriptorHeap*> _srv_descriptor_heaps;
  
  void SetMVP(const MVPStruct& mvp);
  void Update(ID3D12Device* device, std::vector<GFX::Texture>& textures, uint32_t geometry);

  uint64_t MVPConstantBufferView(); 
  uint64_t MaterialConstantBufferView();
  ID3D12DescriptorHeap* SRVDescriptorHeap(uint32_t index);

  friend class Renderer;
  friend class Editor;
};
}

#endif  // !__RENDERER_COMPONENT_H__
