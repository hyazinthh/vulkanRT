#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "shader.h"
#include "swap_chain.h"
#include "buffer.h"

class Device;

class Pipeline {
	public:
		Pipeline(Device* device, Shader* vertexShader, Shader* fragmentShader, const VkDescriptorSetLayout& descriptorSetLayout);

		~Pipeline();

		void bind(VkPipelineBindPoint bindPoint);

		VkPipeline get() { return pipeline; }

		VkPipelineLayout getLayout() { return layout; }

	private:
		Device* device;

		VkPipelineLayout layout;

		VkPipeline pipeline;
};

#endif // !PIPELINE_H_
