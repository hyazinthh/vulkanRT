#version 460

#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "common/types.glsl"

layout(location = 0) rayPayloadInNV RayPayload payloadIn;

void main() {
    payloadIn.color = vec4(0.412f, 0.796f, 1.0f, 1.0f);
}