#ifndef DEVICE_H_
#define DEVICE_H_

#include <vulkan/vulkan.hpp>
#include <optional>

#include "swap_chain.h"

class Instance;
typedef std::vector<std::string> Extensions;

struct QueueFamilies {
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;

	bool isComplete() {
		return graphics.has_value() && present.has_value();
	}
};

class Device {
	public:
		Device(Instance* instance, int width, int height, VkSurfaceKHR surface, Extensions requiredExtensions);

		~Device();

		VkDevice get() const { return device; }

		VkPhysicalDevice getPhysical() const { return physicalDevice; }

		VkSurfaceKHR getSurface() const { return surface; }

		Extensions getExtensions() const {
			return getExtensions(physicalDevice);
		}

		QueueFamilies getQueueFamilies() const {
			return getQueueFamilies(physicalDevice);
		}

		VkQueue getGraphicsQueue() {
			return graphicsQueue;
		}

		VkQueue getPresentQueue() {
			return presentQueue;
		}

		SwapChain* createSwapChain(uint32_t width, uint32_t height);

	private:
		Extensions getExtensions(VkPhysicalDevice device) const;

		bool checkPhysicalDevice(VkPhysicalDevice device);

		QueueFamilies getQueueFamilies(VkPhysicalDevice device) const;

		void createLogicalDevice(VkPhysicalDevice device, int width, int height);

		VkDevice device;

		VkPhysicalDevice physicalDevice;

		VkSurfaceKHR surface;

		QueueFamilies queueFamilies;

		VkQueue graphicsQueue, presentQueue;

		Extensions requiredExtensions;
};

#endif