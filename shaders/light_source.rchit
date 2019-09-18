#version 460
#extension GL_NV_ray_tracing : require

layout(location = 0) rayPayloadInNV vec4 resultColor;

void main() {
    resultColor = vec4(1.0f);
}