#include "renderer/components/local_transform_component.h"

#include "renderer/entity.h"
#include "renderer/components/entity_component.h"

RR::LocalTransform::LocalTransform() {
  level = 0;
  parent = nullptr;

  position = {0.0f, 0.0f, 0.0f};
  rotation = {0.0f, 0.0f, 0.0f};
  scale = {1.0f, 1.0f, 1.0f};
}

void RR::LocalTransform::SetParent(std::shared_ptr<Entity> parent) {
  if (parent == nullptr) {
    return;
  }

  if (parent->_components.count(kComponentType_LocalTransform) == 0) {
    return;
  }

  std::shared_ptr<LocalTransform> parent_transform =
      std::static_pointer_cast<LocalTransform, EntityComponent>(
          parent->_components[kComponentType_LocalTransform]);

  if (parent_transform == nullptr) {
    return;
  }

  level = parent_transform->level + 1;
  this->parent = parent;
}