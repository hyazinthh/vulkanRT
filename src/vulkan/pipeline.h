#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "shader.h"
#include "swap_chain.h"

class Device;

class Pipeline {
	public:
		Pipeline(Device* device, SwapChain* swapchain, Shader* vertexShader, Shader* fragmentShader);

		~Pipeline();

		VkPipeline get() { return pipeline; }

	private:
		Device* device;

		VkPipelineLayout layout;

		VkRenderPass renderPass;

		VkPipeline pipeline;
};

#endif // !PIPELINE_H_
