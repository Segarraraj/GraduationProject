#include "renderer/renderer.h"

#include "renderer/input.h"
#include "renderer/entity.h"
#include "renderer/logger.h"
#include "renderer/common.hpp"
#include "renderer/components/entity_component.h"
#include "renderer/components/local_transform_component.h"
#include "renderer/components/world_transform_component.h"
#include "renderer/components/renderer_component.h"
#include "renderer/components/camera_component.h"

struct UserData {
  RR::Renderer* renderer;
  std::shared_ptr<RR::LocalTransform> tr;
  std::shared_ptr<RR::RendererComponent> rd;
};

static void update(void* user_data) { 
  UserData* data = (UserData*) user_data;

  std::shared_ptr<RR::LocalTransform> transform =
      std::static_pointer_cast<RR::LocalTransform>(
          data->renderer->MainCamera()->GetComponent(
              RR::kComponentType_LocalTransform));

  std::shared_ptr<RR::WorldTransform> world =
      std::static_pointer_cast<RR::WorldTransform>(
          data->renderer->MainCamera()->GetComponent(
              RR::kComponentType_WorldTransform));

  transform->rotation.y += data->renderer->MouseXAxis() * 16.0f;
  transform->rotation.x += data->renderer->MouseYAxis() * 8.0f;

  DirectX::XMVECTOR traslation = DirectX::XMLoadFloat3(&transform->position);
  DirectX::XMFLOAT3 delta = world->forward();

  traslation = DirectX::XMVectorAdd(
      traslation,
      DirectX::XMVectorScale(DirectX::XMLoadFloat3(&delta),
                             0.085f * data->renderer->IsKeyDown('W')));

  traslation = DirectX::XMVectorAdd(
      traslation,
      DirectX::XMVectorScale(DirectX::XMLoadFloat3(&delta),
                             -0.085f * data->renderer->IsKeyDown('S')));
  
  delta = world->right();

  traslation = DirectX::XMVectorAdd(
      traslation,
      DirectX::XMVectorScale(DirectX::XMLoadFloat3(&delta),
                             0.05f * data->renderer->IsKeyDown('D')));

  traslation = DirectX::XMVectorAdd(
      traslation,
      DirectX::XMVectorScale(DirectX::XMLoadFloat3(&delta),
                             -0.05f * data->renderer->IsKeyDown('A')));

  DirectX::XMStoreFloat3(&transform->position, traslation);

  //data->tr->rotation.y += data->renderer->delta_time * .1f;
 
}

int main(int argc, char** argv) {
  RR::Renderer renderer;
  UserData data = {};

  int result = renderer.Init((void*) &data, update);
  if (result != 0) {
    return 1;
  }

  data.renderer = &renderer;

  
  std::shared_ptr<RR::LocalTransform> transform = std::static_pointer_cast<RR::LocalTransform>(
      renderer.MainCamera()->GetComponent(RR::kComponentType_LocalTransform));
  
  std::shared_ptr<RR::Camera> camera = std::static_pointer_cast<RR::Camera>(
      renderer.MainCamera()->GetComponent(RR::kComponentType_Camera));

  camera->farZ = 500.0f;
  camera->nearZ = 0.001f;

  transform->position = {0.0f, 0.5f, -10.0f};
  transform->rotation = {0.0f, 0.0f, 0.0f};

  std::shared_ptr<RR::Entity> ent =
      renderer.LoadFBXScene("../../resources/sphere.fbx")[0];

  ent->AddComponents(RR::kComponentType_LocalTransform);
  data.tr = std::static_pointer_cast<RR::LocalTransform>(ent->GetComponent(RR::kComponentType_LocalTransform));
  data.rd = std::static_pointer_cast<RR::RendererComponent>(ent->GetComponent(RR::kComponentType_Renderer));

  data.tr->scale.x = 100.0f;
  data.tr->scale.y = 100.0f;
  data.tr->scale.z = 100.0f;

  renderer.Start();

  return 0;
}