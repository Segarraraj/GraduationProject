#ifndef __COMMON_H__
#define __COMMON_H__ 1

#include <DirectXMath.h>

namespace RR {
struct ConstantBufferStruct {
  DirectX::XMFLOAT4X4 model;
  DirectX::XMFLOAT4X4 view;
  DirectX::XMFLOAT4X4 projection;
};

enum ComponentTypes : uint32_t {
  kComponentType_None =           0b0000,
  kComponentType_WorldTransform = 0b0001,
  kComponentType_LocalTransform = 0b0010,
  kComponentType_Renderer =       0b0100,
  kComponentType_Camera =         0b1000
};
}

#endif  // !__COMMON_H__
