#pragma once

#include <vulkan/vulkan.hpp>

class Device;

class Buffer {
	public:
		Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage, 
			   VkMemoryPropertyFlags properties = 0);

		~Buffer();

		void bindAsVertexBuffer(VkDeviceSize offset = 0);

		void bindAsIndexBuffer(VkIndexType indexType, VkDeviceSize offset = 0);

		operator VkBuffer() { return buffer; }

		VkDeviceMemory getMemory() { return memory; }

		VkDeviceSize getSize() { return size; }

		void* map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

		void unmap();

		void fill(const void* data);

		void copyTo(Buffer* dest);

	private:

		Device* device = nullptr;

		VkBuffer buffer = VK_NULL_HANDLE;

		VkDeviceMemory memory = VK_NULL_HANDLE;

		VkDeviceSize size = 0;
};

