#pragma once

#include "vertex.h"
#include "buffer.h"
#include "device.h"
#include "texture.h"
#include "rt/top_level_as.h"
#include "rt/raytracing_pipeline.h"
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

		struct Material {
			std::unique_ptr<Buffer> buffer;
			std::array<std::shared_ptr<Texture>, 4> textures;
			glm::vec4 color;
		};

		struct Instance {
			uint32_t index;
			std::shared_ptr<Object> object;
			std::shared_ptr<Material> material;
			std::unique_ptr<Buffer> buffer;
			glm::mat4 transform;
			uint32_t hitGroup;
			uint32_t mask;
		};

		Scene(Device* device);

		~Scene();

		void trace();

		void updateInstance(std::shared_ptr<Instance> instance);

		void updateMaterial(std::shared_ptr<Material> material);

		std::shared_ptr<Object> addObject(const std::vector<Vertex>& vertices, 
			const std::vector<uint32_t>& indices);

		std::shared_ptr<Texture> addTexture(const std::string& file, VkFormat format);

		std::shared_ptr<Material> addMaterial(const std::array<std::shared_ptr<Texture>, 4>& textures,
			const glm::vec4& color = glm::vec4(1));

		std::shared_ptr<Instance> addInstance(const std::shared_ptr<Object>& object, uint32_t hitGroup,
			const std::shared_ptr<Material>& material = nullptr, const glm::mat4& transform = glm::mat4(1.0f),
			uint32_t mask = 0xff);

		void buildAccelerationStructure(bool updateOnly = false);

		const auto& getAccelerationStructure() const {
			return topLevelAS;
		}

		const auto& getObjects() const { 
			return objects;
		}

		const auto& getInstances() const {
			return instances;
		}

		const auto& getMaterials() const {
			return materials;
		}

		const auto& getTextures() const {
			return textures;
		}

		std::shared_ptr<Scene::Instance> rotatingCube;

		std::shared_ptr<Scene::Instance> floor;

		std::shared_ptr<Scene::Instance> pointLight;

	private:

		std::unique_ptr<Buffer> createBuffer(VkDeviceSize size, const void* data);

		void copyToBuffer(const std::unique_ptr<Buffer>& buffer, VkDeviceSize size, const void* data);

		template <class T>
		int getIndex(const std::vector<T>& vector, const T& x) {
			auto it = std::find(vector.begin(), vector.end(), x);
			return (it != vector.end()) ? (int) std::distance(vector.begin(), it) : -1;
		}

		Device* device = nullptr;

		std::vector<std::shared_ptr<Object>> objects;
		
		std::vector<std::shared_ptr<Instance>> instances;

		std::vector<std::shared_ptr<Material>> materials;

		std::vector<std::shared_ptr<Texture>> textures;

		std::unique_ptr<TopLevelAS> topLevelAS;

		std::unique_ptr<RaytracingPipeline> pipeline;

		std::unique_ptr<ShaderBindingTable> shaderBindingTable;
};