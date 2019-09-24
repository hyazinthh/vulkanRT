#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "types.glsl"
#include "bindings.glsl"

layout(location = 0) rayPayloadInNV RayPayload payloadIn;
layout(location = 1) rayPayloadNV RayPayload payloadOut;
layout(location = 2) rayPayloadNV bool isShadowed;

vec4 lighting(Instance instance, Material material, Vertex vertex) {

    vec3 L = normalize(light.position.xyz - vertex.position.xyz);

    vec3 N = normalize(instance.normalMatrix * vertex.normal);
    vec3 T = normalize(instance.normalMatrix * vertex.tangent);
    vec3 B = cross(N, T);

    // Normal map
    if (material.textureId[1] > -1) {
        vec3 n = texture(textures[material.textureId[1]], vertex.tc).xyz * 2.0f - 1.0f;
        n = normalize(vec3(n.x, n.y, n.z * 5.0f));
        N = normalize(mat3(T, B, N) * n);
    }

    // Shadow ray
    vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
    const uint rayFlags = gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV | gl_RayFlagsSkipClosestHitShaderNV;

    isShadowed = true;
    traceNV(scene, rayFlags, 0xFE, 0, 0, 1, origin, rayTracingSettings.tmin, light.position.xyz - origin, 1.0f, 2);

    // Diffuse color
    vec4 color = (material.textureId[0] > -1) ? texture(textures[material.textureId[0]], vertex.tc) : material.color;

    // Reflection if diffuse color is white
    if (color == vec4(1.0f) && payloadIn.bounce < rayTracingSettings.maxBounces) {
        payloadOut.bounce = payloadIn.bounce + 1;

        traceNV(scene, gl_RayFlagsOpaqueNV, 0xFF, 0, 0, 0,
            origin, rayTracingSettings.tmin, reflect(gl_WorldRayDirectionNV, N), rayTracingSettings.tmax, 1);

        color = payloadOut.color;
    }

    // Diffuse lighting
    float diffuse = isShadowed ? 0.2 : max(dot(L, N), 0.2);
    return color * diffuse;
}

#endif