#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "BoundingBox.hpp"

class AnimationClip;
class Mesh;
class Node;
class Shader;

class Model {
public:
	Model() = default;
	~Model();

	// Core model data
	std::vector<Mesh> meshes;
	std::vector<int> meshNodeIndices; // Mesh -> Node mapping
	std::vector<BoundingBox> boundingBoxes;
	BoundingBox localSpaceBBox;

	// Metadata
	std::string name;
	std::string filePath;

	// Methods for drawing
	void draw(Shader const& shader, glm::mat4 const& modelMatrix) const;

	// Cleanup resources
	void cleanup();

	// Animation helper methods
	void updateJointMatrices();
	void updateJointMatricesFromNodes();
	void initializeDefaultPose();

public:
	// Animation support
	std::vector<std::shared_ptr<AnimationClip>> animations;
	std::vector<std::shared_ptr<Node>> nodes; // node list
	std::shared_ptr<Node> rootNode;						// used to represent the node tree

	// Skinning data
	std::vector<glm::mat4> inverseBindMatrices;
	std::vector<glm::mat4> jointMatrices;
	std::vector<int> nodeToJointMapping;
	std::vector<std::vector<std::pair<int, float>>> vertexJoints; // For each vertex: pairs of (jointIndex, weight)
};

namespace ModelUtil {
BoundingBox getMeshBBox(Mesh const& mesh);
void setLocalBBox(Model& m);
} // namespace ModelUtil