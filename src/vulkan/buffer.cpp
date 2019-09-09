#include "buffer.h"
#include "device.h"

Buffer::Buffer(Device* device, VkDeviceSize size, VkMemoryPropertyFlags properties, VkBufferUsageFlags usage)
	: device(device), size(size) {

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device->get(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device->get(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device->get(), &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory");
	}

	vkBindBufferMemory(device->get(), buffer, memory, 0);
}


Buffer::~Buffer() {
	vkDestroyBuffer(device->get(), buffer, nullptr);
	vkFreeMemory(device->get(), memory, nullptr);
}

void Buffer::fill(const void* data) {
	void* buf;
	vkMapMemory(device->get(), memory, 0, size, 0, &buf);
	memcpy(buf, data, size);
	vkUnmapMemory(device->get(), memory);
}

uint32_t Buffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(device->getPhysical(), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i))
			&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}
