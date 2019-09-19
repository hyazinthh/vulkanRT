#include "scene.h"
#include "rt/raytracing_pipeline.h"
#include "extensions.h"

#include <glm/gtc/matrix_transform.hpp>

Scene::Scene(Device* device) : device(device) {

	// Objects
	std::shared_ptr<Object> quad;
	std::shared_ptr<Object> cube;

	{
		const std::vector<Vertex> vertices = {
			Vertex({-.5f, -.5f, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0}),
			Vertex({ .5f,  .5f, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {1, 1}),
			Vertex({-.5f,  .5f, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 1}),
			Vertex({ .5f, -.5f, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {1, 0})
		};

		const std::vector<uint32_t> indices = {
			0, 1, 2, 0, 3, 1
		};

		quad = addObject(vertices, indices);
	}

	{
		const std::vector<Vertex> vertices = {
			// Top
			Vertex({-.5f, -.5f, .5f}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0}),
			Vertex({ .5f,  .5f, .5f}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {1, 1}),
			Vertex({-.5f,  .5f, .5f}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 1}),
			Vertex({ .5f, -.5f, .5f}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {1, 0}),

			// Bottom
			Vertex({-.5f, -.5f, -.5f}, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, {0, 0}),
			Vertex({ .5f,  .5f, -.5f}, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, {1, 1}),
			Vertex({-.5f,  .5f, -.5f}, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, {0, 1}),
			Vertex({ .5f, -.5f, -.5f}, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, {1, 0}),

			// Left
			Vertex({-.5f, -.5f, -.5f}, {-1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0}),
			Vertex({-.5f,  .5f,  .5f}, {-1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1}),
			Vertex({-.5f, -.5f,  .5f}, {-1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 1}),
			Vertex({-.5f,  .5f, -.5f}, {-1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0}),

			// Right
			Vertex({ .5f, -.5f, -.5f}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0}),
			Vertex({ .5f,  .5f,  .5f}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1}),
			Vertex({ .5f, -.5f,  .5f}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 1}),
			Vertex({ .5f,  .5f, -.5f}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0}),

			// Back
			Vertex({-.5f,  .5f, -.5f}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0}),
			Vertex({ .5f,  .5f,  .5f}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 1}),
			Vertex({-.5f,  .5f,  .5f}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1}),
			Vertex({ .5f,  .5f, -.5f}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0}),

			// Front
			Vertex({-.5f, -.5f, -.5f}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0}),
			Vertex({ .5f, -.5f,  .5f}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 1}),
			Vertex({-.5f, -.5f,  .5f}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1}),
			Vertex({ .5f, -.5f, -.5f}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0})
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
		floor = addInstance(quad);
		floor->transform = glm::scale(glm::mat4(1), glm::vec3(6));
		floor->textureId[0] = 0;
		floor->textureId[1] = 2;
	}

	// Rotating cube
	{
		rotatingCube = addInstance(cube);
		rotatingCube->textureId[1] = 2;
	}

	// Point light
	{
		pointLight = addInstance(cube);
		pointLight->mask = 0x01;
	}

	createPipeline();
	createShaderBindingTable();
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

	VkDeviceSize offset = shaderBindingTable->getOffset(ShaderBindingTable::EntryType::HitGroup);
	offset += shaderBindingTable->getEntrySize(ShaderBindingTable::EntryType::HitGroup) * instance->index;
	offset += shaderBindingTable->getShaderGroupHandleSize();

	// Update SBT entry
	ShaderRecord r = createShaderRecord(instance);

	auto buffer = shaderBindingTable->getBuffer();
	void* data = buffer->map(offset, sizeof(ShaderRecord));

	memcpy_s(data, sizeof(ShaderRecord), &r, sizeof(r));

	buffer->unmap();
}

std::shared_ptr<Scene::Object> Scene::addObject(const std::vector<Vertex>& vertices,
	const std::vector<uint32_t>& indices) {

	auto obj = std::make_shared<Object>();
	obj->vertices = vertices;
	obj->vertexBuffer = (createBuffer(sizeof(Vertex) * vertices.size(), vertices.data()));
	obj->indices = indices;
	obj->indexBuffer = (createBuffer(sizeof(uint32_t) * indices.size(), indices.data()));
	obj->blAS = std::make_unique<BottomLevelAS>(device,
		obj->vertexBuffer.get(), (uint32_t) vertices.size(), sizeof(Vertex),
		obj->indexBuffer.get(), (uint32_t) indices.size());

	objects.push_back(obj);

	return obj;
}

std::shared_ptr<Scene::Instance> Scene::addInstance(const std::shared_ptr<Object>& object,
	const glm::vec3& color, const glm::mat4& transform, uint32_t mask) {

	auto inst = std::make_shared<Instance>();
	inst->index = instances.size();
	inst->object = object;
	inst->color = glm::vec4(color, 1);
	inst->transform = transform;
	inst->mask = mask;
	inst->textureId.fill(-1);

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

void Scene::createPipeline() {
	std::unique_ptr<Shader> shaderMiss(Shader::loadFromFile(device, "shaders/primary.rmiss", Shader::Type::Miss));
	std::unique_ptr<Shader> shaderShadowMiss(Shader::loadFromFile(device, "shaders/shadow.rmiss", Shader::Type::Miss));
	std::unique_ptr<Shader> shaderClosestHit(Shader::loadFromFile(device, "shaders/primary.rchit", Shader::Type::ClosestHit));
	std::unique_ptr<Shader> shaderRayGen(Shader::loadFromFile(device, "shaders/primary.rgen", Shader::Type::RayGen));
	std::unique_ptr<Shader> shaderLight(Shader::loadFromFile(device, "shaders/light_source.rchit", Shader::Type::ClosestHit));

	RaytracingPipeline* pipeline = new RaytracingPipeline(device);

	pipeline->addShaderStage(shaderRayGen.get());
	pipeline->addShaderStage(shaderMiss.get());
	pipeline->addShaderStage(shaderShadowMiss.get());

	floor->hitGroup = pipeline->startHitGroup();
	pipeline->addHitShaderStage(shaderClosestHit.get());
	pipeline->endHitGroup();

	rotatingCube->hitGroup = pipeline->startHitGroup();
	pipeline->addHitShaderStage(shaderClosestHit.get());
	pipeline->endHitGroup();

	pointLight->hitGroup = pipeline->startHitGroup();
	pipeline->addHitShaderStage(shaderLight.get());
	pipeline->endHitGroup();

	this->pipeline.reset(pipeline->create());
}

void Scene::createShaderBindingTable() {
	shaderBindingTable = std::make_unique<ShaderBindingTable>(device, pipeline.get());
	shaderBindingTable->addEntry(ShaderBindingTable::EntryType::RayGen);
	shaderBindingTable->addEntry(ShaderBindingTable::EntryType::Miss);
	shaderBindingTable->addEntry(ShaderBindingTable::EntryType::Miss);

	for (const auto& inst : instances) {
		ShaderRecord data = createShaderRecord(inst);
		shaderBindingTable->addEntry(ShaderBindingTable::EntryType::HitGroup, &data, sizeof(data));
	}

	shaderBindingTable->create();
}

std::unique_ptr<Buffer> Scene::createBuffer(VkDeviceSize size, const void* data) {

	Buffer localBuffer(device, size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	auto buffer = std::make_unique<Buffer>(device, localBuffer.getSize(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	localBuffer.fill(data);
	localBuffer.copyTo(buffer.get());

	return buffer;
}

Scene::ShaderRecord Scene::createShaderRecord(const std::shared_ptr<Instance>& instance) {
	ShaderRecord r = {};
	r.ids[0] = getObjectIndex(instance);
	r.ids[1] = instance->textureId[0];
	r.ids[2] = instance->textureId[1];
	r.ids[3] = instance->textureId[2];
	r.color = instance->color;
	r.normalMatrix = glm::transpose(glm::inverse(glm::mat3(instance->transform)));

	return r;
}

int Scene::getObjectIndex(const std::shared_ptr<Instance> instance) {

	for (int i = 0; i < objects.size(); i++) {
		if (objects[i] == instance->object) {
			return i;
		}
	}

	throw std::runtime_error("Instance references invalid object");
}