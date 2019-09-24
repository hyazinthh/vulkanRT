#include "application.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <chrono>

#include "vulkan/extensions.h"

const uint32_t BINDING_SCENE = 0;
const uint32_t BINDING_OUTPUT = 1;
const uint32_t BINDING_SETTINGS = 2;
const uint32_t BINDING_CAMERA = 3;
const uint32_t BINDING_VERTEX_BUFFERS = 4;
const uint32_t BINDING_INDEX_BUFFERS = 5;
const uint32_t BINDING_SPHERE_BUFFERS = 6;
const uint32_t BINDING_INSTANCE_BUFFERS = 7;
const uint32_t BINDING_MATERIAL_BUFFERS = 8;
const uint32_t BINDING_TEXTURE_SAMPLERS = 9;
const uint32_t BINDING_LIGHT_BUFFER = 10;

struct SettingsUniforms {
	uint32_t maxBounces;
	float tmin;
	float tmax;
};

struct CameraUniforms {
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
};

struct LightUniforms {
	glm::vec4 position;
	glm::vec4 diffuseColor;
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
	delete settingsUniformBuffer;
	delete cameraUniformBuffer;
	delete lightUniformBuffer;
	delete device;
	vkDestroySurfaceKHR(*instance, surface, nullptr);
	delete instance;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::update(float dt) {

	// Update scene
	auto id = glm::mat4(1.0f);

	{
		auto rotation = glm::rotate(id, dt * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		auto rotationSelf = glm::rotate(
								glm::rotate(id, -dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
							    -dt * glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)
							);
		auto translation = glm::translate(id, glm::vec3(1, 1, 1));

		scene->rotatingCube->transform = rotation * translation * rotationSelf;
	}

	LightUniforms light = {};

	{
		auto scale = glm::scale(id, glm::vec3(0.025f));
		auto translation = glm::translate(id, glm::vec3(
			glm::cos(dt * glm::radians(90.0f)) * 3, 
			0,
			3.5f + glm::sin(dt * glm::radians(180.0f)) * 0.5f)
		);

		scene->pointLight->transform = translation * scale;
		light.position = scene->pointLight->transform * glm::vec4(0, 0, 0, 1);
	}

	lightUniformBuffer->fill(&light);

	scene->updateInstance(scene->rotatingCube);
	scene->updateInstance(scene->pointLight);
	scene->buildAccelerationStructure(true);

	// Update matrices
	auto ext = device->getSwapchain()->getExtent();

	CameraUniforms camera = {};
	camera.viewInverse = glm::inverse(glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	camera.projInverse = glm::inverse(glm::perspective(glm::radians(45.0f), ext.width / (float) ext.height, 0.1f, 10.0f));
	camera.projInverse[1][1] *= -1;

	cameraUniformBuffer->fill(&camera);
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
		settingsUniformBuffer = new Buffer(device, sizeof(SettingsUniforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		SettingsUniforms settings;
		settings.maxBounces = 1;
		settings.tmin = 0.001f;
		settings.tmax = 48.0f;

		settingsUniformBuffer->fill(&settings);
	}

	{
		cameraUniformBuffer = new Buffer(device, sizeof(CameraUniforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	{
		lightUniformBuffer = new Buffer(device, sizeof(LightUniforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
}

VkDescriptorSetLayoutCreateInfo Application::getDescriptorSetLayoutInfo() {

	bindings.clear();

	// Acceleration structure
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_SCENE;
		b.descriptorCount = 1;
		b.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	// Result image
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_OUTPUT;
		b.descriptorCount = 1;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

		bindings.push_back(b);
	}

	// Settings uniform buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_SETTINGS;
		b.descriptorCount = 1;
		b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	// Camera uniform buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_CAMERA;
		b.descriptorCount = 1;
		b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

		bindings.push_back(b);
	}

	// Vertex buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_VERTEX_BUFFERS;
		b.descriptorCount = 32;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	// Index buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_INDEX_BUFFERS;
		b.descriptorCount = 32;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	// Sphere buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_SPHERE_BUFFERS;
		b.descriptorCount = 32;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_INTERSECTION_BIT_NV;

		bindings.push_back(b);
	}

	// Instances
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_INSTANCE_BUFFERS;
		b.descriptorCount = 32;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_INTERSECTION_BIT_NV;

		bindings.push_back(b);
	}

	// Materials
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_MATERIAL_BUFFERS;
		b.descriptorCount = 32;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	// Textures
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_TEXTURE_SAMPLERS;
		b.descriptorCount = 32;
		b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	// Light uniform buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = BINDING_LIGHT_BUFFER;
		b.descriptorCount = 1;
		b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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
			wds.dstBinding = BINDING_SCENE;
			wds.descriptorCount = 1;
			wds.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			VkDescriptorBufferInfo info = {};
			info.buffer = *settingsUniformBuffer;
			info.offset = 0;
			info.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = ds;
			wds.dstArrayElement = 0;
			wds.descriptorCount = 1;
			wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			wds.dstBinding = BINDING_SETTINGS;
			wds.pBufferInfo = &info;

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			VkDescriptorBufferInfo info = {};
			info.buffer = *cameraUniformBuffer;
			info.offset = 0;
			info.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = ds;
			wds.dstArrayElement = 0;
			wds.descriptorCount = 1;
			wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			wds.dstBinding = BINDING_CAMERA;
			wds.pBufferInfo = &info;

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			std::vector<VkDescriptorBufferInfo> info;
			
			for (const auto& m : scene->getMeshes())
			{
				VkDescriptorBufferInfo i = {};
				i.buffer = *m->getVertexBuffer();
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
			wds.dstBinding = BINDING_VERTEX_BUFFERS;
			wds.pBufferInfo = info.data();

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			std::vector<VkDescriptorBufferInfo> info;

			for (const auto& m : scene->getMeshes()) {
				VkDescriptorBufferInfo i = {};
				i.buffer = *m->getIndexBuffer();
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
			wds.dstBinding = BINDING_INDEX_BUFFERS;
			wds.pBufferInfo = info.data();

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			std::vector<VkDescriptorBufferInfo> info;

			for (const auto& s : scene->getSpheres()) {
				VkDescriptorBufferInfo i = {};
				i.buffer = *s->getBuffer();
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
			wds.dstBinding = BINDING_SPHERE_BUFFERS;
			wds.pBufferInfo = info.data();

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			std::vector<VkDescriptorBufferInfo> info;

			for (const auto& o : scene->getInstances()) {
				VkDescriptorBufferInfo i = {};
				i.buffer = *o->buffer;
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
			wds.dstBinding = BINDING_INSTANCE_BUFFERS;
			wds.pBufferInfo = info.data();

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			std::vector<VkDescriptorBufferInfo> info;

			for (const auto& m : scene->getMaterials()) {
				VkDescriptorBufferInfo i = {};
				i.buffer = *m->buffer;
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
			wds.dstBinding = BINDING_MATERIAL_BUFFERS;
			wds.pBufferInfo = info.data();

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			std::vector<VkDescriptorImageInfo> info;

			for (const auto& t : scene->getTextures()) {
				VkDescriptorImageInfo i = {};
				i.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				i.imageView = t->getImageView();
				i.sampler = t->getSampler();

				info.push_back(i);
			}

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = ds;
			wds.dstArrayElement = 0;
			wds.descriptorCount = (uint32_t) info.size();
			wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			wds.dstBinding = BINDING_TEXTURE_SAMPLERS;
			wds.pImageInfo = info.data();

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}

		{
			VkDescriptorBufferInfo info = {};
			info.buffer = *lightUniformBuffer;
			info.offset = 0;
			info.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = ds;
			wds.dstArrayElement = 0;
			wds.descriptorCount = 1;
			wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			wds.dstBinding = BINDING_LIGHT_BUFFER;
			wds.pBufferInfo = &info;

			vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
		}
	}
}

void Application::updateRaytracingRenderTarget() {

	device->imageBarrier(device->getCommandBuffer(), device->getBackBuffer(),
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