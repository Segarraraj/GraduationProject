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

static struct UserData {
  RR::Renderer* renderer;

  std::shared_ptr<RR::LocalTransform> transform;
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

  data->transform->rotation.y = data->renderer->elapsed_time * 0.02f;
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
  camera->nearZ - 0.001f;

  transform->position = {0.0f, 0.0f, 0.0f};
  transform->rotation = {0.0f, 0.0f, 0.0f};

  std::shared_ptr<RR::Entity> hollow_knight = renderer.LoadFBXScene("../../resources/hollow-knight/source/HollowKnight.fbx")[0];
  hollow_knight->AddComponents(RR::kComponentType_LocalTransform);
  
  data.transform = std::static_pointer_cast<RR::LocalTransform>(
      hollow_knight->GetComponent(RR::kComponentType_LocalTransform));

  data.transform->scale = {.5f, .5f, .5f};

  renderer.Start();

  return 0;
}