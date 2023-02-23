#ifndef __LOCAL_TRANSFORM_COMPONENT_H__
#define __LOCAL_TRANSFORM_COMPONENT_H__ 1

#include <memory>

#include "renderer/common.hpp"
#include "renderer/components/entity_component.h"

namespace RR {
class Entity;
class EntityComponent;

class LocalTransform : public EntityComponent {
 public:
  LocalTransform();
  ~LocalTransform() = default;

  void SetParent(std::shared_ptr<Entity> parent);

  DirectX::XMFLOAT3 position;
  DirectX::XMFLOAT3 rotation;
  DirectX::XMFLOAT3 scale;

  friend class Renderer;
 private:
  uint32_t level;
  std::shared_ptr<Entity> parent;
};
}

#endif  // !__LOCAL_TRANSFORM_COMPONENT_H__
