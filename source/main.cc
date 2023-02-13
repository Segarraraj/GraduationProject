#include "renderer/renderer.h"

#include "renderer/logger.h"

static void update() { 
  LOG_DEBUG("MAIN", "Update called");
  LOG_WARNING("MAIN", "Update called");
  LOG_ERROR("MAIN", "Update called");
}

int main(int argc, char** argv) {
  RR::Renderer renderer;

  renderer.Init(update);
  renderer.Start();

  return 0;
}