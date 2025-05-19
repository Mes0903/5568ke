#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class Model;

class Node {
public:
	Node(int nodeNum);

	// Getters
	glm::mat4 const& getNodeMatrix() const { return nodeMatrix_; }
	glm::mat4 const& getLocalTRSMatrix() const { return localTRSMatrix_; }

	// Operations
	void updateLocalTRSMatrix();
	void updateNodeMatrix(glm::mat4 const& parentMatrix);

	// Hierarchy
	int nodeNum{-1};
	std::string nodeName;
	std::vector<std::shared_ptr<Node>> children;

	// Local transform components
	glm::vec3 translation{0.0f};
	glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
	glm::vec3 scale{1.0f};

private:
	// Matrices
	glm::mat4 localTRSMatrix_{1.0f};
	glm::mat4 nodeMatrix_{1.0f};
};

namespace NodeUtil {

// Update node tree matrices
void updateNodeTreeMatricesRecursive(std::shared_ptr<Node> node, glm::mat4 const& parentMatrix);
void updateNodeListLocalTRSMatrix(std::vector<std::shared_ptr<Node>> const& nodes);
void updateNodeListJointMatrices(Model& model);
std::shared_ptr<Node> createRoot(int nodeNum);
} // namespace NodeUtil
