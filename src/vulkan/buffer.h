#pragma once

#include <vulkan/vulkan.hpp>

class Device;

class Buffer {
	public:
		Buffer(Device* device, VkDeviceSize size, VkMemoryPropertyFlags properties, VkBufferUsageFlags usage);

		~Buffer();

		const VkBuffer& get() { return buffer; }

		VkDeviceMemory getMemory() { return memory; }

		VkDeviceSize getSize() { return size; }

		void fill(const void* data);

	private:

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		Device* device;

		VkBuffer buffer;

		VkDeviceMemory memory;

		VkDeviceSize size;
};

