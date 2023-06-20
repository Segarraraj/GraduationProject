Texture2D textures[5] : register(t0);
SamplerState s1 : register(s0);

static const float PI = 3.14159265359f;

cbuffer MVP : register(b0) {
  float4x4 model;
  float4x4 view;
  float4x4 projection;
};

cbuffer MaterialParameters : register(b1) {
  float metallic;
  float perceptualRoughness;
  float reflectance;
  float padding;
  float4 baseColor;

  bool baseColorTexture;
  bool normalTexture;
  bool metallicTexture;
  bool roughnessTexture;
  bool reflectanceTexture;
};

cbuffer Constants : register(b2) {
  float elapsedTime;
  float ambientIntensity;
  float3 cameraPos;
  float3 lightPos;
};

struct VertexOutput {
  float4 position : SV_POSITION;
  float4 worldPos : POSITION;
  float3 normal : NORMAL;
  float3 worldNormal : WORLD_NORMAL;
  float2 uv : UV;
  float3x3 tbn : TBN;
};

// GGX Normal distribution function (NDF)
float D_GGX(float NoH, float roughness) {
  float a = NoH * roughness;
  float k = roughness / (1.0f - NoH * NoH + a * a);
  return k * k * (1.0f / PI);
}

// Geometric shadowing
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
  float GGXV = NoL * sqrt(NoV * NoV * (1.0f - roughness) + roughness);
  float GGXL = NoV * sqrt(NoL * NoL * (1.0f - roughness) + roughness);
  return 0.5f / (GGXV + GGXL);
}

// Fresnel
float3 F_Schlick(float u, float3 f0) {
  return f0 + (float3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - u, 5.0f);
}

// Float fresnel
float F_Schlick(float u, float f0, float f90) {
  return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
}

// Disney burley BDRF diffuse
float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
  float f90 = 0.5f + 2.0f * roughness * LoH * LoH;
  float lightScatter = F_Schlick(NoL, 1.0f, f90);
  float viewScatter = F_Schlick(NoV, 1.0f, f90);
  return lightScatter * viewScatter * (1.0f / PI);
}

// Simplified BDRF diffuse
float Fd_Lambert() { return 1.0 / PI; }

float3 BRDF(float3 V, float3 L, float3 N, float3 diffuseColor, float3 F0, float roughness) {
  float3 H = normalize(V + L);

  float NoV = abs(dot(N, V)) + 1e-5;
  float NoL = clamp(dot(N, L), 0.0f, 1.0f);
  float NoH = clamp(dot(N, H), 0.0f, 1.0f);
  float LoH = clamp(dot(L, H), 0.0f, 1.0f);

  // PBR
  float D = D_GGX(NoH, roughness);
  float3 F = F_Schlick(LoH, F0);
  float G = V_SmithGGXCorrelated(NoV, NoL, roughness);

  // specular BRDF
  float3 Fr = (D * G) * F;

  float3 Fd = diffuseColor * Fd_Lambert();

  return max((Fd + Fr) * NoL, float3(0.0f, 0.0f, 0.0f));
}

float4 main(VertexOutput input) : SV_TARGET {
  // Material parameters remap
  float4 realBaseColor = baseColor;
  if (baseColorTexture) {
    realBaseColor = textures[0].Sample(s1, input.uv);
    float3 correct = pow(realBaseColor.rgb, 2.2f);
    realBaseColor = float4(correct.rgb, realBaseColor.a);
  }

  float realMetallic = metallic;
  if (metallicTexture) {
    realMetallic = pow(textures[1].Sample(s1, input.uv), 2.2f).r;
  }

  float realperceptualRoughness = perceptualRoughness;
  if (roughnessTexture) {
    realperceptualRoughness = pow(textures[3].Sample(s1, input.uv), 2.2f).r;
  }

  float realReflectance = reflectance;
  if (reflectanceTexture) {
    realReflectance = pow(textures[4].Sample(s1, input.uv), 2.2f).r;
  }

  float3 N;
  if (normalTexture) {
    N = textures[2].Sample(s1, input.uv).xyz * 2.0f - 1.0f;
    N = mul(input.tbn, N);
    N = mul(transpose(model), float4(N, 1.0f)).xyz;
  } else {
    N = input.worldNormal;
  }

  float3 diffuseColor = (1.0f - realMetallic) * realBaseColor.rgb;
  float3 f0 =
      0.16f * realReflectance * realReflectance * (1.0f - realMetallic) +
      realBaseColor.rgb * realMetallic;
  float roughness = realperceptualRoughness * realperceptualRoughness;

  // Calculate PBR components
  N = normalize(N);
  float3 V = normalize(cameraPos - input.worldPos.xyz);
  // Light direction
  float3 L = normalize(-1.0f * lightPos);

  // TODO: Add AO texture, fixed ambient intensity/color
  float3 ambient = ambientIntensity * diffuseColor;

  float3 light = ambient + BRDF(V, L, N, diffuseColor, f0, roughness);

  // Linear to srgb
  light = pow(light, 1.0 / 2.2);

  return float4(light, realBaseColor.a);
}