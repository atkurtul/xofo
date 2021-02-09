#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 norm;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 outtex;
layout(location = 2) out vec3 outnorm;

layout(binding=0) uniform UBO00 {
    mat4 vp;
} cam;

void main() {
    gl_Position = cam.vp * vec4(pos, 1);
    color = vec4(norm, 1);
    outtex = tex;
    outnorm = norm;
}

