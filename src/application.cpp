#include "application.h"

#include <vector>
#include <iostream>
#include <stdexcept>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
											 VkDebugUtilsMessageTypeFlagsEXT messageType,
											 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
											 void* pUserData) {

	switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			std::cout << "Diagnostic: ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			std::cout << "Info: ";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			std::cout << "Warning: ";
			break;
		default:
			std::cout << "Error: ";
	}

	std::cout << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void printExtensions(const std::string& type, const Extensions& extensions) {
	std::cout << "Vulkan " << type << " reports " << extensions.size() << " extensions: " << std::endl << std::endl;

	for (auto e : extensions) {
		std::cout << e << std::endl;
	}

	std::cout << std::endl;
}

Application::Application(const std::string& name, uint32_t width, uint32_t height)
		: name(name), width(width), height(height) {
	createWindow();
	createInstance();
	createSurface();
	createDevice();
}

Application::~Application() {
	delete swapchain;
	delete device;
	vkDestroySurfaceKHR(instance->get(), surface, nullptr);
	delete instance;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::run() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void Application::queryExtensions() {
	auto ext = instance->getExtensions();

	for (auto& e : ext) {
		std::cout << e << std::endl;
	}

	std::cout << std::endl;
}

Extensions Application::getRequiredExtensions(bool debug) {

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	Extensions extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (debug) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}


void Application::createWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
}

void Application::createInstance() {
#ifndef NDEBUG
	auto cb = debugCallback;
#else
	auto cb = nullptr;
#endif

	instance = new Instance(this, getRequiredExtensions(cb != nullptr), cb);
	printExtensions("instance", instance->getExtensions());
}

void Application::createDevice() {
	device = instance->createDevice(Extensions());
	printExtensions("device", device->getExtensions());
}

void Application::createSurface() {
	if (glfwCreateWindowSurface(instance->get(), window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}
}