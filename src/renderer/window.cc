#include "renderer/window.h"

#include <Windows.h>

RR::Window::Window() {}

RR::Window::~Window() {}

void RR::Window::Init(void* instance, const char* class_name,
                      const char* window_header,
                      long long (*window_proc)(void*, unsigned int, unsigned long long, long long),
                      void* user_data) { 

  WNDCLASS window_class = {};

  window_class.hInstance = (HINSTANCE)instance;
  window_class.lpfnWndProc = (WNDPROC)window_proc;
  window_class.lpszClassName = class_name;

  RegisterClass(&window_class);

  HWND window = CreateWindowEx(0, class_name, window_header, WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, NULL, NULL, (HINSTANCE)instance, NULL);

  SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)user_data);
  
  _window = window;
}

void RR::Window::Show() const { ShowWindow((HWND)_window, 1); }

void* RR::Window::window() const { return _window; }

uint32_t RR::Window::width() const {
  RECT bounds;
  GetClientRect((HWND)_window, &bounds);
  return bounds.right - bounds.left;
}

uint32_t RR::Window::height() const {
  RECT bounds;
  GetClientRect((HWND)_window, &bounds);
  return bounds.bottom - bounds.top;
}

uint32_t RR::Window::screenCenterX() const {
  RECT bounds;
  GetWindowRect((HWND)_window, &bounds);
  return (bounds.right - bounds.left) / 2 + bounds.left;
}

uint32_t RR::Window::screenCenterY() const { 
  RECT bounds;
  GetWindowRect((HWND)_window, &bounds);
  return (bounds.bottom - bounds.top) / 2 + bounds.top;
}

float RR::Window::aspectRatio() const {
  RECT bounds;
  GetClientRect((HWND)_window, &bounds);
  if ((bounds.bottom - bounds.top) != 0) {
    return (bounds.right - bounds.left) / (bounds.bottom - bounds.top);
  } else {
    return 1.0f;
  }
}

bool RR::Window::isCaptureMouse() const { return _capture_mouse; }

void RR::Window::CaptureMouse() { 
  _capture_mouse = !_capture_mouse;

  if (_capture_mouse) {
    SetCapture((HWND)_window);
  } else {
    ReleaseCapture();
  }
}
