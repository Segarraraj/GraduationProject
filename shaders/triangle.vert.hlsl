cbuffer CB : register(b0) { 
  float4x4 model;
  float4x4 view;
  float4x4 projection;
};

struct VertexInput {
  float3 position : POSITION;
  float3 color : COLOR;
};

struct VertexOutput {
  float4 position : SV_POSITION;
  float3 color : COLOR;
};

VertexOutput main(VertexInput input) {
  VertexOutput output;
  output.position = mul(float4(input.position, 1.0f), mul(model, mul(view, projection)));
  output.color = input.color;
  return output;
}