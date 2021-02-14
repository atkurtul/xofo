#version 450


layout(location = 0) in vec2 in_pos;


layout(push_constant) uniform push_block {
  mat4 view;
  mat4 prj;
} cam;


void main() {
  gl_Position = cam.prj * cam.view * vec4(in_pos.x, 0, in_pos.y,  1);
}