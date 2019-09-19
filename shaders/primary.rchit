#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

struct RayPayload {
    vec4 color;
    int bounce;
};

layout(location = 0) rayPayloadInNV RayPayload payloadIn;
layout(location = 1) rayPayloadNV bool isShadowed;
layout(location = 2) rayPayloadNV RayPayload payloadOut;

struct Vertex {
    vec4 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 tc;
};

layout(set = 0, binding = 0) uniform accelerationStructureNV scene;

layout(set = 0, binding = 2) uniform RayTracingSettings {
    uint maxBounces;
    float tmax;
} rayTracingSettings;

layout(set = 0, binding = 4) buffer Vertices { 
    Vertex v[]; 
} vertices[];

layout(set = 0, binding = 5) buffer Indices {
    uint i[];
} indices[];

layout(set = 0, binding = 6) uniform Light {
    vec4 position;
    vec4 diffuseColor;
} light;

layout(set = 0, binding = 7) uniform sampler2D[] textureSamplers;

layout(shaderRecordNV) buffer ShaderRecord {
	int objectId;
    int textureId[3];
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
    hitPoint.normal = normalize(bc.x * v0.normal + bc.y * v1.normal + bc.z * v2.normal);
    hitPoint.tangent = normalize(bc.x * v0.tangent + bc.y * v1.tangent + bc.z * v2.tangent);
    hitPoint.bitangent = normalize(bc.x * v0.bitangent + bc.y * v1.bitangent + bc.z * v2.bitangent);
    hitPoint.tc = bc.x * v0.tc + bc.y * v1.tc + bc.z * v2.tc;

    return hitPoint;
}

void main() {

    Vertex v = getHitPoint();

    // Lighting
    vec3 lightVector = normalize(light.position.xyz - v.position.xyz);

    // Normal
    vec3 N = normalize(normalMatrix * v.normal);
    vec3 T = normalize(normalMatrix * v.tangent);
    vec3 B = normalize(normalMatrix * v.bitangent);

    if (textureId[1] > -1) {
        vec3 n = texture(textureSamplers[textureId[1]], v.tc.xy).xyz * 2.0f - 1.0f;
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
    vec4 color = (textureId[0] > -1) ? texture(textureSamplers[textureId[0]], v.tc.xy) : color;

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