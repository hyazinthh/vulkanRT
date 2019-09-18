#include "top_level_as.h"
#include "bottom_level_as.h"
#include "../extensions.h"

TopLevelAS::TopLevelAS(Device* device, const std::vector<Instance>& instances, bool allowUpdate) 
	: AccelerationStructure(device) {

	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	info.instanceCount = (uint32_t) instances.size();
	info.flags = allowUpdate ? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV : 0;
	info.geometryCount = 0;   // Since this is a top-level AS, it does not contain any geometry
	info.pGeometries = VK_NULL_HANDLE;

	std::unique_ptr<Buffer> buffer(createInstanceBuffer(instances));
	create(info, buffer.get());
}

TopLevelAS::~TopLevelAS() {
}

void TopLevelAS::update(const std::vector<Instance>& instances) {
	std::unique_ptr<Buffer> buffer(createInstanceBuffer(instances));
	build(info, buffer.get(), true);
}

Buffer* TopLevelAS::createInstanceBuffer(const std::vector<Instance>& instances) {
	std::vector<VkGeometryInstance> geometryInstances;

	for (const auto& inst : instances) {
		uint64_t handle = 0;
		if (VkExt::vkGetAccelerationStructureHandleNV(*device, *inst.bottomLevelAS, sizeof(uint64_t), &handle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to get acceleration structure handle");
		}

		glm::mat4 transform = glm::transpose(inst.transform);

		VkGeometryInstance gInst;
		gInst.instanceId = inst.instanceId;
		gInst.mask = inst.mask;
		gInst.instanceOffset = inst.hitGroupIndex;
		gInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		gInst.accelerationStructureHandle = handle;
		memcpy(gInst.transform, &transform, sizeof(gInst.transform));

		geometryInstances.push_back(gInst);
	}

	Buffer* instanceBuffer = nullptr;

	if (geometryInstances.size() > 0) {
		instanceBuffer = new Buffer(device, geometryInstances.size() * sizeof(VkGeometryInstance),
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		instanceBuffer->fill(geometryInstances.data());
	}

	return instanceBuffer;
}

TopLevelAS::Instance::Instance(BottomLevelAS* blAS,
	uint32_t instanceId, uint32_t hitGroup, uint32_t mask,
	const glm::mat4& transform) {

	this->bottomLevelAS = blAS;
	this->instanceId = instanceId;
	this->hitGroupIndex = hitGroup;
	this->mask = mask;
	this->transform = transform;
}
