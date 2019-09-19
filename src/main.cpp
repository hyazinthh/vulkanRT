#include <iostream>
#include "application.h"

int main() {
	try {
		Application app("Vulkan Raytracer", 1600, 900);
		app.run();
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}