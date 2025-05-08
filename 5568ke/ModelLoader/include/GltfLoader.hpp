#pragma once

#include "include_5568ke.hpp"

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>


#include "Animation.hpp"
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

	// Animation loading methods
	void loadSkeleton(tinygltf::Model& model, Model* outModel);
	void loadBones(tinygltf::Model& model, Model* outModel);
	void loadAnimations(tinygltf::Model& model, Model* outModel);
	void processNode(tinygltf::Model& model, int nodeIndex, SkeletonNode* parent, SkeletonNode* outNode,
									 std::unordered_map<std::string, SkeletonNode*>& nodeMap); // Changed from vector to unordered_map
	void processSkin(tinygltf::Model& model, int skinIndex, Skeleton& skeleton);
	void processAnimation(tinygltf::Model& model, int animIndex, Model* outModel);
	void extractKeyframes(tinygltf::Model& model, tinygltf::Animation const& anim, // Changed to const reference
												int channelIndex, Bone& bone);
	void applyVertexBoneData(tinygltf::Model& model, int meshIndex, Mesh& outMesh, Skeleton& skeleton);

	// Conversion helpers
	glm::mat4 aiMatrix4x4ToGlm(float const* mat);
	glm::quat aiQuaternionToGlm(float const* q);
	glm::vec3 aiVector3DToGlm(float const* vec);

	// Bounding box calculations
	BoundingBox calculateBoundingBox(Mesh const& mesh);
	BoundingBox calculateGlobalBoundingBox(std::vector<BoundingBox> const& boundingBoxes);
};