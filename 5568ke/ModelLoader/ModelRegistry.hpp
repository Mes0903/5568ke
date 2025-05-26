#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "Scene.hpp"

// Forward declarations
class Model;
class GltfLoader;

// Enum for supported model formats
enum class ModelFormat {
	UNSUPPORTED,
	GLTF,
	OBJ,
	FBX,
	AUTO_DETECT // Automatically detect format from file extension
};

// Central registry for managing all model loaders and models in the application
class ModelRegistry {
public:
	static ModelRegistry& getInstance();

	// Load a model with optional position parameters
	std::shared_ptr<Model> loadModel(std::string const& path, std::string const& name = "");

	// Add a model to a scene with a transform matrix
	std::shared_ptr<GameObject> addModelToScene(Scene& scene, std::shared_ptr<Model> model);

	// Remove a model from a scene
	void removeModelFromScene(Scene& scene, std::string const& name);

public:
	Scene& sceneRef = Scene::getInstance();

private:
	ModelRegistry();
	~ModelRegistry() = default;

	// Detect format from file extension
	ModelFormat detectFormat_(std::string const& path);

	// Concrete loader instances
	std::unique_ptr<GltfLoader> gltfLoader_;
};
