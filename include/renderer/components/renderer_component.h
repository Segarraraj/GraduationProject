#ifndef __RENDERER_COMPONENT_H__
#define __RENDERER_COMPONENT_H__ 1

#include <vector>

#include "renderer/common.hpp"
#include "renderer/components/entity_component.h"

struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct ID3D12Device;

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

  void Init(const Renderer* renderer, uint32_t pipeline_type);

  std::vector<int32_t> geometries;
  std::vector<MaterialSettings> settings;
  std::vector<TextureSettings> textureSettings;
 private:
  uint32_t _pipeline_type = 0U;
  bool _initialized = false;

  std::vector<ID3D12Resource*> _mvp_constant_buffers;
  std::vector<ID3D12Resource*> _material_constant_buffers;
  std::vector<ID3D12DescriptorHeap*> _srv_descriptor_heaps;
  
  void SetMVP(const MVPStruct& mvp, uint32_t frame);
  void Update(ID3D12Device* device, std::vector<GFX::Texture>& textures, uint32_t frame, uint32_t geometry);

  uint64_t MVPConstantBufferView(uint32_t frame); 
  uint64_t MaterialConstantBufferView(uint32_t frame);

  friend class Renderer;
};
}

#endif  // !__RENDERER_COMPONENT_H__
