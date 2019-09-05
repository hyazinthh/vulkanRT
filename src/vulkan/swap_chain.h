#ifndef SWAP_CHAIN_H_
#define SWAP_CHAIN_H_

#include <vector>
#include <vulkan/vulkan.h>

class Device;

class SwapChain {
	public:
		SwapChain(Device* device, VkExtent2D extent, VkFormat format, VkColorSpaceKHR colorSpace, VkPresentModeKHR mode);

		~SwapChain();

		VkSwapchainKHR get() { return swapchain; }

		const std::vector<VkImage>& getImages() { return images; }

		const std::vector<VkImageView>& getImageViews() { return imageViews; }

	private:
		Device* device;

		VkSwapchainKHR swapchain;

		std::vector<VkImage> images;

		std::vector<VkImageView> imageViews;

		VkFormat format;

		VkExtent2D extent;
};

#endif
