#ifndef __WORLD_TRANSFORM_COMPONENT_H__
#define __WORLD_TRANSFORM_COMPONENT_H__ 1

#include <memory>

#include "renderer/common.hpp"
#include "renderer/components/entity_component.h"

namespace RR {
class Entity;
class EntityComponent;

class WorldTransform : public EntityComponent {
 public:
  WorldTransform();
  ~WorldTransform() = default;

  DirectX::XMFLOAT3 forward();
  DirectX::XMFLOAT3 right();
  DirectX::XMFLOAT3 up();

  DirectX::XMFLOAT4X4 world;
};
}  // namespace RR

#endif  // !__WORLD_TRANSFORM_COMPONENT_H__