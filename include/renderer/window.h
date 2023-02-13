#ifndef __PROJECT_RENDERER_WINDOW__
#define __PROJECT_RENDERER_WINDOW__ 1

//typedef void* HINSTANCE;
//typedef void* HWND;
//typedef LONG_PTR LRESULT;
//typedef unsigned int UINT;
//typedef unsigned int* WPARAM;
//typedef int* LPARAM;

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
 private:
  void* window_ = nullptr;
};
}  // namespace RR

#endif  // !__PROJECT_RENDERER_WINDOW__


