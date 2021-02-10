#version 450

layout(location = 0) in vec4 col;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 norm;
layout(location = 0) out vec4 color;

layout(set = 1, binding = 0) uniform sampler2D albedo;
layout(set = 1, binding = 1) uniform sampler2D normal;
layout(set = 1, binding = 2) uniform sampler2D metalic;

void main() {

    color =  texture(albedo, tex);
    // color = col;
}