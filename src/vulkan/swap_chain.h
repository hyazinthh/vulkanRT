#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Device;

class SwapChain {
	public:
		SwapChain(Device* device, VkSurfaceKHR surface, VkExtent2D extent, VkFormat format, VkColorSpaceKHR colorSpace, VkPresentModeKHR mode);

		~SwapChain();

		const VkSwapchainKHR& get() { return swapchain; }

		const VkFormat& getFormat() { return format; }

		const VkExtent2D& getExtent() { return extent; }

		const std::vector<VkImage>& getImages() { return images; }

		const std::vector<VkImageView>& getImageViews() { return imageViews; }

	private:
		Device* device = nullptr;

		VkSwapchainKHR swapchain = VK_NULL_HANDLE;

		std::vector<VkImage> images;

		std::vector<VkImageView> imageViews;

		VkFormat format = VK_FORMAT_UNDEFINED;

		VkExtent2D extent = {};
};