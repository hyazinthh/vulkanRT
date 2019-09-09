#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "shader.h"
#include "swap_chain.h"
#include "buffer.h"

class Device;

class Pipeline {
	public:
		Pipeline(Device* device, SwapChain* swapchain, Buffer* vertexBuffer, Buffer* indexBuffer, Shader* vertexShader, Shader* fragmentShader);

		~Pipeline();

		VkPipeline get() { return pipeline; }

		const std::vector<VkCommandBuffer>& getCommandBuffers() { return commandBuffers;  }

	private:
		Device* device;

		VkPipelineLayout layout;

		VkRenderPass renderPass;

		VkPipeline pipeline;

		std::vector<VkFramebuffer> framebuffers;

		VkCommandPool commandPool;

		std::vector<VkCommandBuffer> commandBuffers;
};

#endif // !PIPELINE_H_
