#pragma once

#include "../device.h"
#include "../pipeline.h"

typedef std::vector<uint8_t> ByteArray;

class ShaderBindingTable {
	public:
		enum class EntryType {
			RayGen,
			Miss,
			Callable,
			HitGroup,
			Count
		};

		// Wrapper for SBT entries, each consisting of the (implicit) name of the program and a list of values,
		// which can be either offsets or raw 32-bit constants
		struct Entry {
			Entry(const void* data, size_t dataSize) {
				inlineData.resize(dataSize);
				memcpy_s(inlineData.data(), inlineData.size(), data, dataSize);
			}

			ByteArray inlineData;
		};

		ShaderBindingTable(Device* device, Pipeline* pipeline);

		~ShaderBindingTable();

		void addEntry(EntryType type, const void* data = nullptr, size_t dataSize = 0) {
			entries[(int) type].push_back(Entry(data, dataSize));
		}

		ShaderBindingTable* create();

		Buffer* getBuffer() { return buffer; }

		uint32_t getBaseIndex(EntryType type) const;

		VkDeviceSize getSectionSize(EntryType type) const;

		VkDeviceSize getEntrySize(EntryType type) const;

		VkDeviceSize getOffset(EntryType type) const;

		VkDeviceSize getShaderGroupHandleSize() const {
			return shaderGroupHandleSize;
		}

	private:

		// For each entry, copy the shader identifier followed by its resource pointers and/or root
		// constants in outputData, and returns the size in bytes actually written to outputData.
		VkDeviceSize copyShaderData(EntryType type, uint8_t* outputData,
			const uint8_t* shaderHandleStorage, bool inlineDataOnly = false);

		void getRaytracingProperties();

		VkDeviceSize getEntrySize(const std::vector<Entry>& entries) const;

		VkDeviceSize getSize();

		Device* device = nullptr;

		Pipeline* pipeline = nullptr;

		VkDeviceSize shaderGroupHandleSize = 0;

		std::vector<Entry> entries[(int) EntryType::Count];

		Buffer* buffer = nullptr;
};