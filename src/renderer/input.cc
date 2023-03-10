#include "renderer/input.h"

#include <string.h>

void RR::Input::Flush(float width, float heigth, int mouse_x, int mouse_y) { 
  _width = width;
  _heigth = heigth;

  if (mouse_x != -1) {
    _mouse_x = mouse_x;
  }

  if (mouse_y != -1) {
    _mouse_y = mouse_y;
  }

  _delta_mouse_x = 0;
  _delta_mouse_y = 0;
}

void RR::Input::SetMouse(int mouse_x, int mouse_y) {
  _delta_mouse_x = (mouse_x - _mouse_x) / (float) _width;
  _delta_mouse_y = (mouse_y - _mouse_y) / (float) _heigth;

  _mouse_x = mouse_x;
  _mouse_y = mouse_y;
}

int RR::Input::MouseX() const { return _mouse_x; }
int RR::Input::MouseY() const { return _mouse_y; }
float RR::Input::MouseXAxis() const { return _delta_mouse_x; }
float RR::Input::MouseYAxis() const { return _delta_mouse_y; }
