#ifndef SHADER_H_
#define SHADER_H_

#include <string>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

class Device;

class Shader {

	enum class Type {
		Vertex,
		Fragment
	};

	public:
		Shader(Device* device, const std::string& src, Type type);

		~Shader();

		VkShaderModule getModule() { return module; }

		VkPipelineShaderStageCreateInfo getStageInfo() { return stageInfo; }

	private:
	
		std::string prepare(const std::string& src, shaderc::Compiler& compiler, shaderc::CompileOptions& options);

		std::vector<uint32_t> compile(const std::string& src, shaderc::Compiler& compiler, shaderc::CompileOptions& options);

		shaderc_shader_kind getKind();

		VkShaderStageFlagBits getStageFlagBits();

		Device* device;

		Type type;

		VkShaderModule module;

		VkPipelineShaderStageCreateInfo stageInfo;
};

#endif