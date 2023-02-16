float4 main(float3 inPos : POSITION) : SV_POSITION {
  return float4(inPos, 1.0f);
}