#include "acceleration_structure.h"
#include "../extensions.h"

#include <memory>

AccelerationStructure::~AccelerationStructure() {
	vkFreeMemory(*device, resultMemory, nullptr);
	VkExt::vkDestroyAccelerationStructureNV(*device, accelerationStructure, nullptr);
}

void AccelerationStructure::create(const VkAccelerationStructureInfoNV& info, Buffer* instanceBuffer) {

	VkAccelerationStructureCreateInfoNV createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	createInfo.compactedSize = 0;
	createInfo.info = info;

	if (VkExt::vkCreateAccelerationStructureNV(*device, &createInfo, nullptr, &accelerationStructure) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create acceleration structure");
	}

	computeMemoryRequirements(info);
	allocateMemory();
	build(info, instanceBuffer);
}

void AccelerationStructure::allocateMemory() {

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = resultMemoryRequirements.size;
	allocInfo.memoryTypeIndex = device->findMemoryType(resultMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(*device, &allocInfo, nullptr, &resultMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate acceleration structure result memory");
	}

	// Bind the acceleration structure descriptor to the actual memory that will store the AS itself
	VkBindAccelerationStructureMemoryInfoNV bindInfo;
	bindInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	bindInfo.pNext = nullptr;
	bindInfo.accelerationStructure = accelerationStructure;
	bindInfo.memory = resultMemory;
	bindInfo.memoryOffset = 0;
	bindInfo.deviceIndexCount = 0;
	bindInfo.pDeviceIndices = nullptr;

	VkExt::vkBindAccelerationStructureMemoryNV(*device, 1, &bindInfo);

	// Buffer for update / build
	scratchBuffer.reset(new Buffer(device, scratchMemoryRequirements.size,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
}

void AccelerationStructure::build(const VkAccelerationStructureInfoNV& info, Buffer* instanceBuffer, bool updateOnly) {

	auto cmdBuffer = device->beginSingleTimeCommands();
	VkExt::vkCmdBuildAccelerationStructureNV(cmdBuffer, &info,
		instanceBuffer ? *instanceBuffer : VK_NULL_HANDLE, 0, false,
		accelerationStructure, updateOnly ? accelerationStructure : VK_NULL_HANDLE,
		*scratchBuffer, 0);

	barrier(cmdBuffer);
	device->endSingleTimeCommands(cmdBuffer);
}

void AccelerationStructure::computeMemoryRequirements(const VkAccelerationStructureInfoNV& info) {
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo;
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.pNext = nullptr;
	memoryRequirementsInfo.accelerationStructure = accelerationStructure;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;

	// Result size
	VkMemoryRequirements2 req;
	VkExt::vkGetAccelerationStructureMemoryRequirementsNV(*device, &memoryRequirementsInfo, &req);
	resultMemoryRequirements = req.memoryRequirements;

	// Scratch size
	bool update = info.flags &  VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;

	memoryRequirementsInfo.type = update 
		? VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV 
		: VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

	VkExt::vkGetAccelerationStructureMemoryRequirementsNV(*device, &memoryRequirementsInfo, &req);
	scratchMemoryRequirements = req.memoryRequirements;
}

void AccelerationStructure::barrier(VkCommandBuffer commandBuffer) {
	VkMemoryBarrier memoryBarrier;
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV,
		0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}