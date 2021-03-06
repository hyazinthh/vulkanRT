#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>

#include "swap_chain.h"
#include "pipeline.h"

class Instance;
typedef std::vector<std::string> StringList;

class Device {

	private:
		static const int MAX_FRAMES = 2;

	public:
		Device(Instance* instance, int width, int height, VkSurfaceKHR surface,
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo, StringList requiredExtensions);

		~Device();

		bool frameBegin();

		void beginRenderPass();

		void endRenderPass();

		void frameEnd();

		void framePresent();

		void setClearColor(const VkClearColorValue& value) { clearColor = value; }

		void setClearDepthStencil(const VkClearDepthStencilValue& value) { clearDepthStencil = value; }

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		void imageBarrier(VkCommandBuffer commandBuffer, VkImage image, VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout);

		operator VkDevice() { return device; }

		VkPhysicalDevice getPhysical() { return physicalDevice; }

		VkSurfaceKHR getSurface() { return surface; }

		SwapChain* getSwapchain() { return swapchain; }

		StringList getExtensions() const {
			return getExtensions(physicalDevice);
		}

		uint32_t getQueueFamily() const {
			return queueFamily;
		}

		VkQueue getQueue() {
			return queue;
		}

		VkCommandPool getCommandPool() {
			return commandPool;
		}

		VkCommandBuffer getCommandBuffer() {
			return commandBuffers[frameIndex];
		}

		VkImage getBackBuffer() {
			return swapchain->getImages()[backBufferIndices[frameIndex]];
		}

		VkImageView getBackBufferView() {
			return swapchain->getImageViews()[backBufferIndices[frameIndex]];
		}

		VkDescriptorPool getDescriptorPool() {
			return descriptorPool;
		}

		const VkDescriptorSetLayout& getDescriptorSetLayout() {
			return descriptorSetLayout;
		}

		VkDescriptorSet& getDescriptorSet() {
			return descriptorSets[frameIndex];
		}

		std::vector<VkDescriptorSet> getDescriptorSets() {
			return std::vector<VkDescriptorSet>(descriptorSets, descriptorSets + MAX_FRAMES);
		}

		VkRenderPass getRenderPass() {
			return renderPass;
		}

		VkCommandBuffer beginSingleTimeCommands();

		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	private:
		void createCommandPools();

		void createCommandBuffers();

		void createSyncObjects();

		void createRenderPass();

		void createFramebuffers();

		void createDescriptorPool();

		void createDescriptorSets();

		StringList getExtensions(VkPhysicalDevice device) const;

		bool checkPhysicalDevice(VkPhysicalDevice device);

		std::optional<uint32_t> getQueueFamily(VkPhysicalDevice device) const;

		void createLogicalDevice(VkPhysicalDevice device, int width, int height);

		VkDevice device = VK_NULL_HANDLE;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		SwapChain* swapchain = nullptr;

		uint32_t queueFamily;

		VkQueue queue = VK_NULL_HANDLE;

		StringList requiredExtensions;

		VkCommandPool commandPool = VK_NULL_HANDLE;

		VkCommandPool commandPoolSingle = VK_NULL_HANDLE;

		VkCommandBuffer commandBuffers[MAX_FRAMES] = { VK_NULL_HANDLE };

		VkRenderPass renderPass = VK_NULL_HANDLE;

		std::vector<VkFramebuffer> framebuffers;

		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};

		VkDescriptorSet descriptorSets[MAX_FRAMES] = { VK_NULL_HANDLE };

		uint32_t backBufferIndices[MAX_FRAMES];

		VkSemaphore imageAvailableSemaphores[MAX_FRAMES] = { VK_NULL_HANDLE };

		VkSemaphore renderFinishedSemaphores[MAX_FRAMES] = { VK_NULL_HANDLE };

		VkFence frameFences[MAX_FRAMES] = { VK_NULL_HANDLE };

		VkClearColorValue clearColor = { 1.0f, 0.0f, 0.0f, 1.0f };

		VkClearDepthStencilValue clearDepthStencil = {};

		int frameIndex = 0;
};