#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <array>

struct Vertex {
	glm::vec4 position;
	glm::vec4 normal;
	glm::vec4 tangent;
	glm::vec4 bitangent;
	glm::vec4 texCoord;

	Vertex(const glm::vec3& p, const glm::vec3& n, const glm::vec3& t, const glm::vec3& b, const glm::vec2& uv) {
		position = glm::vec4(p, 1.0f);
		normal = glm::vec4(n, 1.0f);
		tangent = glm::vec4(t, 1.0f);
		bitangent = glm::vec4(b, 1.0f);
		texCoord = glm::vec4(uv, 0.0f, 0.0f);
	}

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription desc = {};
		desc.binding = 0;
		desc.stride = sizeof(Vertex);
		desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		
		return desc;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 2> desc;
		desc[0].binding = 0;
		desc[0].location = 0;
		desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		desc[0].offset = offsetof(Vertex, position);

		desc[1].binding = 0;
		desc[1].location = 1;
		desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		desc[1].offset = offsetof(Vertex, normal);

		return desc;
	}
};