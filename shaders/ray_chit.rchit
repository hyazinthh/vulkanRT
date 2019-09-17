#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) rayPayloadInNV vec4 resultColor;
hitAttributeNV vec2 hitAttribs;

struct Vertex {
    vec4 position;
    vec4 color;
};

layout(binding = 3, set = 0) buffer Vertices { Vertex v[]; }
vertices[];

layout(binding = 4, set = 0) buffer Indices { uint i[]; }
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
    resultColor = v0.color * barycentrics.x + v1.color * barycentrics.y + v2.color * barycentrics.z;
}