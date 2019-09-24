#ifndef TYPES_GLSL_
#define TYPES_GLSL_

struct Vertex {
    vec4 position;
    vec3 normal;
    vec3 tangent;
    vec2 tc;
};

struct Face {
    uint indices[3];
};

struct Sphere {
    float radius;
};

struct Instance {
	int objectId;
    int materialId;
    mat3 normalMatrix;
};

struct Material {
    int textureId[4];
    vec4 color;
};

struct Camera {
    mat4 viewInverse;
    mat4 projInverse;
};

struct Light {
    vec4 position;
    vec4 diffuseColor;
};

struct RayPayload {
    vec4 color;
    int bounce;
};

struct RayTracingSettings {
    uint maxBounces;
    float tmin;
    float tmax;
};

#endif