#include "renderer/renderer.h"

#include "renderer/entity.h"
#include "renderer/components/entity_component.h"
#include "renderer/components/local_transform_component.h"
#include "renderer/common.hpp"

static struct UserData {
  RR::Renderer* renderer;
  std::shared_ptr<RR::Entity> cube1;
  std::shared_ptr<RR::Entity> cube2;
};

static void update(void* user_data) { 
  UserData* data;
}

int main(int argc, char** argv) {
  RR::Renderer renderer;
  UserData data = {};

  int result = renderer.Init((void*) &data, update);
  if (result != 0) {
    return 1;
  }

  data.renderer = &renderer;
  data.cube1 = renderer.RegisterEntity(0);
  data.cube2 = renderer.RegisterEntity(0);

  std::shared_ptr<RR::LocalTransform> transform =
      std::static_pointer_cast<RR::LocalTransform>(
          data.cube2->GetComponent(RR::kComponentType_LocalTransform));

  transform->scale = {.25f, .25f, .25f};
  transform->position = {5.f, .0f, .0f};
  transform->SetParent(data.cube1);

  transform = std::static_pointer_cast<RR::LocalTransform>(
      data.cube1->GetComponent(RR::kComponentType_LocalTransform));

  transform->scale = {.5f, .5f, .5f};

  transform = std::static_pointer_cast<RR::LocalTransform>(
      renderer.MainCamera()->GetComponent(RR::kComponentType_LocalTransform));;
  transform->position = {.0f, 1.5f, -6.0f};
  transform->rotation = {15.0f, 180.0f, .0f};

  renderer.Start();

  return 0;
}