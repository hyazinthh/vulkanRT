#include "application.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <chrono>

#include "vulkan/extensions.h"

const std::vector<Vertex> vertices = {
	{{-0.75f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	{{0.75f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
};

const std::vector<uint32_t> indices = {
	0, 1, 2, 0, 3, 1
};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

uint32_t rayGenIndex;

uint32_t missIndex;

uint32_t hitGroupIndex;

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
	createBuffers();
	createAccelerationStructures();
	createDescriptorSetLayout();
	createDescriptorSets();
	createPipeline();
	createShaderBindingTable();
}

Application::~Application() {

	vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);

	delete topLevelAS;
	delete bottomLevelAS;

	delete uniformBuffer;
	delete vertexBuffer;
	delete indexBuffer;

	delete shaderBindingTable;
	delete pipeline;
	delete device;
	vkDestroySurfaceKHR(*instance, surface, nullptr);
	delete instance;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::update(float dt) {

	auto ext = device->getSwapchain()->getExtent();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), ext.width / (float) ext.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

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

		draw();

		device->endRenderPass();
		device->frameEnd();
		device->framePresent();
	}

	vkDeviceWaitIdle(*device);
}

void Application::draw() {
	pipeline->bind(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV);
	vertexBuffer->bindAsVertexBuffer();
	indexBuffer->bindAsIndexBuffer(VK_INDEX_TYPE_UINT32);
	device->bindDescriptorSet(descriptorSet, pipeline->getLayout(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV);

	auto rg = ShaderBindingTable::EntryType::RayGen;
	auto miss = ShaderBindingTable::EntryType::Miss;
	auto hitGroup = ShaderBindingTable::EntryType::HitGroup;

	VkExt::vkCmdTraceRaysNV(device->getCommandBuffer(),
		*shaderBindingTable->getBuffer(), shaderBindingTable->getOffset(rg),
		*shaderBindingTable->getBuffer(), shaderBindingTable->getOffset(miss), shaderBindingTable->getEntrySize(miss),
		*shaderBindingTable->getBuffer(), shaderBindingTable->getOffset(hitGroup), shaderBindingTable->getEntrySize(hitGroup),
		VK_NULL_HANDLE, 0, 0, width, height, 1);

	//uint32_t count = (uint32_t) indices.size();
	//vkCmdDrawIndexed(device->getCommandBuffer(), count, count / 3, 0, 0, 0);
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
	if (glfwCreateWindowSurface(*instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}
}

void Application::createPipeline() {

	std::unique_ptr<Shader> shaderMiss(Shader::loadFromFile(device, "shaders/ray_miss.rmiss", Shader::Type::Miss));
	std::unique_ptr<Shader> shaderClosestHit(Shader::loadFromFile(device, "shaders/ray_chit.rchit", Shader::Type::ClosestHit));
	std::unique_ptr<Shader> shaderRayGen(Shader::loadFromFile(device, "shaders/ray_gen.rgen", Shader::Type::RayGen));

	RaytracingPipeline* pipeline = new RaytracingPipeline(device, descriptorSetLayout);

	rayGenIndex = pipeline->addShaderStage(shaderRayGen.get());
	missIndex = pipeline->addShaderStage(shaderMiss.get());

	hitGroupIndex = pipeline->startHitGroup();
	pipeline->addHitShaderStage(shaderClosestHit.get());
	pipeline->endHitGroup();

	this->pipeline = pipeline->create();
}

void Application::createShaderBindingTable() {
	shaderBindingTable = new ShaderBindingTable(device, pipeline);
	shaderBindingTable->addEntry(ShaderBindingTable::EntryType::RayGen, rayGenIndex);
	shaderBindingTable->addEntry(ShaderBindingTable::EntryType::Miss, missIndex);
	shaderBindingTable->addEntry(ShaderBindingTable::EntryType::HitGroup, hitGroupIndex);

	shaderBindingTable->create();
}

void Application::createAccelerationStructures() {
	bottomLevelAS = AccelerationStructure::createBottomLevel(device, 
						vertexBuffer, (uint32_t) vertices.size(), sizeof(Vertex),
						indexBuffer, (uint32_t) indices.size());

	std::vector<AccelerationStructure::Instance> instances;
	instances.push_back(AccelerationStructure::Instance(bottomLevelAS, 0, 0));

	topLevelAS = AccelerationStructure::createTopLevel(device, instances);
}

void Application::createBuffers() {
	{
		Buffer localBuffer(device, sizeof(Vertex) * vertices.size(),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		vertexBuffer = new Buffer(device, localBuffer.getSize(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		localBuffer.fill(vertices.data());
		localBuffer.copyTo(vertexBuffer);
	}

	{
		Buffer localBuffer(device, sizeof(uint32_t) * indices.size(),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		indexBuffer = new Buffer(device, localBuffer.getSize(),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		localBuffer.fill(indices.data());
		localBuffer.copyTo(indexBuffer);
	}

	{
		uniformBuffer = new Buffer(device, sizeof(UniformBufferObject),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
}

void Application::createDescriptorSetLayout() {

	VkDescriptorSetLayoutBinding accelerationStructureBinding = {};
	accelerationStructureBinding.binding = 0;
	accelerationStructureBinding.descriptorCount = 1;
	accelerationStructureBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
	accelerationStructureBinding.pImmutableSamplers = nullptr;
	accelerationStructureBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

	VkDescriptorSetLayoutBinding imageBufferBinding = {};
	imageBufferBinding.binding = 1;
	imageBufferBinding.descriptorCount = 1;
	imageBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	imageBufferBinding.pImmutableSamplers = nullptr;
	imageBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

	VkDescriptorSetLayoutBinding uboBinding = {};
	uboBinding.binding = 2;
	uboBinding.descriptorCount = 1;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.pImmutableSamplers = nullptr;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { accelerationStructureBinding, imageBufferBinding, uboBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (uint32_t) bindings.size();
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Application::createDescriptorSets() {
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = device->getDescriptorPool();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(*device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	std::vector<VkWriteDescriptorSet> writeDescriptorSets;

	{
		VkAccelerationStructureNV as = *topLevelAS;

		VkWriteDescriptorSetAccelerationStructureNV wdsAccelerationStructure = {};
		wdsAccelerationStructure.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
		wdsAccelerationStructure.accelerationStructureCount = 1;
		wdsAccelerationStructure.pAccelerationStructures = &as;

		VkWriteDescriptorSet wds = {};
		wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.pNext = &wdsAccelerationStructure;
		wds.dstSet = descriptorSet;
		wds.dstBinding = 0;
		wds.descriptorCount = 1;
		wds.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

		writeDescriptorSets.push_back(wds);
	}

	{
		VkDescriptorBufferInfo info = {};
		info.buffer = *uniformBuffer;
		info.offset = 0;
		info.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet wds = {};
		wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		wds.dstSet = descriptorSet;
		wds.dstArrayElement = 0;
		wds.descriptorCount = 1;
		wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		wds.dstBinding = 2;
		wds.pBufferInfo = &info;

		writeDescriptorSets.push_back(wds);
	}

	vkUpdateDescriptorSets(*device, (uint32_t) writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
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
	wds.dstSet = descriptorSet;
	wds.dstArrayElement = 0;
	wds.descriptorCount = 1;
	wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	wds.dstBinding = 1;
	wds.pImageInfo = &info;

	vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
}