struct PixelInput {
  float3 color : COLOR;
};

struct PixelOutput {
  float4 attachment0 : SV_Target0;
};

PixelOutput main(PixelInput pixelInput) {
  PixelOutput output;
  output.attachment0 = float4(pixelInput.color, 1.0f);
  return output;
}