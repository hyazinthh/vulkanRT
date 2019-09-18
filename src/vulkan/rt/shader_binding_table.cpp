#include "shader_binding_table.h"
#include "../extensions.h"

#include <algorithm>

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment) (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

ShaderBindingTable::ShaderBindingTable(Device* device, Pipeline* pipeline) 
	: device(device), pipeline(pipeline) {
	getRaytracingProperties();
}

ShaderBindingTable::~ShaderBindingTable() {
	delete buffer;
}

ShaderBindingTable* ShaderBindingTable::create() {

	uint32_t groupCount = 0;
	for (const auto& e : entries) {
		groupCount += (uint32_t) e.size();
	}

	ByteArray shaderHandles(groupCount * shaderGroupHandleSize);

	if (VkExt::vkGetRayTracingShaderGroupHandlesNV(*device, *pipeline, 0, groupCount, 
												   (uint32_t) shaderHandles.size(), shaderHandles.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to get raytracing shader group handles");
	}

	buffer = new Buffer(device, getSize(),
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


	auto* data = (uint8_t*) buffer->map();

	// Copy the shader identifiers followed by their resource pointers or root constants: first the
	// ray generation, then the miss shaders, and finally the set of hit groups
	for (int i = 0; i < (int) EntryType::Count; i++) {
		data += copyShaderData((EntryType) i, data, shaderHandles.data());
	}

	buffer->unmap();

	return this;
}

VkDeviceSize ShaderBindingTable::copyShaderData(EntryType type, uint8_t* outputData,
	const uint8_t* shaderHandleStorage, bool inlineDataOnly) {

	uint8_t* pData = outputData;
	const auto& entries = this->entries[(int) type];
	auto entrySize = getEntrySize(entries);

	for (int i = 0; i < entries.size(); i++) {
		uint32_t groupIndex = getBaseIndex(type) + i;
		const auto& data = entries[i].inlineData;

		// Copy the shader identifier that was previously obtained with
		// vkGetRayTracingShaderGroupHandlesNV
		if (!inlineDataOnly) {
			memcpy(pData, shaderHandleStorage + groupIndex * shaderGroupHandleSize, shaderGroupHandleSize);
		}

		// Copy all its resources pointers or values in bulk
		if (!data.empty()) {
			memcpy(pData + shaderGroupHandleSize, data.data(), data.size());
		}

		pData += entrySize;
	}
	// Return the number of bytes actually written to the output buffer
	return static_cast<uint32_t>(entries.size()) * entrySize;
}

void ShaderBindingTable::getRaytracingProperties() {
	VkPhysicalDeviceRayTracingPropertiesNV rayTracingProperties = {};
	rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;

	VkPhysicalDeviceProperties2 deviceProps2 = {};
	deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProps2.pNext = &rayTracingProperties;

	vkGetPhysicalDeviceProperties2(device->getPhysical(), &deviceProps2);

	shaderGroupHandleSize = rayTracingProperties.shaderGroupHandleSize;
}

VkDeviceSize ShaderBindingTable::getEntrySize(const std::vector<Entry>& entries) const {
	// Find the maximum number of parameters used by a single entry
	size_t maxDataSize = 0;
	for (const auto& e : entries) {
		maxDataSize = std::max(maxDataSize, e.inlineData.size());
	}

	// A SBT entry is made of a program ID and a set of 4-byte parameters (offsets or push constants)
	VkDeviceSize entrySize = shaderGroupHandleSize + static_cast<VkDeviceSize>(maxDataSize);

	// The entries of the shader binding table must be 16-bytes-aligned
	return ROUND_UP(entrySize, 16);
}

VkDeviceSize ShaderBindingTable::getSize() {

	VkDeviceSize entrySizes = 0;
	for (const auto& e : entries) {
		entrySizes += getEntrySize(e) * e.size();
	}

	return entrySizes;
}

uint32_t ShaderBindingTable::getBaseIndex(EntryType type) const {

	uint32_t index = 0;

	for (uint32_t i = 0; i < (uint32_t) type; i++) {
		index += (uint32_t) entries[i].size();
	}

	return index;
}

VkDeviceSize ShaderBindingTable::getSectionSize(EntryType type) const {
	return getEntrySize(type) * entries[(int) type].size();
}

VkDeviceSize ShaderBindingTable::getEntrySize(EntryType type) const {
	return getEntrySize(entries[(int) type]);
}

VkDeviceSize ShaderBindingTable::getOffset(EntryType type) const {
	VkDeviceSize offset = 0;

	for (int i = 0; i < (int) type; i++) {
		offset += getSectionSize((EntryType) i);
	}

	return offset;
}