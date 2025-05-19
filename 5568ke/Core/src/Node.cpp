#define GLM_ENABLE_EXPERIMENTAL

#include "Node.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "Model.hpp"

namespace NodeUtil {
void updateNodeTreeMatricesRecursive(std::shared_ptr<Node> node, glm::mat4 const& parentMatrix)
{
	if (!node) {
		return;
	}

	// Calculate global matrix
	node->updateNodeMatrix(parentMatrix);

	// Process all children
	for (auto const& child : node->children) {
		updateNodeTreeMatricesRecursive(child, node->getNodeMatrix());
	}
}

void updateNodeListLocalTRSMatrix(std::vector<std::shared_ptr<Node>> const& nodes)
{
	for (auto const& node : nodes) {
		if (node) {
			node->updateLocalTRSMatrix();
		}
	}
}

void updateNodeListJointMatrices(Model& model)
{
	// Update the joint matrices
	for (auto const& node : model.nodes) {
		if (!node)
			continue;

		int nodeIndex = node->nodeNum;
		if (nodeIndex < model.nodeToJointMapping.size()) {
			int jointIndex = model.nodeToJointMapping[nodeIndex];
			if (jointIndex >= 0 && jointIndex < model.jointMatrices.size() && jointIndex < model.inverseBindMatrices.size()) {
				model.jointMatrices[jointIndex] = node->getNodeMatrix() * model.inverseBindMatrices[jointIndex];
			}
		}
	}
}

std::shared_ptr<Node> createRoot(int nodeNum) { return std::make_shared<Node>(nodeNum); }
} // namespace NodeUtil

Node::Node(int nodeNum) : nodeNum(nodeNum) {}

void Node::updateLocalTRSMatrix()
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

void Node::updateNodeMatrix(glm::mat4 const& parentMatrix)
{
	// Multiply parent matrix by local matrix to get global matrix
	nodeMatrix_ = parentMatrix * localTRSMatrix_;

	// Debug log the result if it's significantly different from identity
	glm::vec3 position = glm::vec3(nodeMatrix_[3]);
	if (glm::length(position) > 0.01f) {
		std::cout << "[Node INFO] Node " << nodeNum << " local space position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
	}
}
