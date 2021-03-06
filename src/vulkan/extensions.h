#pragma once

#include "instance.h"
#include "device.h"

namespace VkExt {
	extern PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
	extern PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
	extern PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
	extern PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
	extern PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
	extern PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
	extern PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
	extern PFN_vkCmdCopyAccelerationStructureNV  vkCmdCopyAccelerationStructureNV;
	extern PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;
	extern PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
	extern PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
	extern PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
	extern PFN_vkCmdWriteAccelerationStructuresPropertiesNV vkCmdWriteAccelerationStructuresPropertiesNV;
	extern PFN_vkCompileDeferredNV vkCompileDeferredNV;

	VkResult initProcs(Instance* instance);
	VkResult initDeviceProcs(Device* device);
}