#include "renderer/renderer.h"

static void update() {

}

int main(int argc, char** argv) {
  RR::Renderer renderer;

  renderer.Init(update);
  renderer.Start();

  return 0;
}