Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

struct VertexOutput {
  float4 position : SV_POSITION;
  float2 uv : UV;
};

float4 main(VertexOutput input) : SV_TARGET {
  return t1.Sample(s1, input.uv);
}