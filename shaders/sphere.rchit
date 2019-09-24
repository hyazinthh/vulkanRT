#version 460

#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "common/bindings.glsl"
#include "common/lighting.glsl"

hitAttributeNV Vertex hitAttribs;

void main() {

    Instance instance = instances[gl_InstanceCustomIndexNV].instance;
    Material material = materials[instance.materialId].material;

    payloadIn.color = lighting(instance, material, hitAttribs);
}