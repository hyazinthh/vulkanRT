#ifndef APPLICATION_H_
#define APPLICATION_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#include "vulkan/instance.h"
#include "vulkan/device.h"
#include "vulkan/swap_chain.h"
#include "vulkan/pipeline.h"

class Application {
	public:

		Application(const std::string& name, uint32_t width, uint32_t height);

		~Application();

		void run();
		
		void draw();

		const std::string& getName() const { return name; }

		uint32_t getWidth() const { return width; }

		uint32_t getHeight() const { return height; }

		GLFWwindow* getWindow() const { return window; }

		VkSurfaceKHR getSurface() const { return surface; }

	private:

		void createWindow();

		void createInstance();

		void createDevice();

		void createSurface();

		void createSwapchain();

		void createPipeline();

		void createSemaphores();

		void queryExtensions();

		Extensions getRequiredExtensions(bool debug);

		uint32_t width, height;

		std::string name;

		GLFWwindow* window = nullptr;

		Instance* instance = nullptr;

		Device* device = nullptr;

		VkSurfaceKHR surface;

		SwapChain* swapchain = nullptr;

		Pipeline* pipeline = nullptr;

		VkSemaphore imageAvailableSemaphore;

		VkSemaphore renderFinishedSemaphore;
};

#endif