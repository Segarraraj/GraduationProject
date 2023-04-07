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

enum PipelineTypes : uint32_t {
  kPipelineType_None = 0U,
  kPipelineType_PBR = 1U,
  kPipelineType_Phong = 2U,
};

struct GeometryData {
  std::vector<float> vertex_data;
  std::vector<uint32_t> index_data;
};

struct PBRTextures {
  int32_t base_color;
  int32_t normal;
  int32_t metallic;
  int32_t roughness;
  int32_t reflectance;
};

struct PBRSettings {
 public:
  float metallic;
  float roughness;
  float reflectance;
  float padding;
  float base_color[4];

 private:
  bool base_color_texture;
  char padding1[3];
  bool normal_texture;
  char padding2[3];
  bool metallic_texture;
  char padding3[3];
  bool roughness_texture;
  char padding4[3];
  bool reflectance_texture;
  char padding5[3];

  friend class RendererComponent;
};

struct PhongSettings {
 public:
  float color[3];
};

union MaterialSettings {
  PBRSettings pbr_settings;
  PhongSettings phong_settings;
};

union TextureSettings {
  PBRTextures pbr_textures;
};

struct MeshData {
  std::vector<uint32_t> geometries;
  std::vector<PBRSettings> settings;
  std::vector<PBRTextures> textures;
  DirectX::XMFLOAT4X4 world;
};
}

#endif  // !__COMMON_H__
