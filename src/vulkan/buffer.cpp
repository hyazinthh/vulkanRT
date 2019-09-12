#include "buffer.h"
#include "device.h"

Buffer::Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	: device(device), size(size) {

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(*device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(*device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(*device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory");
	}

	vkBindBufferMemory(*device, buffer, memory, 0);
}


Buffer::~Buffer() {
	vkDestroyBuffer(*device, buffer, nullptr);
	vkFreeMemory(*device, memory, nullptr);
}

void Buffer::bindAsVertexBuffer(VkDeviceSize offset) {
	VkDeviceSize offsets[] = { offset };
	vkCmdBindVertexBuffers(device->getCommandBuffer(), 0, 1, &buffer, offsets);
}

void Buffer::bindAsIndexBuffer(VkIndexType indexType, VkDeviceSize offset) {
	vkCmdBindIndexBuffer(device->getCommandBuffer(), buffer, offset, indexType);
}

void* Buffer::map() {
	void* data;
	vkMapMemory(*device, memory, 0, size, 0, &data);
	return data;
}

void Buffer::unmap() {
	vkUnmapMemory(*device, memory);
}

void Buffer::fill(const void* data) {
	void* buf = map();
	memcpy(buf, data, size);
	unmap();
}

void Buffer::copyTo(Buffer* dest) {
	VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, buffer, *dest, 1, &copyRegion);

	device->endSingleTimeCommands(commandBuffer);
}