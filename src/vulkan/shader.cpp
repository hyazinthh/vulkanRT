#include "shader.h"
#include "device.h"

#include <shaderc/shaderc.hpp>
#include <iostream>

Shader::Shader(Device* device, const std::string& src, Type type) : device(device) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	auto spv = compile(prepare(src, compiler, options), compiler, options);

	VkShaderModuleCreateInfo moduleInfo = {};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.codeSize = spv.size();
	moduleInfo.pCode = spv.data();
	
	if (vkCreateShaderModule(device->get(), &moduleInfo, nullptr, &module) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module");
	}

	stageInfo = {};
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.stage = getStageFlagBits();
	stageInfo.module = module;
	stageInfo.pName = "main";

}

Shader::~Shader() {
	vkDestroyShaderModule(device->get(), module, nullptr);
}

std::string Shader::prepare(const std::string& src, shaderc::Compiler& compiler, shaderc::CompileOptions& options) {
	auto result = compiler.PreprocessGlsl(src, getKind(), "", options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cout << result.GetErrorMessage() << std::endl;
		throw std::runtime_error("Failed to prepare shader");
	}

	return { result.begin(), result.end() };
}

std::vector<uint32_t> Shader::compile(const std::string& src, shaderc::Compiler& compiler, shaderc::CompileOptions& options) {
	auto result = compiler.CompileGlslToSpv(src, getKind(), "", "main", options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cout << result.GetErrorMessage() << std::endl;
		throw std::runtime_error("Failed to compile shader");
	}

	return { result.begin(), result.end() };
}

shaderc_shader_kind Shader::getKind() {
	switch (type) {
		case Type::Vertex:
			return shaderc_vertex_shader;
		case Type::Fragment:
		default:
			return shaderc_fragment_shader;
	}
}

VkShaderStageFlagBits Shader::getStageFlagBits() {
	switch (type) {
		case Type::Vertex:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case Type::Fragment:
		default:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
	}
}