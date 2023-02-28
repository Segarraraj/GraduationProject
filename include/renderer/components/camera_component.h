#ifndef __CAMERA_COMPONENT_H__
#define __CAMERA_COMPONENT_H__ 1

#include <memory>

#include "renderer/common.hpp"
#include "renderer/components/entity_component.h"

namespace RR {
class Entity;
class EntityComponent;

class Camera : public EntityComponent {
 public:
  Camera();
  ~Camera() = default;

  float fov;
  float nearZ;
  float farZ;
  float clear_color[4];

  friend class Renderer;
};
}  // namespace RR

#endif  // !__CAMERA_COMPONENT_H__