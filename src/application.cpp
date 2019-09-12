#include "application.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <chrono>

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
	createDescriptorSetLayout();
	createDescriptorSets();
	createPipeline();
	createShaderBindingTable();
}

Application::~Application() {

	vkDestroyDescriptorSetLayout(device->get(), descriptorSetLayout, nullptr);

	delete uniformBuffer;
	delete vertexBuffer;
	delete indexBuffer;

	delete sbt;
	delete pipeline;
	delete device;
	vkDestroySurfaceKHR(instance->get(), surface, nullptr);
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
			continue;
		}

		device->beginRenderPass();

		draw();

		device->endRenderPass();
		device->frameEnd();
		device->framePresent();
	}

	vkDeviceWaitIdle(device->get());
}

void Application::draw() {
	pipeline->bind(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV);
	vertexBuffer->bindAsVertexBuffer();
	indexBuffer->bindAsIndexBuffer(VK_INDEX_TYPE_UINT32);
	device->bindDescriptorSet(descriptorSet, pipeline->getLayout(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV);

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
	if (glfwCreateWindowSurface(instance->get(), window, nullptr, &surface) != VK_SUCCESS) {
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
	sbt = new ShaderBindingTable(device, pipeline);
	sbt->addEntry(ShaderBindingTable::EntryType::RayGen, rayGenIndex);
	sbt->addEntry(ShaderBindingTable::EntryType::Miss, missIndex);
	sbt->addEntry(ShaderBindingTable::EntryType::HitGroup, hitGroupIndex);

	sbt->create();
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

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(device->get(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Application::createDescriptorSets() {
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = device->getDescriptorPool();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(device->get(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer->get();
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet wds = {};
	wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	wds.dstSet = descriptorSet;
	wds.dstArrayElement = 0;
	wds.descriptorCount = 1;
	wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	wds.dstBinding = 0;
	wds.pBufferInfo = &bufferInfo;
	
	vkUpdateDescriptorSets(device->get(), 1, &wds, 0, nullptr);
}