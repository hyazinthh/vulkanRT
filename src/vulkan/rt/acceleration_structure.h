#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "../device.h"
#include "../buffer.h"

class AccelerationStructure {
	public:

		AccelerationStructure(Device* device) : device(device) {
		}

		virtual ~AccelerationStructure();

		operator VkAccelerationStructureNV() { return accelerationStructure; }

	protected:

		void computeMemoryRequirements(const VkAccelerationStructureInfoNV& info);

		void allocateMemory();

		void build(const VkAccelerationStructureInfoNV& info, Buffer* instanceBuffer, bool updateOnly = false);

		void create(const VkAccelerationStructureInfoNV& info, Buffer* instanceBuffer = nullptr);

		void barrier(VkCommandBuffer commandBuffer);

		Device* device = nullptr;

		VkAccelerationStructureNV accelerationStructure = VK_NULL_HANDLE;

		VkAccelerationStructureInfoNV info = {};

		VkMemoryRequirements resultMemoryRequirements = {};

		VkMemoryRequirements scratchMemoryRequirements = {};

		VkDeviceMemory resultMemory = VK_NULL_HANDLE;

		std::unique_ptr<Buffer> scratchBuffer;
};

