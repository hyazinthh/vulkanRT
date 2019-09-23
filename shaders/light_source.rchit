#version 460

#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "common/types.glsl"

layout(location = 0) rayPayloadInNV RayPayload payloadIn;

void main() {
    payloadIn.color = vec4(1.0f);
}