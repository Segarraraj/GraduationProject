#ifndef __COMMON_H__
#define __COMMON_H__ 1

#include <DirectXMath.h>

#include <vector>

namespace RR {
enum ComponentTypes : uint32_t {
  kComponentType_None           = 0b0000,
  kComponentType_WorldTransform = 0b0001,
  kComponentType_LocalTransform = 0b0010,
  kComponentType_Renderer       = 0b0100,
  kComponentType_Camera         = 0b1000
};

enum GeometryTypes : uint32_t { 
  kGeometryType_None                          = 0U,
  kGeometryType_Positions_Normals             = 1U,
  kGeometryType_Positions_Normals_UV          = 2U,
  kGeometryType_Positions_Normals_Tangents_UV = 3U,
};

struct GeometryData {
  std::vector<float> vertex_data;
  std::vector<uint32_t> index_data;
};

enum PipelineTypes : uint32_t {
  kPipelineType_None   = 0U,
  kPipelineType_PBR    = 1U,
  kPipelineType_Phong  = 2U,
};
}

#endif  // !__COMMON_H__
