#include "renderer/components/world_transform_component.h"

#include "renderer/entity.h"
#include "renderer/components/entity_component.h"

RR::WorldTransform::WorldTransform() {
  DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixIdentity());
}

DirectX::XMFLOAT3 RR::WorldTransform::forward() { 
  return {world._31, world._32, world._33}; 
}

DirectX::XMFLOAT3 RR::WorldTransform::right() {
  return {world._11, world._12, world._13};
}

DirectX::XMFLOAT3 RR::WorldTransform::up() { 
  return {world._21, world._22, world._23};
}
