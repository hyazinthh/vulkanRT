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
		class IObject {
			public:
				virtual ~IObject() = default;
				
				virtual BottomLevelAS* getBottomLevelAS() = 0;

				virtual uint32_t getIndex() const = 0;
		};

		class Mesh : public IObject {
			public:
				Mesh(uint32_t index, std::unique_ptr<BottomLevelAS>& blAS,
					std::unique_ptr<Buffer>& vertexBuffer, std::unique_ptr<Buffer>& indexBuffer) 
					: index(index), blAS(std::move(blAS)), vertexBuffer(std::move(vertexBuffer)), indexBuffer(std::move(indexBuffer)) {}

				uint32_t getIndex() const {
					return index;
				}

				BottomLevelAS* getBottomLevelAS() {
					return blAS.get();
				}

				const auto& getVertexBuffer() const {
					return vertexBuffer;
				}

				const auto& getIndexBuffer() const {
					return indexBuffer;
				}

			private:
				uint32_t index;

				std::unique_ptr<BottomLevelAS> blAS;

				std::unique_ptr<Buffer> vertexBuffer;

				std::unique_ptr<Buffer> indexBuffer;
		};

		struct Sphere : public IObject {
			public:
				Sphere(uint32_t index, std::unique_ptr<BottomLevelAS>& blAS, std::unique_ptr<Buffer>& buffer)
					: index(index), blAS(std::move(blAS)), buffer(std::move(buffer)) {}

				uint32_t getIndex() const {
					return index;
				}

				BottomLevelAS* getBottomLevelAS() {
					return blAS.get();
				}

				const auto& getBuffer() const {
					return buffer;
				}

			private:
				uint32_t index;

				std::unique_ptr<BottomLevelAS> blAS;

				std::unique_ptr<Buffer> buffer;
		};

		struct Material {
			std::unique_ptr<Buffer> buffer;
			std::array<std::shared_ptr<Texture>, 4> textures;
			glm::vec4 color;
		};

		struct Instance {
			uint32_t index;
			std::shared_ptr<IObject> object;
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

		std::shared_ptr<IObject> addMesh(const std::vector<Vertex>& vertices, 
			const std::vector<uint32_t>& indices);

		std::shared_ptr<IObject> addSphere(float radius);

		std::shared_ptr<Texture> addTexture(const std::string& file, VkFormat format);

		std::shared_ptr<Material> addMaterial(const std::array<std::shared_ptr<Texture>, 4>& textures,
			const glm::vec4& color = glm::vec4(1));

		std::shared_ptr<Instance> addInstance(const std::shared_ptr<IObject>& object, uint32_t hitGroup,
			const std::shared_ptr<Material>& material = nullptr, const glm::mat4& transform = glm::mat4(1.0f),
			uint32_t mask = 0xff);

		void buildAccelerationStructure(bool updateOnly = false);

		const auto& getAccelerationStructure() const {
			return topLevelAS;
		}

		const auto& getMeshes() const { 
			return meshes;
		}

		const auto& getSpheres() const {
			return spheres;
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

		std::vector<std::shared_ptr<Mesh>> meshes;

		std::vector<std::shared_ptr<Sphere>> spheres;
		
		std::vector<std::shared_ptr<Instance>> instances;

		std::vector<std::shared_ptr<Material>> materials;

		std::vector<std::shared_ptr<Texture>> textures;

		std::unique_ptr<TopLevelAS> topLevelAS;

		std::unique_ptr<RaytracingPipeline> pipeline;

		std::unique_ptr<ShaderBindingTable> shaderBindingTable;
};