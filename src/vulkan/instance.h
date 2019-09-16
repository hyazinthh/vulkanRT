#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "device.h"

typedef PFN_vkDebugUtilsMessengerCallbackEXT DebugCallback;
typedef std::vector<std::string> StringList;

class Application;

class Instance {

	public:
		Instance(Application* application, DebugCallback debugCallback = nullptr);

		~Instance();

		operator VkInstance() { return instance; }

		bool isDebug() const { return debugCallback != nullptr; }

		StringList getExtensions() const;

		Device* createDevice();

	private:
		bool registerDebugCallback();

		Application* application = nullptr;

		DebugCallback debugCallback = nullptr;

		VkInstance instance = VK_NULL_HANDLE;

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
};