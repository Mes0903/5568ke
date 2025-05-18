#define GLM_ENABLE_EXPERIMENTAL

#include "Node.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

namespace NodeUtil {
void updateNodeMatricesRecursive(std::shared_ptr<Node> node, glm::mat4 const& parentMatrix)
{
	if (!node) {
		return;
	}

	// Calculate global matrix
	node->calculateNodeMatrix(parentMatrix);

	// Process all children
	for (auto const& child : node->children) {
		updateNodeMatricesRecursive(child, node->getNodeMatrix());
	}
}

std::shared_ptr<Node> createRoot(int nodeNum) { return std::make_shared<Node>(nodeNum); }
} // namespace NodeUtil

Node::Node(int nodeNum) : nodeNum(nodeNum) {}

void Node::calculateLocalTRSMatrix()
{
	// Create transform = T * R * S
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
	glm::mat4 rotationMatrix = glm::toMat4(rotation);
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	// Correct transformation order: first scale, then rotate, then translate
	localTRSMatrix_ = translationMatrix * rotationMatrix * scaleMatrix;

	// Debug log for non-identity matrices
	if (glm::length(translation) > 0.01f || std::abs(glm::length(rotation) - 1.0f) > 0.01f || glm::length(scale - glm::vec3(1.0f)) > 0.01f) {
		std::cout << "[Node INFO] Node " << nodeNum << " local matrix updated: "
							<< "T(" << translation.x << "," << translation.y << "," << translation.z << ") "
							<< "R(" << rotation.w << "," << rotation.x << "," << rotation.y << "," << rotation.z << ") "
							<< "S(" << scale.x << "," << scale.y << "," << scale.z << ")" << std::endl;
	}
}

void Node::calculateNodeMatrix(glm::mat4 const& parentMatrix)
{
	// Multiply parent matrix by local matrix to get global matrix
	nodeMatrix_ = parentMatrix * localTRSMatrix_;

	// Debug log the result if it's significantly different from identity
	glm::vec3 position = glm::vec3(nodeMatrix_[3]);
	if (glm::length(position) > 0.01f) {
		std::cout << "[Node INFO] Node " << nodeNum << " local space position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
	}
}
