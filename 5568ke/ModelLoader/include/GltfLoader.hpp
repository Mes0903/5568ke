#pragma once

#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "BoundingBox.hpp"
#include "Material.hpp"
#include "Texture.hpp"

class Model;
class Node;
class Mesh;

class GltfLoader {
public:
	GltfLoader() = default;
	~GltfLoader() = default;

	// Main loading method
	std::shared_ptr<Model> loadModel(std::string const& path);

private:
	// Main GLTF loading implementation
	std::shared_ptr<Model> loadGltf_(std::string const& path, MaterialType type = MaterialType::BlinnPhong);

	// Helper methods
	Texture* loadTexture_(tinygltf::Model const& model, int textureIndex, TextureType type);
	Material* createMaterial_(tinygltf::Model const& model, tinygltf::Primitive const& primitive, MaterialType type);
	void processMesh_(tinygltf::Model const& model, tinygltf::Mesh const& mesh, Mesh& outMesh, MaterialType materialType);

	// Animation loading methods
	void loadAnimations_(std::shared_ptr<Model> model, tinygltf::Model const& gltfModel);
	void loadNodeHierarchy_(std::shared_ptr<Model> model, tinygltf::Model const& gltfModel);
	void processNodeTreeRecursive_(std::shared_ptr<Model> model, tinygltf::Model const& gltfModel, int nodeIndex, glm::mat4 const& parentMatrix);

	// Skin and animation data loading
	void loadSkinData_(std::shared_ptr<Model> model, tinygltf::Model const& gltfModel);
};
