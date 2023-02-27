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

enum GeometryTypes : uint32_t { 
  kGeometryType_None                 = 0U,
  kGeometryType_Positions_Normals    = 1U,
  kGeometryType_Positions_Normals_UV = 2U,
};

struct GeometryData {
  float* vertex_data;
  uint32_t* index_data;
  uint32_t vertex_size;
  uint32_t index_size;
};

enum PipelineTypes : uint32_t {
  kPipelineType_None = 0U,
  kPipelineType_PBR  = 1U,
};
}

#endif  // !__COMMON_H__
