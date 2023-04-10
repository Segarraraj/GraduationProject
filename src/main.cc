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
  std::vector<RR::LocalTransform*> lt;
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
  for (int i = 0; i < data->lt.size(); i++) {
    data->lt[i]->rotation.y += data->renderer->delta_time * .1f;
  }
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

  std::shared_ptr<std::vector<RR::MeshData>> meshes =
    renderer.LoadFBXScene("../../resources/StormTrooper.fbx");

  for (int i = 0; i < meshes.get()->size(); i++) {
    RR::MeshData* mesh = &meshes.get()->at(i);

    for (int j = 0; j < mesh->geometries.size(); j++) {
      std::shared_ptr<RR::Entity> ent = renderer.RegisterEntity(
          RR::ComponentTypes::kComponentType_Renderer |
          RR::ComponentTypes::kComponentType_WorldTransform | 
          RR::ComponentTypes::kComponentType_LocalTransform);

      std::shared_ptr<RR::WorldTransform> transform =
          std::static_pointer_cast<RR::WorldTransform>(ent.get()->GetComponent(
              RR::ComponentTypes::kComponentType_WorldTransform));

      std::shared_ptr<RR::RendererComponent> renderer_c =
          std::static_pointer_cast<RR::RendererComponent>(
              ent.get()->GetComponent(RR::ComponentTypes::kComponentType_Renderer));

      std::shared_ptr<RR::LocalTransform> transfrom_l =
          std::static_pointer_cast<RR::LocalTransform>(ent.get()->GetComponent(
              RR::ComponentTypes::kComponentType_LocalTransform));

      transform->world = mesh->world;
      
      transfrom_l->position = mesh->position;
      transfrom_l->rotation = mesh->rotation;
      transfrom_l->scale = mesh->scale;

      renderer_c->geometry = mesh->geometries[j];
      renderer_c->settings.pbr_settings = mesh->settings[j];
      renderer_c->textureSettings.pbr_textures = mesh->textures[j];

      renderer_c->Init(&renderer, RR::PipelineTypes::kPipelineType_PBR);

      data.lt.push_back(transfrom_l.get());
    }
  }

  meshes = nullptr;

  renderer.Start();

  return 0;
}