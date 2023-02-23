#ifndef __WINDOW_H__
#define __WINDOW_H__ 1

#include <cstdint>

namespace RR {
class Window {
 public:
  Window();

  Window(const Window&) = delete;
  Window(Window&&) = delete;

  void operator=(const Window&) = delete;
  void operator=(Window&&) = delete;

  ~Window();

  void Init(void* instance, const char* class_name, const char* window_header, 
            long long (*window_proc)(void*, unsigned int, unsigned long long, long long),
            void* user_data);
  void Show() const;

  const void* window() const;
  uint32_t width() const;
  uint32_t height() const;
  float aspectRatio() const;
 private:
  void* _window = nullptr;
};
}  // names   pace RR

#endif  // !__WINDOW_H__


