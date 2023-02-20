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

const void* RR::Window::window() const { return _window; }

float RR::Window::width() const {
  RECT bounds;
  GetClientRect((HWND)_window, &bounds);
  return ;
}

float RR::Window::height() const {
  RECT bounds;
  GetClientRect((HWND)_window, &bounds);
  return bounds.bottom - bounds.top;
}

float RR::Window::aspectRatio() const {
  RECT bounds;
  GetClientRect((HWND)_window, &bounds);
  return (bounds.right - bounds.left) / (bounds.bottom - bounds.top);
}