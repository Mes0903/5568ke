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

// Load a model with optional position parameters
std::shared_ptr<Model> ModelRegistry::loadModel(std::string const& path, std::string const& name, glm::vec3 position, glm::vec3 rotation, float scale)
{
	// Use provided name or generate one from path
	// Since most of the model are called scene.gltf, so we used the folder name that contained the model as the default name.
	std::string modelName = name.empty() ? std::filesystem::path(path).parent_path().stem().string() // Name of parent folder
																			 : name;

	// Check if model is already loaded
	// Append (1), (2), ... until an unused name is found
	int suffix = 1;

	int index = 1;
	std::string baseName = modelName;
	while (sceneRef.findEntity(modelName)) {
		modelName = baseName + '(' + std::to_string(suffix++) + ')';
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
		model->modelName = modelName;
		model->updateMatrices();

		std::cout << "[ModelRegistry] Successfully loaded model '" << modelName << "'" << std::endl;
		return model;
	}

	std::cout << "[ModelRegistry ERROR] Failed to load model '" << path << "'" << std::endl;
	return nullptr;
}

// Add a model to a scene with a transform matrix
void ModelRegistry::addModelToScene(Scene& scene, std::shared_ptr<Model> model)
{
	if (!model) {
		std::cout << "[ModelRegistry ERROR] Invalid Model Pointer" << std::endl;
		return;
	}

	// Add to scene
	scene.addEntity(model);

	std::cout << "[ModelRegistry] Added model '" << model->modelName << "' to scene" << std::endl;
}

// Remove a model from a scene
void ModelRegistry::removeModelFromScene(Scene& scene, std::string const& name) { scene.removeEntity(name); }

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
