#version 460

#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "common/bindings.glsl"
#include "common/lighting.glsl"

hitAttributeNV vec2 hitAttribs;

Vertex getHitPoint(Instance inst) {

    uint objId = inst.objectId;
    Face f = indexBuffer[objId].faces[gl_PrimitiveID];

    Vertex v[3];
    v[0] = vertexBuffer[objId].vertices[f.indices[0]];
    v[1] = vertexBuffer[objId].vertices[f.indices[1]];
    v[2] = vertexBuffer[objId].vertices[f.indices[2]];

    const vec3 bc = vec3(1.0f - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);

    Vertex hitPoint;
    hitPoint.position = bc.x * v[0].position + bc.y * v[1].position + bc.z * v[2].position;
    hitPoint.normal = normalize(bc.x * v[0].normal + bc.y * v[1].normal + bc.z * v[2].normal);
    hitPoint.tangent = normalize(bc.x * v[0].tangent + bc.y * v[1].tangent + bc.z * v[2].tangent);
    hitPoint.tc = bc.x * v[0].tc + bc.y * v[1].tc + bc.z * v[2].tc;

    return hitPoint;
}

void main() {

    Instance instance = instances[gl_InstanceCustomIndexNV].instance;
    Material material = materials[instance.materialId].material;
    Vertex vertex = getHitPoint(instance);

    payloadIn.color = lighting(instance, material, vertex);
}