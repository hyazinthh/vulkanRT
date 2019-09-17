#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) rayPayloadInNV vec4 resultColor;
layout(location = 1) rayPayloadNV bool isShadowed;
hitAttributeNV vec2 hitAttribs;

struct Vertex {
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) uniform accelerationStructureNV scene;

layout(set = 0, binding = 3) buffer Vertices { Vertex v[]; }
vertices[];

layout(set = 0, binding = 4) buffer Indices { uint i[]; }
indices[];

layout(shaderRecordNV) buffer ShaderRecord {
	int objectID;
};

void main() {

    int id = objectID;

    ivec3 ind = ivec3(indices[id].i[3 * gl_PrimitiveID],
        indices[id].i[3 * gl_PrimitiveID + 1],
        indices[id].i[3 * gl_PrimitiveID + 2]);

    Vertex v0 = vertices[id].v[ind.x];
    Vertex v1 = vertices[id].v[ind.y];
    Vertex v2 = vertices[id].v[ind.z];    

    const vec3 barycentrics = vec3(1.0f - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);
    vec4 c = v0.color * barycentrics.x + v1.color * barycentrics.y + v2.color * barycentrics.z;

    // Shadow ray
    vec3 lightPosition = vec3(0, 0, 10);

    float tmin = 0.001;
    float tmax = 100.0;
    vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;

    isShadowed = true;

    traceNV(scene, gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV | gl_RayFlagsSkipClosestHitShaderNV, 
        0xFF, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */,
        1 /* missIndex */, origin, tmin, normalize(lightPosition - origin), tmax, 1 /*payload location*/);

    if (isShadowed)
        resultColor = c * 0.4;
    else
        resultColor = c; 
}