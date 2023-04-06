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

struct PBRTextures {
  int32_t base_color;
  int32_t normal;
  int32_t metallic;
  int32_t roughness;
  int32_t reflectance;
};

struct PBRSettings {
 public:
  float metallic;
  float roughness;
  float reflectance;
  float padding;
  float base_color[4];

 private:
  bool base_color_texture;
  char padding1[3];
  bool normal_texture;
  char padding2[3];
  bool metallic_texture;
  char padding3[3];
  bool roughness_texture;
  char padding4[3];
  bool reflectance_texture;
  char padding5[3];

  friend class RendererComponent;
};

struct PhongSettings {
 public:
  float color[3];
};

union MaterialSettings {
  PBRSettings pbr_settings;
  PhongSettings phong_settings;
};

union TextureSettings {
  PBRTextures pbr_textures;
};

class Renderer;
class RendererComponent : public EntityComponent {
 public:
  RendererComponent() = default;
  ~RendererComponent() = default;

  void Init(const Renderer* renderer, uint32_t pipeline_type, int geometry);

  MaterialSettings settings;
  TextureSettings textureSettings;
  int32_t geometry = -1;
 private:
  uint32_t _pipeline_type = 0U;
  bool _initialized = false;

  std::vector<ID3D12Resource*> _mvp_constant_buffers;
  std::vector<ID3D12Resource*> _material_constant_buffers;
  std::vector<ID3D12DescriptorHeap*> _srv_descriptor_heaps;
  
  void Update(const MVPStruct& mvp, uint32_t frame);
  uint64_t MVPConstantBufferView(uint32_t frame); 
  uint64_t MaterialConstantBufferView(uint32_t frame); 
  void CreateResourceViews(ID3D12Device* device, 
                           std::vector<GFX::Texture>& textures, 
                           uint32_t frame);

  friend class Renderer;
};
}

#endif  // !__RENDERER_COMPONENT_H__
