struct VertexOutput {
  float4 position : SV_POSITION;
  float3 normal : NORMAL;
  float3 color : COLOR;
};

float4 main(VertexOutput input) : SV_TARGET {
  float3 directionalLight = normalize(float3(0.5f, -0.5f, 1.0f));
  float3 light_color = float3(254.0f / 255.0f, 251.0f / 255.0f, 232.0f / 255.0f);
  
  float3 ambient = 0.1f * light_color;
  float diffuse = max(dot(input.normal, directionalLight), 0.0);

  float3 light = (ambient + diffuse * light_color) * input.color;

  return float4(light, 1.0f);
}