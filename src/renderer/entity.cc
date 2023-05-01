#include "renderer/entity.h"

#include "renderer/common.hpp"
#include "renderer/components/entity_component.h"
#include "renderer/components/camera_component.h"
#include "renderer/components/renderer_component.h"
#include "renderer/components/local_transform_component.h"
#include "renderer/components/world_transform_component.h"

RR::Entity::Entity(uint32_t components) { AddComponents(components); }

void RR::Entity::AddComponents(
    uint32_t components) {
  for (uint32_t i = 1; i <= components; i = (i << 1)) {
    if ((components & i) != i) {
      continue;
    }

    switch (i) {
      case ComponentTypes::kComponentType_WorldTransform:
        _components[i] = std::make_shared<WorldTransform>();
        break;
      case ComponentTypes::kComponentType_LocalTransform:
        _components[i] = std::make_shared<LocalTransform>();
        break;
      case ComponentTypes::kComponentType_Renderer:
        _components[i] = std::make_shared<RendererComponent>();
        break;
      case ComponentTypes::kComponentType_Camera:
        _components[i] = std::make_shared<Camera>();
        break;
    }
  }
}

std::shared_ptr<RR::EntityComponent> RR::Entity::GetComponent(
    uint32_t component_type) const {
  return _components.count(component_type) != 0 
    ? _components.at(component_type) : nullptr;
}
