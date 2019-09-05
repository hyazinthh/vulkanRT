#include "device.h"
#include "instance.h"

#include <set>

Device::Device(Instance* instance, int width, int height, VkSurfaceKHR surface, Extensions requiredExtensions)
	: surface(surface), requiredExtensions(requiredExtensions) {
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(instance->get(), &count, nullptr);

	std::vector<VkPhysicalDevice> devices(count);
	vkEnumeratePhysicalDevices(instance->get(), &count, devices.data());

	for (auto& d : devices) {
		if (checkPhysicalDevice(d)) {
			createLogicalDevice(d, width, height);
			return;
		}
	}
	
	throw std::runtime_error("No suitable Vulkan device found");
}

Device::~Device() {
	vkDestroyDevice(device, nullptr);
}

Extensions Device::getExtensions(VkPhysicalDevice device) const {
	uint32_t count = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

	std::vector<VkExtensionProperties> props(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, props.data());

	Extensions ext;
	std::transform(props.begin(), props.end(), std::back_inserter(ext), [](VkExtensionProperties& p) {
		return p.extensionName;
	});

	return ext;
}

void Device::createLogicalDevice(VkPhysicalDevice physicalDevice, int width, int height) {
	this->physicalDevice = physicalDevice;
	queueFamilies = getQueueFamilies(physicalDevice);

	// Queues
	std::vector<VkDeviceQueueCreateInfo> queueInfos;
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilies.graphics.value(), queueFamilies.present.value() };

	float priority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = queueFamily;
		info.queueCount = 1;
		info.pQueuePriorities = &priority;
		queueInfos.push_back(info);
	}

	// Features
	VkPhysicalDeviceFeatures deviceFeatures = {};

	// Extensions
	std::vector<const char*> ext;
	ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	std::transform(requiredExtensions.begin(), requiredExtensions.end(), std::back_inserter(ext), [](auto& e) {
		return e.c_str();
	});

	// Create device
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pQueueCreateInfos = queueInfos.data();
	deviceInfo.queueCreateInfoCount = (uint32_t) queueInfos.size();
	deviceInfo.pEnabledFeatures = &deviceFeatures;
	deviceInfo.enabledExtensionCount = (uint32_t) ext.size();
	deviceInfo.ppEnabledExtensionNames = ext.data();

	if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan device");
	}

	// Get queues
	vkGetDeviceQueue(device, queueFamilies.graphics.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamilies.present.value(), 0, &presentQueue);
}

bool Device::checkPhysicalDevice(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);

	auto ext = getExtensions(device);
	for (auto e : requiredExtensions) {
		if (std::find(ext.begin(), ext.end(), e) == ext.end()) {
			return false;
		}
	}

	return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			getQueueFamilies(device).isComplete();
}

QueueFamilies Device::getQueueFamilies(VkPhysicalDevice device) const {
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

	std::vector<VkQueueFamilyProperties> props(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());

	QueueFamilies families;

	for (uint32_t i = 0; i < count; i++) {
		if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			families.graphics = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			families.present = i;
		}
	}

	return families;
}

SwapChain* Device::createSwapChain(uint32_t width, uint32_t height) {
	VkExtent2D extent = { width, height };
	return new SwapChain(this, surface, extent, VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, VK_PRESENT_MODE_FIFO_KHR);
}