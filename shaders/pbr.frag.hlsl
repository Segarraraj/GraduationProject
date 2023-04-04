Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

static const float PI = 3.14159265359f;

cbuffer MaterialParameters : register(b1) {
  float metallic;
  float perceptualRoughness;
  float reflectance;
  float padding;
  float3 baseColor;
};

cbuffer Constants : register(b2) {
  float time;
  float3 cameraPos;
};

struct VertexOutput {
  float4 position : SV_POSITION;
  float4 worldPos : POSITION;
  float3 normal : NORMAL;
  float2 uv : UV;
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

float4 main(VertexOutput input) : SV_TARGET {
  // Calculate PBR components
  float3 N = normalize(input.normal);
  float3 V = normalize(cameraPos - input.worldPos.xyz);
  float3 L = normalize(float3(0.0f, 1.0f, -1.0f));
  float3 H = normalize(V + L);

  float NoV = abs(dot(N, V)) + 1e-5;
  float NoL = clamp(dot(N, L), 0.0f, 1.0f);
  float NoH = clamp(dot(N, H), 0.0f, 1.0f);
  float LoH = clamp(dot(L, H), 0.0f, 1.0f);

  // Material parameters remap
  float3 diffuseColor = (1.0f - metallic) * baseColor.rgb;
  float3 f0 = 0.16f * reflectance * reflectance * (1.0f - metallic) +
             baseColor * metallic;
  float roughness = perceptualRoughness * perceptualRoughness;

  // PBR
  float D = D_GGX(NoH, roughness);
  float3 F = F_Schlick(LoH, f0);
  float G = V_SmithGGXCorrelated(NoV, NoL, roughness);

  // specular BRDF
  float3 Fr = (D * G) * F;
  
  // diffuse BRDF
  float3 Fd = diffuseColor * Fd_Lambert();

  return float4((Fd + Fr) * float3(1.0f, 1.0f, 1.0f) * NoL, 1.0f);
  //return float4((input.normal + 1.0f) * 0.5f, 1.0f);
}