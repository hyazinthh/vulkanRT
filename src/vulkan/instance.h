#ifndef INSTANCE_H_
#define INSTANCE_H_

#include <vector>
#include <vulkan/vulkan.hpp>

#include "device.h"

typedef PFN_vkDebugUtilsMessengerCallbackEXT DebugCallback;
typedef std::vector<std::string> Extensions;

class Application;

class Instance {

	public:
		Instance(Application* application, const Extensions& requiredExtensions, DebugCallback debugCallback = nullptr);

		~Instance();

		VkInstance get() { return instance; }

		bool isDebug() const { return debugCallback != nullptr; }

		Extensions getExtensions() const;

		Device* createDevice(Extensions extensions);		

	private:
		bool registerDebugCallback();

		Application* application;

		DebugCallback debugCallback;

		VkInstance instance;

		VkDebugUtilsMessengerEXT debugMessenger;
};

#endif