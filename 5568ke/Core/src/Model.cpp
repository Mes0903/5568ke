#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Model.hpp"

Model::~Model() { cleanup(); }

void Model::draw(Shader& shader, glm::mat4 const& modelMatrix) const
{
	shader.setMat4("model", modelMatrix);

	// If model has animations, set bone matrices
	if (hasAnimations) {
		for (int i = 0; i < skeleton.boneCount && i < Skeleton::MAX_BONES; i++) {
			std::string uniformName = "boneMatrices[" + std::to_string(i) + "]";
			shader.setMat4(uniformName.c_str(), skeleton.finalBoneMatrices[i]);
		}
	}

	for (auto const& mesh : meshes)
		mesh.draw(shader);
}

void Model::updateAnimation(float dt)
{
	if (hasAnimations) {
		animationPlayer.update(dt);
	}
}

glm::mat4 Model::calculateCenteredTransform(float scale) const
{
	if (boundingBoxes.empty()) {
		return glm::scale(glm::mat4(1.0f), glm::vec3(scale));
	}

	// Calculate center of the model using global bounding box
	glm::vec3 center = (globalBoundingBox.min + globalBoundingBox.max) * 0.5f;

	// Calculate dimensions
	glm::vec3 size = globalBoundingBox.max - globalBoundingBox.min;
	float maxDim = std::max(std::max(size.x, size.y), size.z);

	// Calculate appropriate scale factor if not provided
	float scaleFactor = (scale <= 0.0f) ? (1.0f / maxDim) : scale;

	// Create transform: first center the model, then scale it
	glm::mat4 transform = glm::mat4(1.0f);
	transform = glm::scale(transform, glm::vec3(scaleFactor));
	transform = glm::translate(transform, -center);

	return transform;
}

glm::mat4 Model::calculateGroundedTransform(float scale) const
{
	if (boundingBoxes.empty()) {
		return glm::scale(glm::mat4(1.0f), glm::vec3(scale));
	}

	// Find the y-coordinate of the bottom of the model
	float bottom = globalBoundingBox.min.y;

	// Calculate center in XZ plane
	glm::vec3 center = (globalBoundingBox.min + globalBoundingBox.max) * 0.5f;
	center.y = 0; // Ignore Y for centering in XZ plane

	// Calculate dimensions
	glm::vec3 size = globalBoundingBox.max - globalBoundingBox.min;
	float maxDim = std::max(std::max(size.x, size.y), size.z);

	// Calculate appropriate scale factor if not provided
	float scaleFactor = (scale <= 0.0f) ? (1.0f / maxDim) : scale;

	// Create transform: first translate the model so bottom is at y=0
	// and it's centered in XZ plane, then scale it
	glm::mat4 transform = glm::mat4(1.0f);
	transform = glm::scale(transform, glm::vec3(scaleFactor));

	// Create a translation that centers in XZ and places bottom at y=0
	glm::vec3 translation = -center;
	translation.y = -bottom; // Move bottom to y=0

	transform = glm::translate(transform, translation);

	return transform;
}

void Model::cleanup()
{
	// Clean up any dynamically allocated resources
	for (auto& mesh : meshes) {
		// Could add a cleanup method to Mesh class
		// mesh.cleanup();
	}
	meshes.clear();
	boundingBoxes.clear();

	// Clean up animations
	animations.clear();
	skeleton.finalBoneMatrices.clear();
}