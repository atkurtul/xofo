#version 450

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in mat3 norm_mat;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D albedo;
layout(set = 1, binding = 1) uniform sampler2D normal;
layout(set = 1, binding = 2) uniform sampler2D metalic;

layout(set = 0, binding = 0) uniform UBO00 {
  vec4 pos;
  vec4 light_pos;
  vec4 light_color;
}
cam;

layout(push_constant) uniform push_block {
  mat4 view;
  mat4 prj;
  mat4 xf;
}
camx;

// toon

void main1() {
  vec3 object_color = texture(albedo, tex).rgb;
  vec3 norm = texture(normal, tex).rgb * 2 - 1;
  norm = normalize(norm_mat * norm);

  vec3 light_dir = normalize(cam.light_pos.xyz - frag_pos);
  float intensity = dot(norm, light_dir);

  if (intensity > 0.98)
    object_color *= 1.5;
  else if (intensity > 0.9)
    object_color *= 1.0;
  else if (intensity > 0.5)
    object_color *= 0.6;
  else if (intensity > 0.25)
    object_color *= 0.4;
  else
    object_color *= 0.2;
  // Desaturate a bit
//  object_color = vec3(mix(object_color, vec3(dot(vec3(0.2126, 0.7152, 0.0722), object_color)), 0.1));
  out_color = vec4(object_color * 2, 1.0);
}

void main0() {
  vec3 object_color = texture(albedo, tex).rgb;
  vec3 norm = texture(normal, tex).rgb * 2 - 1;
  norm = normalize(norm_mat * norm);

  vec3 light_dir = normalize(cam.light_pos.xyz - frag_pos);

  vec3 light_color = cam.light_color.xyz;

  vec3 diffuse = max(dot(norm_mat[2].xyz, light_dir), 0.0) * light_color;

  vec3 ambient = vec3(0.45);
  vec3 final_color = (ambient + diffuse) * object_color;
  out_color = vec4(final_color, 1.0);
}

const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

vec3 srgb_to_linear(vec3 c) {
  return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
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

  vec3 L = normalize(cam.light_pos.xyz - frag_pos);
  vec3 H = normalize(V + L);
  float distance = length(L);
  float attenuation = 1.0 / (distance * distance);
  vec3 radiance = cam.light_color.xyz * attenuation;

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
  vec3 Lo = (kD * col / PI + specular) * radiance * NdotL;

  vec3 ambient = vec3(0.01) * col;
  vec3 color = ambient + Lo;

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

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