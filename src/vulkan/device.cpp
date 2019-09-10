#include "device.h"
#include "instance.h"
#include "extensions.h"

#include <set>

Device::Device(Instance* instance, int width, int height, VkSurfaceKHR surface, Extensions requiredExtensions)
	: surface(surface), requiredExtensions(requiredExtensions) {
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(instance->get(), &count, nullptr);

	std::vector<VkPhysicalDevice> devices(count);
	vkEnumeratePhysicalDevices(instance->get(), &count, devices.data());

	for (auto& d : devices) {
		if (checkPhysicalDevice(d)) {
			createLogicalDevice(d, width, height);
			return;
		}
	}
	
	throw std::runtime_error("No suitable Vulkan device found");
}

Device::~Device() {
	vkDeviceWaitIdle(device);

	delete swapchain;

	for (auto f : framebuffers) {
		vkDestroyFramebuffer(device, f, nullptr);
	}

	vkDestroyRenderPass(device, renderPass, nullptr);

	for (int i = 0; i < MAX_FRAMES; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, frameFences[i], nullptr);
	}

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDevice(device, nullptr);
}

bool Device::frameBegin() {

	vkWaitForFences(device, 1, &frameFences[frameIndex], VK_TRUE, UINT64_MAX);

	if (vkAcquireNextImageKHR(device, swapchain->get(), UINT64_MAX,
						      imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE,
							  &backBufferIndices[frameIndex]) != VK_SUCCESS) {
		return false;
	}

	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffers[frameIndex], &info);

	return true;
}

void Device::beginRenderPass() {
	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = renderPass;
	info.framebuffer = framebuffers[backBufferIndices[frameIndex]];
	info.renderArea.extent.width = swapchain->getExtent().width;
	info.renderArea.extent.height = swapchain->getExtent().height;
	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = clearColor;
	clearValues[1].depthStencil = clearDepthStencil;

	info.clearValueCount = static_cast<uint32_t>(clearValues.size());
	info.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(commandBuffers[frameIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
}

void Device::endRenderPass() {
	vkCmdEndRenderPass(commandBuffers[frameIndex]);
}

void Device::frameEnd() {
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &imageAvailableSemaphores[frameIndex];
	info.pWaitDstStageMask = waitStages;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &commandBuffers[frameIndex];
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = &renderFinishedSemaphores[frameIndex];

	vkEndCommandBuffer(commandBuffers[frameIndex]);
	vkResetFences(device, 1, &frameFences[frameIndex]);
	vkQueueSubmit(queue, 1, &info, frameFences[frameIndex]);
}

void Device::framePresent() {
	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &renderFinishedSemaphores[frameIndex];
	info.swapchainCount = 1;
	info.pSwapchains = &swapchain->get();
	info.pImageIndices = &backBufferIndices[frameIndex];
	vkQueuePresentKHR(queue, &info);

	frameIndex = (frameIndex + 1) % MAX_FRAMES;
}

void Device::bindDescriptorSet(VkDescriptorSet descriptorSet, VkPipelineLayout pipelineLayout, VkPipelineBindPoint bindPoint) {
	vkCmdBindDescriptorSets(getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
}

Extensions Device::getExtensions(VkPhysicalDevice device) const {
	uint32_t count = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

	std::vector<VkExtensionProperties> props(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, props.data());

	Extensions ext;
	std::transform(props.begin(), props.end(), std::back_inserter(ext), [](VkExtensionProperties& p) {
		return p.extensionName;
	});

	return ext;
}

void Device::createLogicalDevice(VkPhysicalDevice physicalDevice, int width, int height) {
	this->physicalDevice = physicalDevice;
	queueFamily = getQueueFamily(physicalDevice).value();

	// Queues
	VkDeviceQueueCreateInfo queueInfo = {};
	float priority = 1.0f;
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueFamilyIndex = queueFamily;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &priority;

	// Features
	VkPhysicalDeviceFeatures deviceFeatures = {};

	// Extensions
	std::vector<const char*> ext;
	ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	ext.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);

	std::transform(requiredExtensions.begin(), requiredExtensions.end(), std::back_inserter(ext), [](auto& e) {
		return e.c_str();
	});

	// Create device
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pEnabledFeatures = &deviceFeatures;
	deviceInfo.enabledExtensionCount = (uint32_t) ext.size();
	deviceInfo.ppEnabledExtensionNames = ext.data();

	if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan device");
	}

	// Get queue
	vkGetDeviceQueue(device, queueFamily, 0, &queue);

	if (VkExt::initDeviceProcs(this) != VK_SUCCESS) {
		throw std::runtime_error("Failed to setup Vulkan extension procs");
	}

	// Pools
	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
	createDescriptorPool();

	// Swapchain
	VkExtent2D extent = { (uint32_t) width, (uint32_t) height };
	swapchain = new SwapChain(this, surface, extent, VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, VK_PRESENT_MODE_FIFO_KHR);

	createRenderPass();
	createFramebuffers();
}

void Device::createCommandPool() {
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}
}

void Device::createCommandBuffers() {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = MAX_FRAMES;

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers");
	}
}

void Device::createSyncObjects() {

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &frameFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create synchronization objects");
		}
	}
}

void Device::createRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchain->getFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	// Subpass dependency
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}
}

void Device::createFramebuffers() {
	framebuffers.resize(swapchain->getImageViews().size());

	for (size_t i = 0; i < swapchain->getImageViews().size(); i++) {

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &swapchain->getImageViews()[i];
		framebufferInfo.width = swapchain->getExtent().width;
		framebufferInfo.height = swapchain->getExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer");
		}
	}
}

void Device::createDescriptorPool() {
	// Descriptor pool
	VkDescriptorPoolSize poolSize[] = {
		//{VK_DESCRIPTOR_TYPE_SAMPLER, 0},
		//{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		//{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0},
		//{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0},
		//{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0},
		//{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 0},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32},
		{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 32}
		//{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		//{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0},
		//{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 0},
		//{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0}
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 32;
	poolInfo.poolSizeCount = _countof(poolSize);
	poolInfo.pPoolSizes = poolSize;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

bool Device::checkPhysicalDevice(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);

	auto ext = getExtensions(device);
	for (auto e : requiredExtensions) {
		if (std::find(ext.begin(), ext.end(), e) == ext.end()) {
			return false;
		}
	}

	return (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && getQueueFamily(device).has_value();
}

std::optional<uint32_t> Device::getQueueFamily(VkPhysicalDevice device) const {
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

	std::vector<VkQueueFamilyProperties> props(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());

	for (uint32_t i = 0; i < count; i++) {
		if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
			continue;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			return i;
		}
	}

	return {};
}

VkCommandBuffer Device::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Device::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i))
			&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}
