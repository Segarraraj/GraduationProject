#include "renderer/components/world_transform_component.h"

#include "renderer/entity.h"
#include "renderer/components/entity_component.h"

RR::WorldTransform::WorldTransform() {
  DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixIdentity());
}