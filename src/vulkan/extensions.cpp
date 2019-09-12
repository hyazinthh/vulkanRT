#include "extensions.h"

PFN_vkCreateDebugUtilsMessengerEXT VkExt::vkCreateDebugUtilsMessengerEXT = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT VkExt::vkDestroyDebugUtilsMessengerEXT = nullptr;
PFN_vkCreateAccelerationStructureNV VkExt::vkCreateAccelerationStructureNV = nullptr;
PFN_vkDestroyAccelerationStructureNV VkExt::vkDestroyAccelerationStructureNV = nullptr;
PFN_vkGetAccelerationStructureMemoryRequirementsNV VkExt::vkGetAccelerationStructureMemoryRequirementsNV = nullptr;
PFN_vkBindAccelerationStructureMemoryNV VkExt::vkBindAccelerationStructureMemoryNV = nullptr;
PFN_vkCmdBuildAccelerationStructureNV VkExt::vkCmdBuildAccelerationStructureNV = nullptr;
PFN_vkCmdCopyAccelerationStructureNV  VkExt::vkCmdCopyAccelerationStructureNV = nullptr;
PFN_vkCmdTraceRaysNV VkExt::vkCmdTraceRaysNV = nullptr;
PFN_vkCreateRayTracingPipelinesNV VkExt::vkCreateRayTracingPipelinesNV = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesNV VkExt::vkGetRayTracingShaderGroupHandlesNV = nullptr;
PFN_vkGetAccelerationStructureHandleNV VkExt::vkGetAccelerationStructureHandleNV = nullptr;
PFN_vkCmdWriteAccelerationStructuresPropertiesNV VkExt::vkCmdWriteAccelerationStructuresPropertiesNV = nullptr;
PFN_vkCompileDeferredNV VkExt::vkCompileDeferredNV = nullptr;

VkResult VkExt::initProcs(Instance* instance) {
	if (instance->isDebug()) {
		if ((vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT")) == nullptr) {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		if ((vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT")) == nullptr) {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	return VK_SUCCESS;
}

VkResult VkExt::initDeviceProcs(Device* device) {
	if ((vkCreateAccelerationStructureNV = (PFN_vkCreateAccelerationStructureNV) vkGetDeviceProcAddr(*device, "vkCreateAccelerationStructureNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkDestroyAccelerationStructureNV = (PFN_vkDestroyAccelerationStructureNV) vkGetDeviceProcAddr(*device, "vkDestroyAccelerationStructureNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkGetAccelerationStructureMemoryRequirementsNV = (PFN_vkGetAccelerationStructureMemoryRequirementsNV) vkGetDeviceProcAddr(*device, "vkGetAccelerationStructureMemoryRequirementsNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkBindAccelerationStructureMemoryNV = (PFN_vkBindAccelerationStructureMemoryNV) vkGetDeviceProcAddr(*device, "vkBindAccelerationStructureMemoryNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkCmdBuildAccelerationStructureNV = (PFN_vkCmdBuildAccelerationStructureNV) vkGetDeviceProcAddr(*device, "vkCmdBuildAccelerationStructureNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkCmdCopyAccelerationStructureNV = (PFN_vkCmdCopyAccelerationStructureNV) vkGetDeviceProcAddr(*device, "vkCmdCopyAccelerationStructureNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkCmdTraceRaysNV = (PFN_vkCmdTraceRaysNV) vkGetDeviceProcAddr(*device, "vkCmdTraceRaysNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkCreateRayTracingPipelinesNV = (PFN_vkCreateRayTracingPipelinesNV) vkGetDeviceProcAddr(*device, "vkCreateRayTracingPipelinesNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkGetRayTracingShaderGroupHandlesNV = (PFN_vkGetRayTracingShaderGroupHandlesNV) vkGetDeviceProcAddr(*device, "vkGetRayTracingShaderGroupHandlesNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkGetAccelerationStructureHandleNV = (PFN_vkGetAccelerationStructureHandleNV) vkGetDeviceProcAddr(*device, "vkGetAccelerationStructureHandleNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkCmdWriteAccelerationStructuresPropertiesNV = (PFN_vkCmdWriteAccelerationStructuresPropertiesNV) vkGetDeviceProcAddr(*device, "vkCmdWriteAccelerationStructuresPropertiesNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if ((vkCompileDeferredNV = (PFN_vkCompileDeferredNV) vkGetDeviceProcAddr(*device, "vkCompileDeferredNV")) == nullptr) {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	return VK_SUCCESS;
}