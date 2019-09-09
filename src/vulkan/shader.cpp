#include "shader.h"
#include "device.h"

#include <shaderc/shaderc.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(Device* device, const std::string& src, Type type) : device(device), type(type) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	auto spv = compile(prepare(src, compiler, options), compiler, options);

	VkShaderModuleCreateInfo moduleInfo = {};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.codeSize = spv.size() * sizeof(uint32_t);
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
		std::cout << "shaderc" << result.GetErrorMessage() << std::endl;
		throw std::runtime_error("Failed to prepare shader");
	}

	return { result.begin(), result.end() };
}

std::vector<uint32_t> Shader::compile(const std::string& src, shaderc::Compiler& compiler, shaderc::CompileOptions& options) {
	auto result = compiler.CompileGlslToSpv(src, getKind(), "", "main", options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cout << "shaderc" << result.GetErrorMessage() << std::endl;
		throw std::runtime_error("Failed to compile shader");
	}

	return { result.begin(), result.end() };
}

shaderc_shader_kind Shader::getKind() {
	switch (type) {
		case Type::Vertex:
			return shaderc_vertex_shader;
		case Type::Fragment:
			return shaderc_fragment_shader;
		case Type::RayGen:
			return shaderc_raygen_shader;
		case Type::AnyHit:
			return shaderc_anyhit_shader;
		case Type::ClosestHit:
			return shaderc_closesthit_shader;
		case Type::Miss:
			return shaderc_miss_shader;
		case Type::Intersection:
			return shaderc_intersection_shader;
		case Type::Callable:
		default:
			return shaderc_callable_shader;
	}
}

VkShaderStageFlagBits Shader::getStageFlagBits() {
	switch (type) {
		case Type::Vertex:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case Type::Fragment:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case Type::RayGen:
			return VK_SHADER_STAGE_RAYGEN_BIT_NV;
		case Type::AnyHit:
			return VK_SHADER_STAGE_ANY_HIT_BIT_NV;
		case Type::ClosestHit:
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		case Type::Miss:
			return VK_SHADER_STAGE_MISS_BIT_NV;
		case Type::Intersection:
			return VK_SHADER_STAGE_INTERSECTION_BIT_NV;
		case Type::Callable:
		default:
			return VK_SHADER_STAGE_CALLABLE_BIT_NV;
	}
}

Shader* Shader::loadFromFile(Device* device, const std::string& path, Type type) {
	std::ifstream f(path);
	if (!f.is_open()) {
		throw std::runtime_error(std::string("Failed to open '") + path + "'");
	}

	std::stringstream buffer;
	buffer << f.rdbuf();

	return new Shader(device, buffer.str(), type);
}