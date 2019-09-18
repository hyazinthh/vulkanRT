#pragma once

#include "device.h"

class Texture {

	public:

		Texture(Device* device, const std::string& fileName);

		~Texture();

		VkImage getImage() { 
			return image;
		}

		VkImageView getImageView() {
			return imageView;
		}

		VkSampler getSampler() {
			return sampler;
		}

	private:

		Device* device = nullptr;

		int width = 0;

		int height = 0;

		VkImage image = VK_NULL_HANDLE;

		VkDeviceMemory memory = VK_NULL_HANDLE;

		VkImageView imageView = VK_NULL_HANDLE;

		VkSampler sampler = VK_NULL_HANDLE;

};
