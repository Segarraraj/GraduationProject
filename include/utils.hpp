#ifndef __PROJECT_UTILS__
#define __PROJECT_UTILS__ 1

#include <vector>
#include <fstream>

#include <renderer/logger.h>

namespace Utils {
std::vector<char> ReadFile(const char* file_name) {
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
}

#endif  // !__PROJECT_UTILS__
