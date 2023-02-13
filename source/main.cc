#include "renderer/window.h"

#include <Windows.h>

long long CALLBACK WindowProc(void* window, unsigned int message, unsigned long long wParam,
                              long long lParam) {
  LRESULT result = 0;
  switch (message) {
    default: {
      result = DefWindowProc((HWND) window, message, wParam, lParam);
      break;
    }
  }
  return result;
}

int main(int argc, char** argv) {
  RR::Window wnd;

  wnd.Init(GetModuleHandle(NULL), "Main Class", "DX12 Graduation Project", WindowProc);
  wnd.Show();
  
  while (1) {

  }

  return 0;
}