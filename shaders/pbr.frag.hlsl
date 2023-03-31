Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

cbuffer MaterialParameters : register(b1) {
  float metalic;
  float roughness;
  float reflectance;
  float padding;
  float3 baseColor;
};

cbuffer Constants : register(b2) {
  float time;
  float3 cameraPos;
};

struct VertexOutput {
  float4 position : SV_POSITION;
  float2 uv : UV;
};

float4 main(VertexOutput input) : SV_TARGET {
  float3 color = baseColor;
  color.z = sin(time * 0.001f);
  return float4(color, 1.0f);
}