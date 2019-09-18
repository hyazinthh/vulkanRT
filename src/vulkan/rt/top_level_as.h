#pragma once

#include "acceleration_structure.h"
#include "bottom_level_as.h"

class TopLevelAS : public AccelerationStructure {
	public:
		struct Instance {
			Instance(BottomLevelAS* blAS,
				uint32_t instanceId, uint32_t hitGroup, uint32_t mask,
				const glm::mat4& transform);

			BottomLevelAS* bottomLevelAS;
			uint32_t instanceId;
			uint32_t hitGroupIndex;
			uint32_t mask;
			glm::mat4 transform;
		};

		TopLevelAS(Device* device, const std::vector<Instance>& instances, bool allowUpdate = false);

		~TopLevelAS();

		void update(const std::vector<Instance>& instances);

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

		Buffer* createInstanceBuffer(const std::vector<Instance>& instances);

		VkAccelerationStructureInfoNV info = {};
};

