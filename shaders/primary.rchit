#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) rayPayloadInNV vec4 resultColor;
layout(location = 1) rayPayloadNV bool isShadowed;

struct Vertex {
    vec4 position;
    vec3 normal;
};

layout(set = 0, binding = 0) uniform accelerationStructureNV scene;

layout(set = 0, binding = 3) buffer Vertices { 
    Vertex v[]; 
} vertices[];

layout(set = 0, binding = 4) buffer Indices {
    uint i[];
} indices[];

layout(shaderRecordNV) buffer ShaderRecord {
	int objectId;
    vec4 color;
    mat3 normalMatrix;
};

hitAttributeNV vec2 hitAttribs;

Vertex getHitPoint() {

    ivec3 ind = ivec3(indices[objectId].i[3 * gl_PrimitiveID],
        indices[objectId].i[3 * gl_PrimitiveID + 1],
        indices[objectId].i[3 * gl_PrimitiveID + 2]);

    Vertex v0 = vertices[objectId].v[ind.x];
    Vertex v1 = vertices[objectId].v[ind.y];
    Vertex v2 = vertices[objectId].v[ind.z];    

    const vec3 bc = vec3(1.0f - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);

    Vertex hitPoint;
    hitPoint.position = bc.x * v0.position + bc.y * v1.position + bc.z * v2.position;
    hitPoint.normal = bc.x * v0.normal + bc.y * v1.normal + bc.z * v2.normal;

    return hitPoint;
}

void main() {

    Vertex v = getHitPoint();

    // Lighting
    vec3 lightPosition = vec3(1, -3, 5);
    vec3 lightVector = normalize(lightPosition - v.position.xyz);

    // Normal
    vec3 normal = normalize(normalMatrix * v.normal);

    // Shadow ray
    float tmin = 0.001;
    float tmax = 1.0;
    vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;

    isShadowed = true;

    traceNV(scene, gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV | gl_RayFlagsSkipClosestHitShaderNV, 
        0xFF, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */,
        1 /* missIndex */, origin, tmin, lightPosition - origin, tmax, 1 /*payload location*/);

    // Diffuse lighting
    float diffuse = isShadowed ? 0.2 : max(dot(lightVector, normal), 0.2);
    resultColor = color * diffuse;
}