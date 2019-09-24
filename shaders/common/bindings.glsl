#ifndef BINDINGS_GLSL_
#define BINDINGS_GLSL_

#extension GL_EXT_nonuniform_qualifier : require

#include "constants.glsl"
#include "types.glsl"

layout(set = 0, binding = BINDING_SCENE) uniform accelerationStructureNV scene;

layout(set = 0, binding = BINDING_OUTPUT, rgba8) uniform image2D resultImage;

layout(set = 0, binding = BINDING_SETTINGS) uniform RayTracingSettingsBuffer {
    RayTracingSettings rayTracingSettings;
};

layout(set = 0, binding = BINDING_CAMERA) uniform CameraBuffer {
    Camera camera;
};

layout(set = 0, binding = BINDING_VERTEX_BUFFERS, std430) readonly buffer VertexBuffer { 
    Vertex vertices[]; 
} vertexBuffer[];

layout(set = 0, binding = BINDING_INDEX_BUFFERS, std430) readonly buffer IndexBuffer {
    Face faces[];
} indexBuffer[];

layout(set = 0, binding = BINDING_SPHERE_BUFFERS, std430) readonly buffer SphereBuffer {
    Sphere sphere;
} spheres[];

layout(set = 0, binding = BINDING_INSTANCE_BUFFERS, std430) readonly buffer InstanceBuffer {
    Instance instance;
} instances[];

layout(set = 0, binding = BINDING_MATERIAL_BUFFERS, std430) readonly buffer MaterialBuffer {
    Material material;
} materials[];

layout(set = 0, binding = BINDING_LIGHT_BUFFER) uniform LightBuffer {
    Light light;
};

layout(set = 0, binding = BINDING_TEXTURE_SAMPLERS) uniform sampler2D[] textures;

#endif