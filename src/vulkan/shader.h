#ifndef SHADER_H_
#define SHADER_H_

#include <string>
#include <vulkan/vulkan.h>

#define NV_EXTENSIONS
#include <shaderc/shaderc.hpp>

class Device;

class Shader {

	public:
		enum class Type {
			Vertex,
			Fragment,
			RayGen,
			AnyHit,
			ClosestHit,
			Miss,
			Intersection,
			Callable
		};

		Shader(Device* device, const std::string& src, Type type);

		~Shader();

		VkShaderModule getModule() { return module; }

		VkPipelineShaderStageCreateInfo getStageInfo() { return stageInfo; }

		static Shader* loadFromFile(Device* device, const std::string& path, Type type);

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