#include <iostream>
#include "application.h"

int main() {
	try {
		Application app("Vulkan Raytracer", 800, 600);
		app.run();
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}