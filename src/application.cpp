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
	createSwapchain();
	createPipeline();
	createSyncObjects();
}

Application::~Application() {

	for (int i = 0; i < MAX_FRAMES; i++) {
		vkDestroySemaphore(device->get(), renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device->get(), imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device->get(), frameFences[i], nullptr);
	}

	delete pipeline;
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
		draw();
	}

	vkDeviceWaitIdle(device->get());
}

void Application::draw() {
	static int currentFrame = 0;

	vkWaitForFences(device->get(), 1, &frameFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(device->get(), 1, &frameFences[currentFrame]);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(device->get(), swapchain->get(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &pipeline->getCommandBuffers()[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

	if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, frameFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain->get();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAMES;
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

void Application::createSwapchain() {
	swapchain = device->createSwapChain(width, height);
}

void Application::createPipeline() {
	auto vs = Shader::loadFromFile(device, "shaders/triangle.vert", Shader::Type::Vertex);
	auto fs = Shader::loadFromFile(device, "shaders/triangle.frag", Shader::Type::Fragment);

	pipeline = new Pipeline(device, swapchain, vs, fs);

	delete fs;
	delete vs;
}

void Application::createSyncObjects() {

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES; i++) {
		if (vkCreateSemaphore(device->get(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device->get(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device->get(), &fenceInfo, nullptr, &frameFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create semaphores");
		}
	}
}