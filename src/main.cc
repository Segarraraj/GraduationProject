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
  std::shared_ptr<RR::Entity> parent;
};

static void update(void* user_data) { 
  UserData* data = (UserData*) user_data;

  // UPDATE CAMERA
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

  // UPDATE SCENE
  transform = std::static_pointer_cast<RR::LocalTransform>(
      data->parent->GetComponent(RR::kComponentType_LocalTransform));
  transform->rotation.y += data->renderer->delta_time * 0.05f;  
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

  transform->position = {4.996f, 2.5f, 0.0f};
  transform->rotation = {15.0f, -90.0f, 0.0f};

  std::shared_ptr<std::vector<RR::MeshData>> meshes =
    renderer.LoadFBXScene("../../resources/Helmets.fbx");

  data.parent = renderer.RegisterEntity(
      RR::ComponentTypes::kComponentType_LocalTransform |
      RR::ComponentTypes::kComponentType_WorldTransform);

  for (int i = 0; i < meshes.get()->size(); i++) {
    RR::MeshData* mesh = &meshes.get()->at(i);

    std::shared_ptr<RR::Entity> ent = renderer.RegisterEntity(
        RR::ComponentTypes::kComponentType_Renderer |
        RR::ComponentTypes::kComponentType_WorldTransform | 
        RR::ComponentTypes::kComponentType_LocalTransform);

    std::shared_ptr<RR::LocalTransform> transform =
        std::static_pointer_cast<RR::LocalTransform>(ent.get()->GetComponent(
            RR::ComponentTypes::kComponentType_LocalTransform));

    std::shared_ptr<RR::RendererComponent> renderer_c =
        std::static_pointer_cast<RR::RendererComponent>(ent.get()->GetComponent(
            RR::ComponentTypes::kComponentType_Renderer));

    renderer_c->Init(&renderer, RR::PipelineTypes::kPipelineType_PBR, mesh->geometries.size());

    transform->position = mesh->position;
    transform->rotation = mesh->rotation;
    transform->scale = mesh->scale;

    transform->SetParent(data.parent);

    for (int j = 0; j < mesh->geometries.size(); j++) {  
      renderer_c->geometries[j] = mesh->geometries[j];
      renderer_c->settings[j].pbr_settings = mesh->settings[j];
      renderer_c->textureSettings[j].pbr_textures = mesh->textures[j];
    }
  }

  meshes = nullptr;

  renderer.Start();

  return 0;
}