#version 460
#extension GL_NV_ray_tracing : require

layout(location = 0) rayPayloadInNV vec4 resultColor;
hitAttributeNV vec2 HitAttribs;

struct Vertex
{
    vec4 position;
    vec4 color;
};

layout(binding = 3, set = 0) buffer Vertices { Vertex v[]; }
vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; }
indices;

void main() {

    ivec3 ind = ivec3(indices.i[3 * gl_PrimitiveID],
        indices.i[3 * gl_PrimitiveID + 1],
        indices.i[3 * gl_PrimitiveID + 2]);

    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];    

    const vec3 barycentrics = vec3(1.0f - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);
    resultColor = v0.color * barycentrics.x + v1.color * barycentrics.y + v2.color * barycentrics.z;
}