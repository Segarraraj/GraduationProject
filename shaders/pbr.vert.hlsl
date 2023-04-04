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
  float4 worldPos : POSITION;
  float3 normal : NORMAL;
  float2 uv : UV;
};

VertexOutput main(VertexInput input) {
  VertexOutput output;
  output.position = mul(float4(input.position, 1.0f), mul(model, mul(view, projection)));
  output.worldPos = mul(model, float4(input.position, 1.0f));
  output.normal = normalize(mul(transpose(model), float4(input.normal, 1.0f)).xyz);
  output.uv = input.uv;
  return output;
}