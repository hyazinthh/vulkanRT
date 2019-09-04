#include "swap_chain.h"
#include "device.h"

SwapChain::SwapChain(Device* device, VkExtent2D extent, VkFormat format, VkColorSpaceKHR colorSpace, VkPresentModeKHR mode)
	: device(device) {

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.clipped = VK_TRUE;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.surface = device->getSurface();
	info.minImageCount = 1;
	info.imageFormat = format;
	info.imageColorSpace = colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.presentMode = mode;
	info.oldSwapchain = VK_NULL_HANDLE;

	QueueFamilies families = device->getQueueFamilies();
	uint32_t indices[] = { families.graphics.value(), families.present.value() };

	if (families.graphics != families.present) {
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = indices;
	} else {
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateSwapchainKHR(device->get(), &info, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain");
	}

	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(device->get(), swapchain, &imageCount, nullptr);

	images.resize(imageCount);
	vkGetSwapchainImagesKHR(device->get(), swapchain, &imageCount, images.data());
}

SwapChain::~SwapChain() {
	vkDestroySwapchainKHR(device->get(), swapchain, nullptr);
}