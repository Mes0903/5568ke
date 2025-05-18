#define GLM_ENABLE_EXPERIMENTAL

#include "ModelRegistry.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "GltfLoader.hpp"
#include "Model.hpp"
#include "Scene.hpp"

// Singleton accessor
ModelRegistry& ModelRegistry::getInstance()
{
	static ModelRegistry instance;
	return instance;
}

// Constructor/Destructor
ModelRegistry::ModelRegistry() : gltfLoader_(std::make_unique<GltfLoader>()) {}
ModelRegistry::~ModelRegistry() { cleanup(); }

// Load a model with optional position parameters
std::shared_ptr<Model> ModelRegistry::loadModel(std::string const& path, std::string const& name, glm::vec3 position, glm::vec3 rotation, float scale)
{
	// Use provided name or generate one from path
	std::string modelName = name.empty() ? std::filesystem::path(path).stem().string() : name;

	// Check if model is already loaded
	auto it = modelCache_.find(modelName);
	if (it != modelCache_.end()) {
		return it->second;
	}

	// Detect format from file extension
	ModelFormat format = detectFormat_(path);

	// Load model based on format
	std::shared_ptr<Model> model;
	switch (format) {
	case ModelFormat::GLTF:
		model = gltfLoader_->loadModel(path);
		break;
	default:
		std::cout << "[ModelRegistry ERROR] Unsupported model format" << std::endl;
		return nullptr;
	}

	if (model) {
		// Cache the model
		auto result = std::shared_ptr<Model>(model);
		modelCache_[modelName] = result;

		std::cout << "[ModelRegistry] Successfully loaded model '" << modelName << "'" << std::endl;
		return result;
	}

	std::cout << "[ModelRegistry ERROR] Failed to load model '" << path << "'" << std::endl;
	return nullptr;
}

// Get a previously loaded model by name
std::shared_ptr<Model> ModelRegistry::getModel(std::string const& name)
{
	auto it = modelCache_.find(name);
	if (it != modelCache_.end()) {
		return it->second;
	}
	return nullptr;
}

// Unload a model by name
bool ModelRegistry::unloadModel(std::string const& name)
{
	auto it = modelCache_.find(name);
	if (it != modelCache_.end()) {
		modelCache_.erase(it);

		// Remove from registered models list
		auto listIt = std::find(registeredModels_.begin(), registeredModels_.end(), name);
		if (listIt != registeredModels_.end()) {
			registeredModels_.erase(listIt);
		}

		return true;
	}
	return false;
}

// Add a model to a scene with a transform matrix
void ModelRegistry::addModelToScene(Scene& scene, std::shared_ptr<Model> model, std::string const& name, glm::mat4 transform)
{
	if (!model) {
		std::cout << "[ModelRegistry ERROR] Invalid Model Pointer" << std::endl;
		return;
	}

	// Add to scene
	scene.addEntity(model, transform, name);

	// Register model name for UI
	if (std::find(registeredModels_.begin(), registeredModels_.end(), name) == registeredModels_.end()) {
		registeredModels_.push_back(name);
	}

	std::cout << "[ModelRegistry] Added model '" << name << "' to scene" << std::endl;
}

// Add a model with automatic centering at a specified position
void ModelRegistry::addModelToSceneCentered(Scene& scene, std::shared_ptr<Model> model, std::string const& name, glm::vec3 position, glm::vec3 rotation,
																						float scale)
{
	if (!model) {
		std::cout << "[ModelRegistry ERROR] Invalid Model Pointer" << std::endl;
		return;
	}

	// Calculate model center based on bounding box
	glm::vec3 center = (model->localSpaceBBox.min + model->localSpaceBBox.max) * 0.5f;

	// Calculate appropriate scale factor if needed
	if (scale <= 0.0f) {
		scale = 1.0f;
	}

	// Create transformation matrix
	glm::mat4 transform = glm::mat4(1.0f);

	// Apply scale
	transform = glm::scale(transform, glm::vec3(scale));

	// Apply translation to center the model first
	transform = glm::translate(transform, -center);

	// Apply rotation
	transform = glm::rotate(transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	transform = glm::rotate(transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	transform = glm::rotate(transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

	// Apply final position
	transform = glm::translate(glm::mat4(1.0f), position) * transform;

	// Add to scene
	addModelToScene(scene, model, name, transform);
}

// Remove a model from a scene
void ModelRegistry::removeModelFromScene(Scene& scene, std::string const& name)
{
	scene.removeEntity(name);
	// We don't remove from registeredModels_ here since the model is still loaded,
	// just not in the scene anymore
}

// Get a list of all registered model names (for UI)
std::vector<std::string> const& ModelRegistry::getRegisteredModels() const { return registeredModels_; }

// Clean up all models
void ModelRegistry::cleanup()
{
	modelCache_.clear();
	registeredModels_.clear();
}

// Private method to detect format from file extension
ModelFormat ModelRegistry::detectFormat_(std::string const& path)
{
	std::string extension = std::filesystem::path(path).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	if (extension == ".gltf" || extension == ".glb") {
		return ModelFormat::GLTF;
	}
	else if (extension == ".obj") {
		return ModelFormat::OBJ;
	}
	else if (extension == ".fbx") {
		return ModelFormat::FBX;
	}

	// Default to GLTF if unknown
	std::cout << "[ModelRegistry ERROR] Unknown file extension '" << extension << "', defaulting to GLTF loader" << std::endl;
	return ModelFormat::UNSUPPORTED;
}
