#include "utils.h"

#include <fstream>

#include <renderer/logger.h>

std::vector<char> Utils::ReadFile(const char* file_name) {
  std::ifstream file =
      std::ifstream(file_name, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    LOG_WARNING("UTILS", "Couldn't read file: %s", file_name);
    return std::vector<char>();
  }

  std::vector<char> buffer = std::vector<char>((size_t)file.tellg());

  file.seekg(0);
  file.read(buffer.data(), buffer.size());

  file.close();

  return buffer;
}