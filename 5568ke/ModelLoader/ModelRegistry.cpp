#include "ModelRegistry.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>

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
Model* ModelRegistry::loadModel(std::string const& path, std::string const& name, glm::vec3 position, glm::vec3 rotation, float scale)
{
	// Use provided name or generate one from path
	std::string modelName = name.empty() ? std::filesystem::path(path).stem().string() : name;

	// Check if model is already loaded
	auto it = modelCache_.find(modelName);
	if (it != modelCache_.end()) {
		return it->second.get();
	}

	// Detect format from file extension
	ModelFormat format = detectFormat_(path);

	// Load model based on format
	Model* model = nullptr;
	switch (format) {
	case ModelFormat::GLTF:
		model = gltfLoader_->loadModel(path);
		break;
	default:
		std::cerr << "[ModelRegistry ERROR] Unsupported model format" << std::endl;
		return nullptr;
	}

	if (model) {
		// Store default transform parameters on the model
		model->defaultScale = scale;
		model->defaultRotation = rotation;
		model->defaultTranslation = position;

		// Cache the model
		auto result = model;
		modelCache_[modelName] = std::unique_ptr<Model>(model);

		std::cout << "[ModelRegistry] Successfully loaded model '" << modelName << "'" << std::endl;
		return result;
	}

	std::cerr << "[ModelRegistry ERROR] Failed to load model '" << path << "'" << std::endl;
	return nullptr;
}

// Get a previously loaded model by name
Model* ModelRegistry::getModel(std::string const& name)
{
	auto it = modelCache_.find(name);
	if (it != modelCache_.end()) {
		return it->second.get();
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
void ModelRegistry::addModelToScene(Scene& scene, Model* model, std::string const& name, glm::mat4 transform)
{
	if (!model) {
		std::cerr << "[ModelRegistry ERROR] Invalid Model Pointer" << std::endl;
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

// Add a model to a scene with position, rotation, scale
void ModelRegistry::addModelToScene(Scene& scene, Model* model, std::string const& name, glm::vec3 position, glm::vec3 rotation, float scale)
{
	if (!model) {
		std::cerr << "[ModelRegistry ERROR] Invalid Model Pointer" << std::endl;
		return;
	}

	// Create transformation matrix
	glm::mat4 transform = glm::mat4(1.0f);

	// Apply scale
	transform = glm::scale(transform, glm::vec3(scale));

	// Apply rotation (in radians)
	transform = glm::rotate(transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	transform = glm::rotate(transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	transform = glm::rotate(transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

	// Apply translation
	transform = glm::translate(transform, position);

	// Call the matrix version
	addModelToScene(scene, model, name, transform);
}

// Add a model with automatic centering at a specified position
void ModelRegistry::addModelToSceneCentered(Scene& scene, Model* model, std::string const& name, glm::vec3 position, glm::vec3 rotation, float scale)
{
	if (!model) {
		std::cerr << "[ModelRegistry ERROR] Invalid Model Pointer" << std::endl;
		return;
	}

	// Calculate model center based on bounding box
	glm::vec3 center = (model->globalBoundingBox.min + model->globalBoundingBox.max) * 0.5f;

	// Calculate appropriate scale factor if needed
	float scaleFactor = scale;
	if (scale <= 0.0f) {
		glm::vec3 size = model->globalBoundingBox.max - model->globalBoundingBox.min;
		float maxDim = std::max(std::max(size.x, size.y), size.z);
		scaleFactor = 1.5f / maxDim;
	}

	// Create transformation matrix
	glm::mat4 transform = glm::mat4(1.0f);

	// Apply scale
	transform = glm::scale(transform, glm::vec3(scaleFactor));

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

// Set format defaults (to be applied to newly loaded models)
void ModelRegistry::setFormatDefaults(ModelFormat format, float scale, glm::vec3 rotation, glm::vec3 translation)
{
	FormatDefaults defaults;
	defaults.scale = scale;
	defaults.rotation = rotation;
	defaults.translation = translation;
	formatDefaults_[format] = defaults;
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
	std::cerr << "[ModelRegistry ERROR] Unknown file extension '" << extension << "', defaulting to GLTF loader" << std::endl;
	return ModelFormat::GLTF;
}