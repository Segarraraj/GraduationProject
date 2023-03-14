cbuffer CB : register(b0) { 
  float4x4 model;
  float4x4 view;
  float4x4 projection;
  float3 color;
};

struct VertexInput {
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 uv : UV;
};

struct VertexOutput {
  float4 position : SV_POSITION;
  float3 normal : NORMAL;
  float3 color : COLOR;
};

VertexOutput main(VertexInput input) {
  VertexOutput output;
  output.position = mul(float4(input.position, 1.0f), mul(model, mul(view, projection)));
  output.normal = input.normal;
  output.color = color;
  return output;
}