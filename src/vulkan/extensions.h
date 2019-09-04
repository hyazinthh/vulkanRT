#ifndef EXTENSIONS_H_
#define EXTENSIONS_H_

#include "instance.h"

namespace VkExt {
	extern PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
	extern PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

	VkResult initProcs(Instance* instance);
}

#endif