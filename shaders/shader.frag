#version 450

struct Light {
  vec4 pos;
  vec4 color;
};

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in mat3 norm_mat;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D albedo;
layout(set = 1, binding = 1) uniform sampler2D normal;
layout(set = 1, binding = 2) uniform sampler2D metalic;

layout(set = 0, binding = 0) uniform UBO00 {
  vec4 pos;
  int n_lights;
  Light lights[1024];
}
cam;

layout(push_constant) uniform push_block {
  mat4 view;
  mat4 prj;
  mat4 xf;
}
camx;

const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

vec3 srgb_to_linear(vec3 c) {
  return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}

void main1() {
  vec3 col = texture(albedo, tex).rgb;
  vec3 norm = texture(normal, tex).rgb * 2 - 1;
  norm = normalize(norm_mat * norm);
  out_color = vec4(norm, 1);
}

void main() {
  vec3 col = texture(albedo, tex).rgb;
  vec3 metal = texture(metalic, tex).rgb;
  vec3 norm = texture(normal, tex).rgb * 2 - 1;
  norm = normalize(norm_mat * norm);

  vec3 V = normalize(cam.pos.xyz - frag_pos);

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, col, metal);

  // reflectance equation
  vec3 Lo = vec3(0.0);
  for(int i = 0; i < 0; ++i)
  {
    Light light = cam.lights[i];
    vec3 L = normalize(light.pos.xyz - frag_pos);
    vec3 H = normalize(V + L);
    float distance = length(L);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = light.color.xyz * attenuation;

    // cook-torrance brdf
    float roughness = 0.2;
    float NDF = DistributionGGX(norm, H, roughness);
    float G = GeometrySmith(norm, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metal;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(norm, V), 0.0) * max(dot(norm, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    // add to outgoing radiance Lo
    float NdotL = max(dot(norm, L), 0.0);
    Lo += (kD * col / PI + specular) * radiance * NdotL;
  }

  vec3 ambient = vec3(0.4) * col;
  vec3 color = ambient + Lo;

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(0.455));

  out_color = vec4(color, 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float num = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float num = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}