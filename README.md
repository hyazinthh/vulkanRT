# vulkanRT
Playing around with Vulkan and ray tracing

## Vulkan SDK
Requires the Vulkan SDK to build, $(VULKAN_SDK) should point to the root directory (containing Bin, Include, etc.)

## shaderc
We need shaderc to compile GLSL to SPIR-V. The Vulkan SDK comes with shaderc but only provides a release library. The debug libraries have to be built manually using CMake which generates a Visual studio solution (make sure to set x64). Build the \*\_combined projects and move shaderc_combined.lib to  $(VULKAN_SDK)/Lib/shaderc_combined_debug.lib.

## Resources
Based on the NVIDIA raytracing example (https://developer.nvidia.com/rtx/raytracing/vkray) by Martin-Karl Lefrançois and Pascal Gautron.

Textures taken from https://www.textures.com.
