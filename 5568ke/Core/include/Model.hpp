#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

#include "BoundingBox.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"

class Model {
public:
	Model() = default;
	~Model();

	// Core model data
	std::vector<Mesh> meshes;
	std::vector<BoundingBox> boundingBoxes;
	BoundingBox globalBoundingBox;

	// Metadata
	std::string name;
	std::string filePath;

	// Default transformation values (can be set per format)
	float defaultScale = 1.0f;
	glm::vec3 defaultRotation = glm::vec3(0.0f);
	glm::vec3 defaultTranslation = glm::vec3(0.0f);

	// Methods for drawing
	void draw(Shader& shader, glm::mat4 const& modelMatrix) const;

	// Calculate a centered and scaled matrix based on model's bounding box
	glm::mat4 calculateCenteredTransform(float scale = 1.0f) const;

	// Calculate a transform that places the model on the ground plane (y=0)
	glm::mat4 calculateGroundedTransform(float scale = 1.0f) const;

	// Cleanup resources
	void cleanup();
};