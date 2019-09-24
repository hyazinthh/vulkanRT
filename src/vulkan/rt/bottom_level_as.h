#pragma once

#include "acceleration_structure.h"

class BottomLevelAS : public AccelerationStructure {
	public:

		BottomLevelAS(Device* device,
			Buffer* vertexBuffer, uint32_t vertexCount, VkDeviceSize vertexStride,
			Buffer* indexBuffer, uint32_t indexCount, bool isOpaque = true);

		BottomLevelAS(Device* device, float radius, bool isOpaque = true);

		~BottomLevelAS();

	private:
};