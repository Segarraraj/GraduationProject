#ifndef __PROJECT_RENDERER_RENDERER__
#define __PROJECT_RENDERER_RENDERER__

#include <memory>

namespace RR {
class Window;
class Renderer {
 public:
  Renderer();

  Renderer(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;

  void operator=(const Renderer&) = delete;
  void operator=(Renderer&&) = delete;

  ~Renderer();

  void Init(void (*update)());
  void Start() const;
  void Stop();

 private:
  std::unique_ptr<RR::Window> _window;
  void (*update_)() = nullptr;

  bool _running = true;
};
}

#endif  // !__PROJECT_RENDERER_RENDERER__
