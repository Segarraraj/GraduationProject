cbuffer cb : register(b0) {
  row_major float4x4 view : packoffset(c0);
  row_major float4x4 model : packoffset(c4);
  row_major float4x4 projection : packoffset(c8);
};

struct VertexInput {
  float3 inPos : POSITION;
  float3 inColor : COLOR;
};

struct VertexOutput {
  float3 color : COLOR;
  float4 position : SV_Position;
};

VertexOutput main(VertexInput vertexInput) {
  VertexOutput output;

  float4 position = mul(float4(vertexInput.inPos, 1.0f), mul(model, mul(view, projection)));

  output.position = position;
  output.color = vertexInput.inColor;

  return output;
}