#include "acceleration_structure.h"
#include "../extensions.h"

#include <memory>

AccelerationStructure::AccelerationStructure(Device* device, VkAccelerationStructureInfoNV info, const std::vector<Instance>& instances)
	: device(device), info(info), instances(instances) {

	VkAccelerationStructureCreateInfoNV createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	createInfo.compactedSize = 0;
	createInfo.info = info;

	if (VkExt::vkCreateAccelerationStructureNV(device->get(), &createInfo, nullptr, &accelerationStructure) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create acceleration structure");
	}

	computeMemoryRequirements();
	generate();
}

AccelerationStructure::~AccelerationStructure() {
	vkFreeMemory(device->get(), resultMemory, nullptr);
	VkExt::vkDestroyAccelerationStructureNV(device->get(), accelerationStructure, nullptr);
}

AccelerationStructure* AccelerationStructure::createBottomLevel(Device* device,
	Buffer* vertexBuffer, uint32_t vertexCount, VkDeviceSize vertexStride,
	Buffer* indexBuffer, uint32_t indexCount, bool isOpaque) {

	VkGeometryNV geometry;
	geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	geometry.pNext = nullptr;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	geometry.geometry.triangles.pNext = nullptr;
	geometry.geometry.triangles.vertexData = vertexBuffer->get();
	geometry.geometry.triangles.vertexOffset = 0;
	geometry.geometry.triangles.vertexCount = vertexCount;
	geometry.geometry.triangles.vertexStride = vertexStride;
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.indexData = indexBuffer->get();
	geometry.geometry.triangles.indexOffset = 0;
	geometry.geometry.triangles.indexCount = indexCount;
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
	geometry.geometry.triangles.transformOffset = 0;
	geometry.geometry.aabbs = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
	geometry.flags = isOpaque ? VK_GEOMETRY_OPAQUE_BIT_NV : 0;

	VkAccelerationStructureInfoNV info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	info.instanceCount = 0;
	info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
	info.geometryCount = 1;
	info.pGeometries = &geometry;

	return new AccelerationStructure(device, info, std::vector<Instance>());
}

AccelerationStructure* AccelerationStructure::createTopLevel(Device* device, const std::vector<Instance>& instances) {

	VkAccelerationStructureInfoNV info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	info.instanceCount = (uint32_t) instances.size();
	info.geometryCount = 0;   // Since this is a top-level AS, it does not contain any geometry
	info.pGeometries = VK_NULL_HANDLE;

	return new AccelerationStructure(device, info, instances);
}

void AccelerationStructure::computeMemoryRequirements() {
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo;
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.pNext = nullptr;
	memoryRequirementsInfo.accelerationStructure = accelerationStructure;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;

	// Result size
	VkMemoryRequirements2 req;
	VkExt::vkGetAccelerationStructureMemoryRequirementsNV(device->get(), &memoryRequirementsInfo, &req);
	resultMemoryRequirements = req.memoryRequirements;

	// Build size
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
	VkExt::vkGetAccelerationStructureMemoryRequirementsNV(device->get(), &memoryRequirementsInfo, &req);
	buildMemoryRequirements = req.memoryRequirements;

	// Update size
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
	VkExt::vkGetAccelerationStructureMemoryRequirementsNV(device->get(), &memoryRequirementsInfo, &req);
	updateMemoryRequirements = req.memoryRequirements;
}

void AccelerationStructure::generate() {
	{
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = resultMemoryRequirements.size;
		allocInfo.memoryTypeIndex = device->findMemoryType(resultMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(device->get(), &allocInfo, nullptr, &resultMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate acceleration structure result memory");
		}
	}

	// Instances
	std::vector<VkGeometryInstance> geometryInstances;

	for (const auto& inst : instances) {
		uint64_t handle = 0;
		if (VkExt::vkGetAccelerationStructureHandleNV(device->get(), inst.bottomLevelAS->get(), sizeof(uint64_t), &handle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to get acceleration structure handle");
		}

		VkGeometryInstance gInst;
		gInst.instanceId = inst.instanceId;
		gInst.mask = 0xff;
		gInst.instanceOffset = inst.hitGroupIndex;
		gInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		gInst.accelerationStructureHandle = handle;

		glm::mat4x4 transp = glm::transpose(inst.transform);
		memcpy(gInst.transform, &transp, sizeof(gInst.transform));

		geometryInstances.push_back(gInst);
	}

	std::unique_ptr<Buffer> instancesBuffer;

	if (geometryInstances.size() > 0) {
		instancesBuffer.reset(new Buffer(device, geometryInstances.size() * sizeof(VkGeometryInstance),
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		instancesBuffer->fill(geometryInstances.data());
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

	VkExt::vkBindAccelerationStructureMemoryNV(device->get(), 1, &bindInfo);

	Buffer scratchBuffer(device, buildMemoryRequirements.size, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	auto cmdBuffer = device->beginSingleTimeCommands();
	VkExt::vkCmdBuildAccelerationStructureNV(cmdBuffer, &info, 
		instancesBuffer ? instancesBuffer->get() : VK_NULL_HANDLE, 
		0, false, accelerationStructure, VK_NULL_HANDLE, scratchBuffer.get(), 0);

	VkMemoryBarrier memoryBarrier;
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	// TODO: Needed for single time command buffer?
	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

	device->endSingleTimeCommands(cmdBuffer);
}

AccelerationStructure::Instance::Instance(AccelerationStructure* blAS,
	uint32_t instanceId, uint32_t hitGroup, const glm::mat4x4& transform)
	: bottomLevelAS(blAS), transform(transform), instanceId(instanceId), hitGroupIndex(hitGroup) {
}