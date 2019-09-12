#pragma once

#include "shader.h"
#include "swap_chain.h"
#include "buffer.h"

class Device;

class Pipeline {
	public:
		Pipeline(Device* device, Shader* vertexShader, Shader* fragmentShader, const VkDescriptorSetLayout& descriptorSetLayout);

		virtual ~Pipeline();

		void bind(VkPipelineBindPoint bindPoint);

		operator VkPipeline() { return pipeline; }

		VkPipelineLayout getLayout() { return layout; }

	protected:
		Pipeline(Device* device, const VkDescriptorSetLayout& descriptorSetLayout);

		Device* device = nullptr;

		VkPipelineLayout layout = VK_NULL_HANDLE;

		VkPipeline pipeline = VK_NULL_HANDLE;
};