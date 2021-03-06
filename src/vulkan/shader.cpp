#include "shader.h"
#include "device.h"

#include <shaderc/shaderc.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

class Includer : public shaderc::CompileOptions::IncluderInterface {

	private:
		struct Source {
			std::string path, content;
		};

	public:
		// Handles shaderc_include_resolver_fn callbacks.
		shaderc_include_result* GetInclude(const char* requested_source,
			shaderc_include_type type,
			const char* requesting_source,
			size_t include_depth) {

			auto rs = new shaderc_include_result();
			*rs = {};

			auto path = std::filesystem::path(requesting_source)
				.parent_path()
				.append(requested_source);

			std::ifstream f(path);
			if (!f.is_open()) {
				rs->source_name = "";
				rs->content = "File not found";
				return rs;
			}

			std::stringstream buffer;
			buffer << f.rdbuf();

			auto src = std::make_unique<Source>();
			src->path = std::filesystem::absolute(path).string();
			src->content = buffer.str();

			rs->source_name = src->path.c_str();
			rs->source_name_length = src->path.length();
			rs->content = src->content.c_str();
			rs->content_length = src->content.length();

			sources.push_back(std::move(src));
			return rs;
		}

		// Handles shaderc_include_result_release_fn callbacks.
		void ReleaseInclude(shaderc_include_result* data) {
			delete data;
		}

	private:

		std::vector<std::unique_ptr<Source>> sources;
};

Shader::Shader(Device* device, const std::string& name, const std::string& src, Type type) : device(device), type(type) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	std::unique_ptr<shaderc::CompileOptions::IncluderInterface> includer = std::make_unique<Includer>();

	options.SetIncluder(std::move(includer));
	auto spv = compile(name, prepare(name, src, compiler, options), compiler, options);

	VkShaderModuleCreateInfo moduleInfo = {};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.codeSize = spv.size() * sizeof(uint32_t);
	moduleInfo.pCode = spv.data();
	
	if (vkCreateShaderModule(*device, &moduleInfo, nullptr, &module) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module");
	}

	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.stage = getStageFlagBits();
	stageInfo.module = module;
	stageInfo.pName = "main";
}

Shader::~Shader() {
	vkDestroyShaderModule(*device, module, nullptr);
}

std::string Shader::prepare(const std::string& name, const std::string& src, shaderc::Compiler& compiler, shaderc::CompileOptions& options) {
	auto result = compiler.PreprocessGlsl(src, getKind(), name.c_str(), options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cout << result.GetErrorMessage() << std::endl;
		throw std::runtime_error("Failed to prepare shader");
	}

	return { result.begin(), result.end() };
}

std::vector<uint32_t> Shader::compile(const std::string& name, const std::string& src, shaderc::Compiler& compiler, shaderc::CompileOptions& options) {
	auto result = compiler.CompileGlslToSpv(src, getKind(), name.c_str(), "main", options);

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

	return new Shader(device, path, buffer.str(), type);
}