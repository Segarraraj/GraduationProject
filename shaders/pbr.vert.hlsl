cbuffer MVP : register(b0) { 
  float4x4 model;
  float4x4 view;
  float4x4 projection;
};

struct VertexInput {
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 uv : UV;
};

struct VertexOutput {
  float4 position : SV_POSITION;
  float2 uv : UV;
};

VertexOutput main(VertexInput input) {
  VertexOutput output;
  output.position = mul(float4(input.position, 1.0f), mul(model, mul(view, projection)));
  output.uv = input.uv;
  return output;
}