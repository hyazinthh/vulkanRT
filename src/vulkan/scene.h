#pragma once

#include "vertex.h"
#include "buffer.h"
#include "device.h"
#include "rt/top_level_as.h"
#include "rt/shader_binding_table.h"

class Scene {

	public:
		struct Object {
			std::vector<Vertex> vertices;
			std::unique_ptr<Buffer> vertexBuffer;
			std::vector<uint32_t> indices;
			std::unique_ptr<Buffer> indexBuffer;
			std::unique_ptr<BottomLevelAS> blAS;
		};

		struct Instance {
			glm::mat4 transform;
			std::shared_ptr<Object> object;
			uint32_t hitGroup;
		};

		Scene(Device* device);

		~Scene();

		std::shared_ptr<Object> addObject(const std::vector<Vertex>& vertices, 
			const std::vector<uint32_t>& indices);

		std::shared_ptr<Instance> addInstance(const std::shared_ptr<Object>& object,
			const glm::mat4& transform = glm::mat4(1.0f));

		void buildAccelerationStructure(bool updateOnly = false);

		void trace();

		const auto& getAccelerationStructure() const {
			return topLevelAS;
		}

		const auto& getObjects() const { 
			return objects;
		}

		std::shared_ptr<Scene::Instance> rotatingQuad;

		std::shared_ptr<Scene::Instance> floor;

	private:

		void createPipeline();

		void createShaderBindingTable();

		std::unique_ptr<Buffer> createBuffer(VkDeviceSize size, const void* data);

		int getObjectIndex(const std::shared_ptr<Instance> instance);

		Device* device = nullptr;

		std::vector<std::shared_ptr<Object>> objects;
		
		std::vector<std::shared_ptr<Instance>> instances;

		std::unique_ptr<TopLevelAS> topLevelAS;

		std::unique_ptr<Pipeline> pipeline;

		std::unique_ptr<ShaderBindingTable> shaderBindingTable;
};