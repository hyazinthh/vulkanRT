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
	Vertex({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}),
	Vertex({0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}),
	Vertex({-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}),
	Vertex({0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f})
};

const std::vector<uint32_t> indices = {
	0, 1, 2, 0, 3, 1
};

struct UniformBufferObject {
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
};

uint32_t rayGenIndex;

uint32_t missIndex;

uint32_t hitGroupIndex;

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
	createAccelerationStructures();
	writeDescriptorSets();
	createPipeline();
	createShaderBindingTable();
}

Application::~Application() {

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

	// Update acceleration structure
	auto model = glm::rotate(glm::mat4(1.0f), dt * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	std::vector<TopLevelAS::Instance> instances;
	instances.push_back(TopLevelAS::Instance(bottomLevelAS, 0, 0, model));

	topLevelAS->update(instances);

	// Update matrices
	auto ext = device->getSwapchain()->getExtent();

	UniformBufferObject ubo = {};
	ubo.viewInverse = glm::inverse(glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
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

		draw();

		device->endRenderPass();
		device->frameEnd();
		device->framePresent();
	}

	vkDeviceWaitIdle(*device);
}

void Application::draw() {
	pipeline->bind(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV);
	//vertexBuffer->bindAsVertexBuffer();
	//indexBuffer->bindAsIndexBuffer(VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(device->getCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline->getLayout(),
		0, 1, &device->getDescriptorSet(), 0, nullptr);

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

void Application::createPipeline() {

	std::unique_ptr<Shader> shaderMiss(Shader::loadFromFile(device, "shaders/ray_miss.rmiss", Shader::Type::Miss));
	std::unique_ptr<Shader> shaderClosestHit(Shader::loadFromFile(device, "shaders/ray_chit.rchit", Shader::Type::ClosestHit));
	std::unique_ptr<Shader> shaderRayGen(Shader::loadFromFile(device, "shaders/ray_gen.rgen", Shader::Type::RayGen));

	RaytracingPipeline* pipeline = new RaytracingPipeline(device);

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
	bottomLevelAS = new BottomLevelAS(device, 
						vertexBuffer, (uint32_t) vertices.size(), sizeof(Vertex),
						indexBuffer, (uint32_t) indices.size());

	std::vector<TopLevelAS::Instance> instances;
	instances.push_back(TopLevelAS::Instance(bottomLevelAS, 0, 0));

	topLevelAS = new TopLevelAS(device, instances, true);
}

void Application::createBuffers() {
	{
		Buffer localBuffer(device, sizeof(Vertex) * vertices.size(),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		vertexBuffer = new Buffer(device, localBuffer.getSize(),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		localBuffer.fill(vertices.data());
		localBuffer.copyTo(vertexBuffer);
	}

	{
		Buffer localBuffer(device, sizeof(uint32_t) * indices.size(),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		indexBuffer = new Buffer(device, localBuffer.getSize(),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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
		b.descriptorCount = 1;
		b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		b.pImmutableSamplers = nullptr;
		b.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		bindings.push_back(b);
	}

	// Index buffer
	{
		VkDescriptorSetLayoutBinding b = {};
		b.binding = 4;
		b.descriptorCount = 1;
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
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;

	for (const auto& ds : device->getDescriptorSets()) {
		{
			VkAccelerationStructureNV as = *topLevelAS;

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

			writeDescriptorSets.push_back(wds);
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

			writeDescriptorSets.push_back(wds);
		}

		{
			VkDescriptorBufferInfo info = {};
			info.buffer = *vertexBuffer;
			info.offset = 0;
			info.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = ds;
			wds.dstArrayElement = 0;
			wds.descriptorCount = 1;
			wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			wds.dstBinding = 3;
			wds.pBufferInfo = &info;

			writeDescriptorSets.push_back(wds);
		}

		{
			VkDescriptorBufferInfo info = {};
			info.buffer = *indexBuffer;
			info.offset = 0;
			info.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = ds;
			wds.dstArrayElement = 0;
			wds.descriptorCount = 1;
			wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			wds.dstBinding = 4;
			wds.pBufferInfo = &info;

			writeDescriptorSets.push_back(wds);
		}
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
	wds.dstSet = device->getDescriptorSet();
	wds.dstArrayElement = 0;
	wds.descriptorCount = 1;
	wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	wds.dstBinding = 1;
	wds.pImageInfo = &info;

	vkUpdateDescriptorSets(*device, 1, &wds, 0, nullptr);
}