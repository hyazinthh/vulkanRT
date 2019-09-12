#pragma once

#include "../device.h"
#include "../pipeline.h"

typedef std::vector<uint8_t> ByteArray;

class ShaderBindingTable {
	public:
		enum class EntryType {
			RayGen,
			Miss,
			HitGroup,
			Count
		};

		// Wrapper for SBT entries, each consisting of the name of the program and a list of values,
		// which can be either offsets or raw 32-bit constants
		struct Entry {
			Entry(uint32_t groupIndex, const ByteArray inlineData)
				: groupIndex(groupIndex), inlineData(inlineData) {
			}

			uint32_t groupIndex;
			const ByteArray inlineData;
		};

		ShaderBindingTable(Device* device, Pipeline* pipeline);

		~ShaderBindingTable();

		void addEntry(EntryType type, uint32_t groupIndex, const ByteArray& data = ByteArray()) {
			entries[(int) type].push_back(Entry(groupIndex, data));
		}

		ShaderBindingTable* create();

		VkDeviceSize getSectionSize(EntryType type) const;

		VkDeviceSize getEntrySize(EntryType type) const;

		VkDeviceSize getOffset(EntryType type) const;

	private:

		// For each entry, copy the shader identifier followed by its resource pointers and/or root
		// constants in outputData, and returns the size in bytes actually written to outputData.
		VkDeviceSize copyShaderData(EntryType type, uint8_t* outputData, const uint8_t* shaderHandleStorage);

		void getRaytracingProperties();

		VkDeviceSize getEntrySize(const std::vector<Entry>& entries) const;

		VkDeviceSize getSize();

		Device* device = nullptr;

		Pipeline* pipeline = nullptr;

		VkDeviceSize shaderGroupHandleSize = 0;

		std::vector<Entry> entries[(int) EntryType::Count];

		Buffer* buffer = nullptr;
};