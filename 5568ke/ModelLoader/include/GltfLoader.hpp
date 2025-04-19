#pragma once

#include "include_5568ke.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "BoundingBox.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Texture.hpp"

// Material type enum
enum class MaterialType { BlinnPhong, PBR };

// Simplified GLTF model loader
class GltfLoader {
public:
	GltfLoader() = default;
	~GltfLoader() = default;

	// Main loading method
	Model* loadModel(std::string const& path);

	// Helper methods for model transformation
	static glm::vec3 calculateModelCenter(Model* model);
	static float calculateModelScale(Model* model, float targetSize = 1.0f);

	// Helper method to suggest a good camera position
	static glm::vec3 suggestCameraPosition(Model* model);

	// Apply positioning to a model
	static void positionModel(Model* model, glm::vec3 position = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(0.0f), float scale = 1.0f);

private:
	// Main GLTF loading implementation
	Model* loadGltf(std::string const& path, MaterialType type = MaterialType::BlinnPhong);

	// Helper methods
	Texture* loadTexture(tinygltf::Model& model, int textureIndex, TextureType type);
	Material* createMaterial(tinygltf::Model& model, tinygltf::Primitive& primitive, MaterialType type);
	void processMesh(tinygltf::Model& model, tinygltf::Mesh& mesh, Mesh& outMesh, MaterialType materialType);

	// Bounding box calculations
	BoundingBox calculateBoundingBox(Mesh const& mesh);
	BoundingBox calculateGlobalBoundingBox(std::vector<BoundingBox> const& boundingBoxes);
};