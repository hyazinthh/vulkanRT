#include "instance.h"
#include "extensions.h"

#include <algorithm>
#include "../application.h"

Instance::Instance(Application* application, const Extensions& requiredExtensions, DebugCallback debugCallback)
	: application(application), debugCallback(debugCallback) {

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = application->getName().c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	std::vector<const char*> validationLayers;
	if (isDebug()) {
		validationLayers.push_back("VK_LAYER_KHRONOS_validation");
	}

	std::vector<const char*> ext;
	std::transform(requiredExtensions.begin(), requiredExtensions.end(), std::back_inserter(ext), [](auto& e) {
		return e.c_str();
	});

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t) ext.size();
	createInfo.ppEnabledExtensionNames = ext.data();
	createInfo.enabledLayerCount = (uint32_t) validationLayers.size();
	createInfo.ppEnabledLayerNames = validationLayers.data();

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan instance");
	}

	if (VkExt::initProcs(this) != VK_SUCCESS) {
		throw std::runtime_error("Failed to setup Vulkan extension procs");
	}

	if (isDebug() && !registerDebugCallback()) {
		throw std::runtime_error("Failed to setup Vulkan debug callback");
	}
}

Instance::~Instance() {
	if (isDebug()) {
		VkExt::vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	vkDestroyInstance(instance, nullptr);
}

Extensions Instance::getExtensions() const {
	uint32_t count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

	std::vector<VkExtensionProperties> props(count);
	vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

	Extensions ext;
	std::transform(props.begin(), props.end(), std::back_inserter(ext), [](VkExtensionProperties& p) {
		return p.extensionName;
	});

	return ext;
}

Device* Instance::createDevice(Extensions extensions) {
	return new Device(this, application->getWidth(), application->getHeight(), application->getSurface(), extensions);
}

bool Instance::registerDebugCallback() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;

	return VkExt::vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) == VK_SUCCESS;
}