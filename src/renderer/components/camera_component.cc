#include "renderer/components/camera_component.h"

RR::Camera::Camera() {
  fov = 45.0f;
  nearZ = 0.01f;
  farZ = 10.0f;
  clear_color[0] = 0.0f;
  clear_color[1] = 0.2f;
  clear_color[2] = 0.4f;
  clear_color[3] = 1.0f;
}