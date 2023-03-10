#ifndef __INPUT_H__
#define __INPUT_H__ 1

namespace RR {
class Input{
 public:
  Input() = default;
  ~Input() = default;

  void Flush(float width, float heigth, int mouse_x, int mouse_y);
  void SetMouse(int mouse_x, int mouse_y);

  int MouseX() const;
  int MouseY() const;
  float MouseXAxis() const;
  float MouseYAxis() const;

  char keys[128] = {0};

 private:
  float _width = 0.0f;
  float _heigth = 0.0f;
  float _delta_mouse_x = 0;
  float _delta_mouse_y = 0;
  int _mouse_x = 0;
  int _mouse_y = 0;
};
}

#endif  // !__INPUT_H__
