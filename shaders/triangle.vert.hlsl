cbuffer CB : register(b0) { 
  float3 color_multiplier; 
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
  output.position = float4(input.position, 1.0f);
  output.color = input.color * color_multiplier;
  return output;
}