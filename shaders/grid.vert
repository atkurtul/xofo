#version 450


layout(location = 0) in vec3 in_pos;


layout(push_constant) uniform push_block {
  mat4 view;
  mat4 prj;
  mat4 xf;
} cam;


void main() {
  gl_Position = cam.prj * cam.view * cam.xf * vec4(in_pos,  1);
}