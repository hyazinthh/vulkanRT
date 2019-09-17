#include "scene.h"
#include "rt/raytracing_pipeline.h"
#include "extensions.h"

Scene::Scene(Device* device) : device(device) {

	// Rotating quad
	{
		const std::vector<Vertex> vertices = {
			Vertex({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}),
			Vertex({0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}),
			Vertex({-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}),
			Vertex({0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f})
		};

		const std::vector<uint32_t> indices = {
			0, 1, 2, 0, 3, 1
		};

		rotatingQuad = addInstance(addObject(vertices, indices));
	}

	// Floor
	{
		const std::vector<Vertex> vertices = {
			Vertex({-3.0f, -3.0f, -1.5f}, {1.0f, 0.0f, 1.0f}),
			Vertex({3.0f, 3.0f, -1.5f}, {1.0f, 0.0f, 1.0f}),
			Vertex({-3.0f, 3.0f, -1.5f}, {1.0f, 0.0f, 1.0f}),
			Vertex({3.0f, -3.0f, -1.5f}, {1.0f, 0.0f, 1.0f})
		};

		const std::vector<uint32_t> indices = {
			0, 1, 2, 0, 3, 1
		};

		floor = addInstance(addObject(vertices, indices));
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
	const glm::mat4& transform) {

	auto inst = std::make_shared<Instance>();
	inst->object = object;
	inst->transform = transform;

	instances.push_back(inst);
	return inst;
}

void Scene::buildAccelerationStructure(bool updateOnly) {

	std::vector<TopLevelAS::Instance> instances;
	for (const auto& i : this->instances) {
		instances.push_back(TopLevelAS::Instance(i->object->blAS.get(), 0, i->hitGroup, i->transform));
	}

	if (updateOnly && topLevelAS.get()) {
		topLevelAS->update(instances);
	} else {
		topLevelAS = std::make_unique<TopLevelAS>(device, instances, true);
	}
}

void Scene::createPipeline() {
	std::unique_ptr<Shader> shaderMiss(Shader::loadFromFile(device, "shaders/ray_miss.rmiss", Shader::Type::Miss));
	std::unique_ptr<Shader> shaderClosestHit(Shader::loadFromFile(device, "shaders/ray_chit.rchit", Shader::Type::ClosestHit));
	std::unique_ptr<Shader> shaderRayGen(Shader::loadFromFile(device, "shaders/ray_gen.rgen", Shader::Type::RayGen));

	RaytracingPipeline* pipeline = new RaytracingPipeline(device);

	pipeline->addShaderStage(shaderRayGen.get());
	pipeline->addShaderStage(shaderMiss.get());

	for (auto& inst : instances) {
		inst->hitGroup = pipeline->startHitGroup();
		pipeline->addHitShaderStage(shaderClosestHit.get());
		pipeline->endHitGroup();
	}

	this->pipeline.reset(pipeline->create());
}

void Scene::createShaderBindingTable() {
	shaderBindingTable = std::make_unique<ShaderBindingTable>(device, pipeline.get());
	shaderBindingTable->addEntry(ShaderBindingTable::EntryType::RayGen);
	shaderBindingTable->addEntry(ShaderBindingTable::EntryType::Miss);

	for (const auto& inst : instances) {
		int id = getObjectIndex(inst);
		shaderBindingTable->addEntry(ShaderBindingTable::EntryType::HitGroup, &id, sizeof(id));
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

int Scene::getObjectIndex(const std::shared_ptr<Instance> instance) {

	for (int i = 0; i < objects.size(); i++) {
		if (objects[i] == instance->object) {
			return i;
		}
	}

	throw std::runtime_error("Instance references invalid object");
}