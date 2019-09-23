#pragma once

#include "../pipeline.h"
#include "shader_binding_table.h"
#include <optional>

struct ShaderGroup {
	VkRayTracingShaderGroupCreateInfoNV getInfo() const {
		VkRayTracingShaderGroupCreateInfoNV info = {};
		info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
		info.type = type;
		info.anyHitShader = anyHitShader;
		info.closestHitShader = closestHitShader;
		info.generalShader = generalShader;
		info.intersectionShader = intersectionShader;
		return info;
	}

	VkRayTracingShaderGroupTypeNV type;
	uint32_t anyHitShader = VK_SHADER_UNUSED_NV;
	uint32_t closestHitShader = VK_SHADER_UNUSED_NV;
	uint32_t generalShader = VK_SHADER_UNUSED_NV;
	uint32_t intersectionShader = VK_SHADER_UNUSED_NV;
};

class RaytracingPipeline : public Pipeline {
	public:
		RaytracingPipeline(Device* device);

		// Start the description of a hit group, that contains at least a closest hit shader, but may
		// also contain an intesection shader and a any-hit shader. The method outputs the index of the
		// created hit group
		uint32_t startHitGroup();

		// Add a hit shader stage in the current hit group
		void addHitShaderStage(Shader* shader);

		// End the description of the hit group
		void endHitGroup();

		// Add a general shader stage, and return the index of the created stage
		uint32_t addShaderStage(Shader* shader);

		// Finalize the pipeline
		RaytracingPipeline* create();

		ShaderBindingTable* generateShaderBindingTable();

	private:
		// Shader stages contained in the pipeline
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		// Each shader stage belongs to a group. There are 3 group types: general, triangle hit and procedural hit.
		// The general group type (VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV) is used for raygen, miss and callable shaders.
		// The triangle hit group type (VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV) is used for closest hit and
		// any hit shaders, when used together with the built-in ray-triangle intersection shader.
		// The procedural hit group type (VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_NV) is used for custom
		// intersection shaders, and also groups closest hit and any hit shaders that are used together with that intersection shader.
		std::vector<VkRayTracingShaderGroupCreateInfoNV> shaderGroups;

		// True if a group description is currently started
		bool isHitGroupOpen = false;

		uint32_t currentHitGroup = 0;

		uint32_t maxRecursionDepth = 8;

};

