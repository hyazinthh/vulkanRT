#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "../device.h"
#include "../buffer.h"

class AccelerationStructure {
	public:

		struct Instance {
			Instance(AccelerationStructure* blAS, uint32_t instanceId, uint32_t hitGroup, const glm::mat4x4& transform = glm::mat4x4());

			AccelerationStructure* bottomLevelAS;
			const glm::mat4x4 transform;
			uint32_t instanceId;
			uint32_t hitGroupIndex;
		};

		AccelerationStructure(Device* device, VkAccelerationStructureInfoNV info, const std::vector<Instance>& instances);

		~AccelerationStructure();

		static AccelerationStructure* createBottomLevel(Device* device,
			Buffer* vertexBuffer, uint32_t vertexCount, VkDeviceSize vertexStride,
			Buffer* indexBuffer, uint32_t indexCount, bool isOpaque = true);

		static AccelerationStructure* createTopLevel(Device* device, const std::vector<Instance>& instances);

		VkAccelerationStructureNV get() { return accelerationStructure; }

	private:

		// Geometry instance, with the layout expected by VK_NV_ray_tracing
		struct VkGeometryInstance {
			// Transform matrix, containing only the top 3 rows
			float transform[12];
			// Instance index
			uint32_t instanceId : 24;
			// Visibility mask
			uint32_t mask : 8;
			// Index of the hit group which will be invoked when a ray hits the instance
			uint32_t instanceOffset : 24;
			// Instance flags, such as culling
			uint32_t flags : 8;
			// Opaque handle of the bottom-level acceleration structure
			uint64_t accelerationStructureHandle;
		};

		static_assert(sizeof(VkGeometryInstance) == 64,
			"VkGeometryInstance structure compiles to incorrect size");

		void computeMemoryRequirements();

		void generate();

		Device* device = nullptr;

		std::vector<VkGeometryNV> geometries;

		VkAccelerationStructureNV accelerationStructure = VK_NULL_HANDLE;

		VkAccelerationStructureInfoNV info = {};

		VkMemoryRequirements resultMemoryRequirements = {};

		VkMemoryRequirements buildMemoryRequirements = {};

		VkMemoryRequirements updateMemoryRequirements = {};

		VkDeviceMemory resultMemory = VK_NULL_HANDLE;

		std::vector<Instance> instances;
};

