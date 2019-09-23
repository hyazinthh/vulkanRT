#include "scene.h"
#include "rt/raytracing_pipeline.h"
#include "extensions.h"

#include <glm/gtc/matrix_transform.hpp>

Scene::Scene(Device* device) : device(device) {

	// Pipeline
	pipeline = std::make_unique<RaytracingPipeline>(device);

	// Shaders
	std::unique_ptr<Shader> shaderMiss(Shader::loadFromFile(device, "shaders/primary.rmiss", Shader::Type::Miss));
	std::unique_ptr<Shader> shaderShadowMiss(Shader::loadFromFile(device, "shaders/shadow.rmiss", Shader::Type::Miss));
	std::unique_ptr<Shader> shaderClosestHit(Shader::loadFromFile(device, "shaders/primary.rchit", Shader::Type::ClosestHit));
	std::unique_ptr<Shader> shaderRayGen(Shader::loadFromFile(device, "shaders/primary.rgen", Shader::Type::RayGen));
	std::unique_ptr<Shader> shaderLight(Shader::loadFromFile(device, "shaders/light_source.rchit", Shader::Type::ClosestHit));

	// Stages
	pipeline->addShaderStage(shaderRayGen.get());
	pipeline->addShaderStage(shaderMiss.get());
	pipeline->addShaderStage(shaderShadowMiss.get());

	uint32_t hitGroupNormal = pipeline->startHitGroup();
	pipeline->addHitShaderStage(shaderClosestHit.get());
	pipeline->endHitGroup();

	uint32_t hitGroupLight = pipeline->startHitGroup();
	pipeline->addHitShaderStage(shaderLight.get());
	pipeline->endHitGroup();

	pipeline->create();
	shaderBindingTable.reset(pipeline->generateShaderBindingTable());

	// Textures
	auto textureChecker = addTexture("textures/checker.png", VK_FORMAT_R8G8B8A8_UNORM);
	auto textureMarble = addTexture("textures/marble.png", VK_FORMAT_R8G8B8A8_UNORM);
	auto textureNormalMap = addTexture("textures/normalmap.png", VK_FORMAT_R8G8B8A8_UNORM);

	// Materials
	auto materialFloor = addMaterial({ textureChecker, nullptr, nullptr, nullptr });
	auto materialCube = addMaterial({ nullptr, textureNormalMap, nullptr, nullptr });

	// Objects
	std::shared_ptr<Object> quad;
	std::shared_ptr<Object> cube;

	{
		const std::vector<Vertex> vertices = {
			Vertex({-.5f, -.5f, 0}, {0, 0, 1}, {1, 0, 0}, {0, 0}),
			Vertex({ .5f,  .5f, 0}, {0, 0, 1}, {1, 0, 0}, {4, 4}),
			Vertex({-.5f,  .5f, 0}, {0, 0, 1}, {1, 0, 0}, {0, 4}),
			Vertex({ .5f, -.5f, 0}, {0, 0, 1}, {1, 0, 0}, {4, 0})
		};

		const std::vector<uint32_t> indices = {
			0, 1, 2, 0, 3, 1
		};

		quad = addObject(vertices, indices);
	}

	{
		const std::vector<Vertex> vertices = {
			// Top
			Vertex({-.5f, -.5f, .5f}, {0, 0, 1}, {1, 0, 0}, {0, 0}),
			Vertex({ .5f,  .5f, .5f}, {0, 0, 1}, {1, 0, 0}, {1, 1}),
			Vertex({-.5f,  .5f, .5f}, {0, 0, 1}, {1, 0, 0}, {0, 1}),
			Vertex({ .5f, -.5f, .5f}, {0, 0, 1}, {1, 0, 0}, {1, 0}),

			// Bottom
			Vertex({-.5f, -.5f, -.5f}, {0, 0, -1}, {1, 0, 0}, {0, 0}),
			Vertex({ .5f,  .5f, -.5f}, {0, 0, -1}, {1, 0, 0}, {1, 1}),
			Vertex({-.5f,  .5f, -.5f}, {0, 0, -1}, {1, 0, 0}, {0, 1}),
			Vertex({ .5f, -.5f, -.5f}, {0, 0, -1}, {1, 0, 0}, {1, 0}),

			// Left
			Vertex({-.5f, -.5f, -.5f}, {-1, 0, 0}, {0, 1, 0}, {0, 0}),
			Vertex({-.5f,  .5f,  .5f}, {-1, 0, 0}, {0, 1, 0}, {1, 1}),
			Vertex({-.5f, -.5f,  .5f}, {-1, 0, 0}, {0, 1, 0}, {0, 1}),
			Vertex({-.5f,  .5f, -.5f}, {-1, 0, 0}, {0, 1, 0}, {1, 0}),

			// Right
			Vertex({ .5f, -.5f, -.5f}, {1, 0, 0}, {0, 1, 0}, {0, 0}),
			Vertex({ .5f,  .5f,  .5f}, {1, 0, 0}, {0, 1, 0}, {1, 1}),
			Vertex({ .5f, -.5f,  .5f}, {1, 0, 0}, {0, 1, 0}, {0, 1}),
			Vertex({ .5f,  .5f, -.5f}, {1, 0, 0}, {0, 1, 0}, {1, 0}),

			// Back
			Vertex({-.5f,  .5f, -.5f}, {0, 1, 0}, {1, 0, 0}, {0, 0}),
			Vertex({ .5f,  .5f,  .5f}, {0, 1, 0}, {1, 0, 0}, {1, 1}),
			Vertex({-.5f,  .5f,  .5f}, {0, 1, 0}, {1, 0, 0}, {0, 1}),
			Vertex({ .5f,  .5f, -.5f}, {0, 1, 0}, {1, 0, 0}, {1, 0}),

			// Front
			Vertex({-.5f, -.5f, -.5f}, {0, -1, 0}, {1, 0, 0}, {0, 0}),
			Vertex({ .5f, -.5f,  .5f}, {0, -1, 0}, {1, 0, 0}, {1, 1}),
			Vertex({-.5f, -.5f,  .5f}, {0, -1, 0}, {1, 0, 0}, {0, 1}),
			Vertex({ .5f, -.5f, -.5f}, {0, -1, 0}, {1, 0, 0}, {1, 0})
		};

		const std::vector<uint32_t> indices = {
			0, 1, 2, 0, 3, 1,
			4, 5, 6, 4, 5, 7,
			8, 9, 10, 8, 9, 11,
			12, 13, 14, 12, 13, 15,
			16, 17, 18, 16, 17, 19,
			20, 21, 22, 20, 21, 23
		};

		cube = addObject(vertices, indices);
	}

	// Floor
	{
		floor = addInstance(quad, hitGroupNormal, materialFloor, glm::scale(glm::mat4(1), glm::vec3(6)));
	}

	// Rotating cube
	{
		rotatingCube = addInstance(cube, hitGroupNormal, materialCube);
	}

	// Point light
	{
		pointLight = addInstance(cube, hitGroupLight);
		pointLight->mask = 0x01;
	}

	buildAccelerationStructure();
}

Scene::~Scene() {

}

void Scene::trace() {
	pipeline->bind(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV);

	vkCmdBindDescriptorSets(device->getCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
		pipeline->getLayout(), 0, 1, &device->getDescriptorSet(), 0, nullptr);

	auto rg = ShaderBindingTable::EntryType::RayGen;
	auto miss = ShaderBindingTable::EntryType::Miss;
	auto hitGroup = ShaderBindingTable::EntryType::HitGroup;

	auto ext = device->getSwapchain()->getExtent();

	VkExt::vkCmdTraceRaysNV(device->getCommandBuffer(),
		*shaderBindingTable->getBuffer(), shaderBindingTable->getOffset(rg),
		*shaderBindingTable->getBuffer(), shaderBindingTable->getOffset(miss), shaderBindingTable->getEntrySize(miss),
		*shaderBindingTable->getBuffer(), shaderBindingTable->getOffset(hitGroup), shaderBindingTable->getEntrySize(hitGroup),
		VK_NULL_HANDLE, 0, 0, ext.width, ext.height, 1);
}

void Scene::updateInstance(std::shared_ptr<Instance> instance) {

	struct Data {
		int objectId;
		int materialId;
		int _pad[2];
		glm::mat4 normalMatrix;
	};

	Data data;
	data.objectId = getIndex(objects, instance->object);
	data.materialId = getIndex(materials, instance->material);
	data.normalMatrix = glm::transpose(glm::inverse(glm::mat3(instance->transform)));

	if (instance->buffer == nullptr) {
		instance->buffer = createBuffer(sizeof(Data), &data);
	} else {
		copyToBuffer(instance->buffer, sizeof(Data), &data);
	}
}

void Scene::updateMaterial(std::shared_ptr<Material> material) {
	struct Data {
		int textureId[4];
		glm::vec4 color;
	};

	Data data;
	data.color = material->color;

	for (size_t i = 0; i < material->textures.size(); i++) {
		data.textureId[i] = getIndex(textures, material->textures[i]);
	}

	if (material->buffer == nullptr) {
		material->buffer = createBuffer(sizeof(Data), &data);
	} else {
		copyToBuffer(material->buffer, sizeof(Data), &data);
	}
}

std::shared_ptr<Scene::Object> Scene::addObject(const std::vector<Vertex>& vertices,
	const std::vector<uint32_t>& indices) {

	auto obj = std::make_shared<Object>();
	obj->vertices = vertices;
	obj->vertexBuffer = createBuffer(sizeof(Vertex) * vertices.size(), vertices.data());
	obj->indices = indices;
	obj->indexBuffer = createBuffer(sizeof(uint32_t) * indices.size(), indices.data());
	obj->blAS = std::make_unique<BottomLevelAS>(device,
		obj->vertexBuffer.get(), (uint32_t) vertices.size(), sizeof(Vertex),
		obj->indexBuffer.get(), (uint32_t) indices.size());

	objects.push_back(obj);

	return obj;
}

std::shared_ptr<Texture> Scene::addTexture(const std::string& file, VkFormat format) {
	auto tex = std::make_shared<Texture>(device, file, format);
	textures.push_back(tex);

	return tex;
}

std::shared_ptr<Scene::Material> Scene::addMaterial(const std::array<std::shared_ptr<Texture>, 4>& textures,
	const glm::vec4& color) {

	auto mat = std::make_shared<Material>();
	mat->color = color;

	for (size_t i = 0; i < textures.size(); i++) {
		mat->textures[i] = textures[i];
	}

	updateMaterial(mat);

	materials.push_back(mat);
	return mat;
}

std::shared_ptr<Scene::Instance> Scene::addInstance(const std::shared_ptr<Object>& object,
	uint32_t hitGroup, const std::shared_ptr<Material>& material,
	const glm::mat4& transform, uint32_t mask) {

	auto inst = std::make_shared<Instance>();
	inst->index = (uint32_t) instances.size();
	inst->object = object;
	inst->material = material;
	inst->transform = transform;
	inst->hitGroup = hitGroup;
	inst->mask = mask;
	updateInstance(inst);

	instances.push_back(inst);
	return inst;
}

void Scene::buildAccelerationStructure(bool updateOnly) {

	std::vector<TopLevelAS::Instance> instances;
	for (const auto& i : this->instances) {
		instances.push_back(
			TopLevelAS::Instance(i->object->blAS.get(), (uint32_t) i->index, i->hitGroup, i->mask, i->transform)
		);
	}

	if (updateOnly && topLevelAS.get()) {
		topLevelAS->update(instances);
	} else {
		topLevelAS = std::make_unique<TopLevelAS>(device, instances, true);
	}
}

std::unique_ptr<Buffer> Scene::createBuffer(VkDeviceSize size, const void* data) {

	auto buffer = std::make_unique<Buffer>(device, size,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	copyToBuffer(buffer, size, data);

	return buffer;
}

void Scene::copyToBuffer(const std::unique_ptr<Buffer>& buffer, VkDeviceSize size, const void* data) {

	Buffer localBuffer(device, size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	localBuffer.fill(data);
	localBuffer.copyTo(buffer.get());
}