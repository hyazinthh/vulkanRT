#include "swap_chain.h"
#include "device.h"

SwapChain::SwapChain(Device* device, VkSurfaceKHR surface, VkExtent2D extent, VkFormat format, VkColorSpaceKHR colorSpace, VkPresentModeKHR mode)
	: format(format), device(device), extent(extent) {

	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->getPhysical(), surface, &capabilities);

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.clipped = VK_TRUE;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.surface = device->getSurface();
	info.minImageCount = capabilities.minImageCount + 1;
	info.imageFormat = format;
	info.imageColorSpace = colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	info.preTransform = capabilities.currentTransform;
	info.presentMode = mode;
	info.oldSwapchain = VK_NULL_HANDLE;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateSwapchainKHR(*device, &info, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain");
	}

	// Images
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, nullptr);

	images.resize(imageCount);
	vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, images.data());

	// Images views
	imageViews.resize(images.size());

	for (uint32_t i = 0; i < images.size(); i++) {
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = images[i];
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = format;
		info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(*device, &info, nullptr, &imageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

SwapChain::~SwapChain() {
	for (auto& v : imageViews) {
		vkDestroyImageView(*device, v, nullptr);
	}

	vkDestroySwapchainKHR(*device, swapchain, nullptr);
}