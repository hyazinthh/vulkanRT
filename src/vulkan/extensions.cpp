#include "extensions.h"

PFN_vkCreateDebugUtilsMessengerEXT VkExt::vkCreateDebugUtilsMessengerEXT = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT VkExt::vkDestroyDebugUtilsMessengerEXT = nullptr;

VkResult VkExt::initProcs(Instance* instance) {
	if (instance->isDebug()) {
		if ((vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance->get(), "vkCreateDebugUtilsMessengerEXT")) == nullptr) {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		if ((vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance->get(), "vkDestroyDebugUtilsMessengerEXT")) == nullptr) {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	return VK_SUCCESS;
}