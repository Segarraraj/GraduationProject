#include "renderer/renderer.h"

static void update() { 

}

int main(int argc, char** argv) {
  RR::Renderer renderer;

  int result = renderer.Init(update);
  if (result != 0) {
    return 1;
  }

  renderer.Start();

  return 0;
}