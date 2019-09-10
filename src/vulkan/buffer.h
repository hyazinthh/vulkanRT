#pragma once

#include <vulkan/vulkan.hpp>

class Device;

class Buffer {
	public:
		Buffer(Device* device, VkDeviceSize size, VkMemoryPropertyFlags properties, VkBufferUsageFlags usage);

		~Buffer();

		void bindAsVertexBuffer(VkDeviceSize offset = 0);

		void bindAsIndexBuffer(VkIndexType indexType, VkDeviceSize offset = 0);

		const VkBuffer& get() { return buffer; }

		VkDeviceMemory getMemory() { return memory; }

		VkDeviceSize getSize() { return size; }

		void fill(const void* data);

		void copyTo(Buffer* dest);

	private:

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		Device* device;

		VkBuffer buffer;

		VkDeviceMemory memory;

		VkDeviceSize size;
};

