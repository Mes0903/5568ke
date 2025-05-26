#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <memory>
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
	void cleanup();

	void draw(Shader const& shader, glm::mat4 const& modelMatrix) const;
	void updateLocalMatrices();

public:
	// Core model data
	std::vector<Mesh> meshes;
	std::vector<int> meshNodeIndices; // Mesh -> Node mapping
	std::vector<BoundingBox> boundingBoxes;
	BoundingBox localSpaceBBox;

	// Metadata
	std::string modelName;

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
