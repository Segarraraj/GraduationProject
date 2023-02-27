#ifndef __RENDERER_COMPONENT_H__
#define __RENDERER_COMPONENT_H__ 1

#include <vector>

#include "renderer/common.hpp"
#include "renderer/components/entity_component.h"

struct ID3D12Resource;

namespace RR {
struct MVPStruct {
  DirectX::XMFLOAT4X4 model;
  DirectX::XMFLOAT4X4 view;
  DirectX::XMFLOAT4X4 projection;
};

struct PBRSettings {
 public:
 private:
  MVPStruct mvp;

  friend class Renderer;
  friend class RendererComponent;
};

union RendererSettings {
  PBRSettings pbr_settings;
};

class Renderer;
class RendererComponent : public EntityComponent {
 public:
  RendererComponent() = default;
  ~RendererComponent() = default;

  void Init(const Renderer* renderer, uint32_t pipeline_type, int geometry);

  RendererSettings settings;
  int32_t geometry = -1;
 private:
  uint32_t _pipeline_type = 0U;
  bool _initialized = false;

  // FIXME: This is dangerous and bad
  std::vector<ID3D12Resource*> _constant_buffers;
  
  void Update(const RendererSettings& settings, uint32_t frame);
  uint64_t ConstantBufferView(uint32_t frame); 

  friend class Renderer;
};
}

#endif  // !__RENDERER_COMPONENT_H__
