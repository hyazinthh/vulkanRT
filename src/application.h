#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#include "vulkan/instance.h"
#include "vulkan/device.h"
#include "vulkan/swap_chain.h"
#include "vulkan/buffer.h"
#include "vulkan/vertex.h"
#include "vulkan/rt/top_level_as.h"
#include "vulkan/rt/bottom_level_as.h"
#include "vulkan/rt/raytracing_pipeline.h"
#include "vulkan/rt/shader_binding_table.h"

class Application {
	public:

		Application(const std::string& name, uint32_t width, uint32_t height);

		~Application();

		void run();

		void update(float dt);
		
		void draw();

		const std::string& getName() const { return name; }

		uint32_t getWidth() const { return width; }

		uint32_t getHeight() const { return height; }

		GLFWwindow* getWindow() const { return window; }

		VkSurfaceKHR getSurface() const { return surface; }

		StringList getRequiredInstanceExtensions();

		StringList getRequiredDeviceExtensions();

		VkDescriptorSetLayoutCreateInfo getDescriptorSetLayoutInfo();

	private:

		void createWindow();

		void createInstance();

		void createDevice();

		void createSurface();

		void createPipeline();

		void createShaderBindingTable();

		void createAccelerationStructures();

		void createBuffers();

		void writeDescriptorSets();

		void updateRaytracingRenderTarget();

		void queryExtensions();

#ifndef NDEBUG
		bool debug = true;
#else
		bool debug = false;
#endif

		uint32_t width, height;

		std::string name;

		GLFWwindow* window = nullptr;

		Instance* instance = nullptr;

		Device* device = nullptr;

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		Pipeline* pipeline = nullptr;

		ShaderBindingTable* shaderBindingTable = nullptr;

		BottomLevelAS* bottomLevelAS = nullptr;

		TopLevelAS* topLevelAS = nullptr;

		Buffer* vertexBuffer = nullptr;

		Buffer* indexBuffer = nullptr;

		Buffer* uniformBuffer = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings;
};