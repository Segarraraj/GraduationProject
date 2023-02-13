#include "renderer/renderer.h"

#include <stdio.h>

static void update() { 
  printf("Update\n");
}

int main(int argc, char** argv) {
  RR::Renderer renderer;

  renderer.Init(update);
  renderer.Start();

  return 0;
}