#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "shader.h"
#include "swap_chain.h"
#include "buffer.h"

class Device;

class Pipeline {
	public:
		Pipeline(Device* device, Shader* vertexShader, Shader* fragmentShader);

		~Pipeline();

		void bind(VkPipelineBindPoint bindPoint);

		VkPipeline get() { return pipeline; }

	private:
		Device* device;

		VkPipelineLayout layout;

		VkPipeline pipeline;
};

#endif // !PIPELINE_H_
