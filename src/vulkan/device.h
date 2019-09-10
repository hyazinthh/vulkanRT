#ifndef DEVICE_H_
#define DEVICE_H_

#include <vulkan/vulkan.hpp>
#include <optional>

#include "swap_chain.h"

class Instance;
typedef std::vector<std::string> Extensions;

class Device {
	public:
		Device(Instance* instance, int width, int height, VkSurfaceKHR surface, Extensions requiredExtensions);

		~Device();

		bool frameBegin();

		void beginRenderPass();

		void endRenderPass();

		void frameEnd();

		void framePresent();

		void bindDescriptorSet(VkDescriptorSet descriptorSet, VkPipelineLayout pipelineLayout, VkPipelineBindPoint bindPoint);

		void setClearColor(const VkClearColorValue& value) { clearColor = value; }

		void setClearDepthStencil(const VkClearDepthStencilValue& value) { clearDepthStencil = value; }

		VkDevice get() { return device; }

		VkPhysicalDevice getPhysical() { return physicalDevice; }

		VkSurfaceKHR getSurface() { return surface; }

		SwapChain* getSwapchain() { return swapchain; }

		Extensions getExtensions() const {
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

		VkDescriptorPool getDescriptorPool() {
			return descriptorPool;
		}

		VkRenderPass getRenderPass() {
			return renderPass;
		}

		VkCommandBuffer beginSingleTimeCommands();

		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	private:
		static const int MAX_FRAMES = 2;

		void createCommandPool();

		void createCommandBuffers();

		void createSyncObjects();

		void createRenderPass();

		void createFramebuffers();

		void createDescriptorPool();

		Extensions getExtensions(VkPhysicalDevice device) const;

		bool checkPhysicalDevice(VkPhysicalDevice device);

		std::optional<uint32_t> getQueueFamily(VkPhysicalDevice device) const;

		void createLogicalDevice(VkPhysicalDevice device, int width, int height);

		VkDevice device;

		VkPhysicalDevice physicalDevice;

		VkSurfaceKHR surface;

		SwapChain* swapchain;

		uint32_t queueFamily;

		VkQueue queue;

		Extensions requiredExtensions;

		VkCommandPool commandPool;

		VkCommandBuffer commandBuffers[MAX_FRAMES];

		VkRenderPass renderPass;

		std::vector<VkFramebuffer> framebuffers;

		VkDescriptorPool descriptorPool;

		uint32_t backBufferIndices[MAX_FRAMES];

		VkSemaphore imageAvailableSemaphores[MAX_FRAMES];

		VkSemaphore renderFinishedSemaphores[MAX_FRAMES];

		VkFence frameFences[MAX_FRAMES];

		VkClearColorValue clearColor;

		VkClearDepthStencilValue clearDepthStencil;

		int frameIndex = 0;
};

#endif