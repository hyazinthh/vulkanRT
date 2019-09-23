#version 460

#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "common/bindings.glsl"

layout(location = 0) rayPayloadInNV RayPayload payloadIn;
layout(location = 1) rayPayloadNV bool isShadowed;
layout(location = 2) rayPayloadNV RayPayload payloadOut;

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

    Instance inst = instances[gl_InstanceCustomIndexNV].instance;
    Material mat = materials[inst.materialId].material;
    Vertex v = getHitPoint(inst);

    // Lighting
    vec3 lightVector = normalize(light.position.xyz - v.position.xyz);

    // Normal
    vec3 N = normalize(inst.normalMatrix * v.normal);
    vec3 T = normalize(inst.normalMatrix * v.tangent);
    vec3 B = cross(N, T);

    if (mat.textureId[1] > -1) {
        vec3 n = texture(textures[mat.textureId[1]], v.tc.xy).xyz * 2.0f - 1.0f;
        n = normalize(vec3(n.x, n.y, n.z * 5.0f));
        N = normalize(mat3(T, B, N) * n);
    }

    // Shadow ray
    float tmin = 0.001;
    float tmax = 1.0;
    vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;

    isShadowed = true;

    traceNV(scene, gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV | gl_RayFlagsSkipClosestHitShaderNV, 
        0xFE, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */,
        1 /* missIndex */, origin, tmin, light.position.xyz - origin, tmax, 1 /*payload location*/);

    // Diffuse color
    vec4 color = (mat.textureId[0] > -1) ? texture(textures[mat.textureId[0]], v.tc.xy) : mat.color;

    if (color == vec4(1.0f) && payloadIn.bounce < rayTracingSettings.maxBounces) {
        payloadOut.bounce = payloadIn.bounce + 1;

        traceNV(scene, gl_RayFlagsOpaqueNV, 
            0xFF, 
            0 /* sbtRecordOffset */, 
            0 /* sbtRecordStride */,
            0 /* missIndex */, 
            origin, tmin, reflect(gl_WorldRayDirectionNV, N), rayTracingSettings.tmax, 
            2 /*payload location*/);

        color = payloadOut.color;
    }

    // Diffuse lighting
    float diffuse = isShadowed ? 0.2 : max(dot(lightVector, N), 0.2);
    payloadIn.color = color * diffuse;
}