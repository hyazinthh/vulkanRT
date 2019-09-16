#include "raytracing_pipeline.h"
#include "../extensions.h"

#include <algorithm>

RaytracingPipeline::RaytracingPipeline(Device* device) 
	: Pipeline(device) {
}

uint32_t RaytracingPipeline::startHitGroup() {
	if (isHitGroupOpen) {
		throw std::logic_error("Hit group already open");
	}

	VkRayTracingShaderGroupCreateInfoNV groupInfo = {};
	groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groupInfo.generalShader = VK_SHADER_UNUSED_NV;
	groupInfo.closestHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	shaderGroups.push_back(groupInfo);

	isHitGroupOpen = true;
	return (uint32_t) shaderGroups.size() - 1;
}

void RaytracingPipeline::addHitShaderStage(Shader* shader) {
	if (!isHitGroupOpen) {
		throw std::logic_error("Cannot add hit stage in when no hit group open");
	}

	auto& group = shaderGroups.back();

	shaderStages.push_back(shader->getStageInfo());
	uint32_t shaderIndex = (uint32_t) shaderStages.size() - 1;

	switch (shader->getType()) {
		case Shader::Type::AnyHit:
			group.anyHitShader = shaderIndex;
			break;

		case Shader::Type::ClosestHit:
			group.closestHitShader = shaderIndex;
			break;

		case Shader::Type::Intersection:
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_NV;
			group.intersectionShader = shaderIndex;
			break;
		default:
			throw std::logic_error("Invalid shader type");
	}
}

void RaytracingPipeline::endHitGroup() {
	if (!isHitGroupOpen) {
		throw std::logic_error("No hit group open");
	}

	isHitGroupOpen = false;
}

uint32_t RaytracingPipeline::addShaderStage(Shader* shader) {
	if (isHitGroupOpen) {
		throw std::logic_error("Cannot add general stage in when hit group open");
	}

	shaderStages.push_back(shader->getStageInfo());
	uint32_t shaderIndex = (uint32_t) shaderStages.size() - 1;

	VkRayTracingShaderGroupCreateInfoNV groupInfo = {};
	groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	groupInfo.pNext = nullptr;
	groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groupInfo.generalShader = shaderIndex;
	groupInfo.closestHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	shaderGroups.push_back(groupInfo);

	return (uint32_t) shaderGroups.size() - 1;
}

RaytracingPipeline* RaytracingPipeline::create() {
	VkRayTracingPipelineCreateInfoNV info = {};
	info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	info.stageCount = (uint32_t) shaderStages.size();
	info.pStages = shaderStages.data();
	info.groupCount = (uint32_t) shaderGroups.size();
	info.pGroups = shaderGroups.data();
	info.maxRecursionDepth = maxRecursionDepth;
	info.layout = layout;
	info.basePipelineHandle = VK_NULL_HANDLE;
	info.basePipelineIndex = 0;

	if (VkExt::vkCreateRayTracingPipelinesNV(*device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create raytracing pipeline");
	}

	return this;
}