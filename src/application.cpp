#include "application.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <chrono>

#include "vulkan/extensions.h"

struct UniformBufferObject {
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
};

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
											 VkDebugUtilsMessageTypeFlagsEXT,
											 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
											 void*) {

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

void printExtensions(const std::string& type, const StringList& extensions) {
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
	createBuffers();
	createScene();
	writeDescriptorSets();
}

Application::~Application() {

	delete scene;
	delete uniformBuffer;

	delete device;
	vkDestroySurfaceKHR(*instance, surface, nullptr);
	delete instance;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::update(float dt) {

	// Update scene
	auto id = glm::mat4(1.0f);
	auto scale = glm::scale(id, glm::vec3(0.5f));
	auto rotation = glm::rotate(id, dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	auto rotationSelf = glm::rotate(id, -dt * glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	auto translation = glm::translate(id, glm::vec3(1, 1, 0));

	scene->rotatingCube->transform = rotation * translation * rotationSelf * scale;

	auto red = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	auto blue = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	float t = glm::mod(dt, 2.0f);
	t = (t > 1) ? (2 - t) : t;

	scene->rotatingCube->color = (1 - t) * red + t * blue;

	scene->updateInstance(scene->rotatingCube);
	scene->buildAccelerationStructure(true);

	// Update matrices
	auto ext = device->getSwapchain()->getExtent();

	UniformBufferObject ubo = {};
	ubo.viewInverse = glm::inverse(glm::lookAt(glm::vec3(4.0f, 4.0f, 4.0f), glm::vec3(0.0f, 0.0f, -1.5f), glm::vec3(0.0f, 0.0f, 1.0f)));
	ubo.projInverse = glm::inverse(glm::perspective(glm::radians(45.0f), ext.width / (float) ext.height, 0.1f, 10.0f));
	ubo.projInverse[1][1] *= -1;

	uniformBuffer->fill(&ubo);
}

void Application::run() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		static auto lastTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		update(std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count());

		if (!device->frameBegin()) {
			std::cout << "frameBegin() failed!" << std::endl;
			continue;
		}

		updateRaytracingRenderTarget();
		device->beginRenderPass();

		scene->trace();

		device->endRenderPass();
		device->frameEnd();
		device->framePresent();
	}

	vkDeviceWaitIdle(*device);
}

void Application::queryExtensions() {
	auto ext = instance->getExtensions();

	for (auto& e : ext) {
		std::cout << e << std::endl;
	}

	std::cout << std::endl;
}

StringList Application::getRequiredInstanceExtensions() {

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	StringList extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (debug) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

StringList Application::getRequiredDeviceExtensions() {
	return { VK_NV_RAY_TRACING_EXTENSION_NAME };
}

void Application::createWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
}

void Application::createInstance() {
	instance = new Instance(this, debug ? debugCallback : nullptr);
	printExtensions("instance", instance->getExtensions());
}

void Application::createDevice() {
	device = instance->createDevice();
	printExtensions("device", device->getExtensions());
}

void Application::createSurface() {
	if (glfwCreateWindowSurface(*instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}
}

void Application::createScene() {
	scene = new Scene(device);
}

void Application::createBuffers() {
	{
		uniformBuffer = new Buffer(device, sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
}

VkDescriptorSetLayoutCreateInfo Application::getDescriptorSetLayoutInfo() {

	bindings.clear();

	// Acceleration structure
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = 0;
		b.descriptorCount = 1;
		b.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	// Result image
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = 1;
		b.descriptorCount = 1;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

		bindings.push_back(b);
	}

	// Camera uniform buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = 2;
		b.descriptorCount = 1;
		b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

		bindings.push_back(b);
	}

	// Vertex buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = 3;
		b.descriptorCount = 2;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	// Index buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = 4;
		b.descriptorCount = 2;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (uint32_t) bindings.size();
	layoutInfo.pBindings = bindings.data();

	return layoutInfo;
}

void Application::writeDescriptorSets() {

	for (const auto& ds : device->getDescriptorSets()) {
		{
			VkAccelerationStructureNV as = *scene->getAccelerationStructure();

			VkWriteDescriptorSetAccelerationStructureNV wdsAccelerationStructure = {};
			wdsAccelerationStructure.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
			wdsAccelerationStructure.accelerationStructureCount = 1;
			wdsAccelerationStructure.pAccelerationStructures = &as;

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.pNext = &wdsAccelerationStructure;
			wds.dstSet = ds;
			wds.dstBinding = 0;
			wds.descriptorCount = 1;
			wds.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			VkDescriptorBufferInfo info = {};
			info.buffer = *uniformBuffer;
			info.offset = 0;
			info.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = ds;
			wds.dstArrayElement = 0;
			wds.descriptorCount = 1;
			wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			wds.dstBinding = 2;
			wds.pBufferInfo = &info;

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			std::vector<VkDescriptorBufferInfo> info;
			
			for (const auto& o : scene->getObjects())
			{
				VkDescriptorBufferInfo i = {};
				i.buffer = *o->vertexBuffer;
				i.offset = 0;
				i.range = VK_WHOLE_SIZE;

				info.push_back(i);
			}

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = ds;
			wds.dstArrayElement = 0;
			wds.descriptorCount = (uint32_t) info.size();
			wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			wds.dstBinding = 3;
			wds.pBufferInfo = info.data();

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			std::vector<VkDescriptorBufferInfo> info;

			for (const auto& o : scene->getObjects()) {
				VkDescriptorBufferInfo i = {};
				i.buffer = *o->indexBuffer;
				i.offset = 0;
				i.range = VK_WHOLE_SIZE;

				info.push_back(i);
			}

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = ds;
			wds.dstArrayElement = 0;
			wds.descriptorCount = (uint32_t) info.size();
			wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			wds.dstBinding = 4;
			wds.pBufferInfo = info.data();

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}
	}
}

void Application::updateRaytracingRenderTarget() {

	VkImageSubresourceRange subresourceRange;
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	device->imageBarrier(device->getCommandBuffer(), device->getBackBuffer(), subresourceRange,
		0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	VkDescriptorImageInfo info;
	info.imageView = device->getBackBufferView();
	info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.sampler = VK_NULL_HANDLE;

	VkWriteDescriptorSet wds = {};
	wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	wds.dstSet = device->getDescriptorSet();
	wds.dstArrayElement = 0;
	wds.descriptorCount = 1;
	wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	wds.dstBinding = 1;
	wds.pImageInfo = &info;

	vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
}