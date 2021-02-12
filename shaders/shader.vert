#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_tex;

layout(location = 2) in vec3 in_norm0;
layout(location = 3) in vec3 in_norm1;
layout(location = 4) in vec3 in_norm2;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec2 out_tex;
layout(location = 2) out mat3 norm_mat;


layout(push_constant) uniform push_block {
  mat4 view;
  mat4 prj;
  mat4 xf;
} cam;

void main() {
  vec4 pos = cam.xf * vec4(in_pos, 1);
  gl_Position = cam.prj * cam.view * pos;
  vec3 T = normalize(cam.xf * vec4(in_norm1, 0.0)).xyz;
  vec3 B = normalize(cam.xf * vec4(in_norm2, 0.0)).xyz;
  vec3 N = normalize(cam.xf * vec4(in_norm0, 0.0)).xyz;
  norm_mat = mat3(T, B, N);
  out_tex = in_tex;
  frag_pos = (cam.xf * pos).xyz;
}