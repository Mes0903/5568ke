#define GLM_ENABLE_EXPERIMENTAL

#include "Model.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "AnimationClip.hpp"
#include "Mesh.hpp"
#include "Node.hpp"
#include "Shader.hpp"

Model::~Model() { cleanup(); }

void Model::draw(Shader const& shader, glm::mat4 const& modelMatrix) const
{
	// Set the model matrix
	shader.sendMat4("model", modelMatrix);

	// For skinned models, send joint matrices to shader
	if (!jointMatrices.empty() && animations.size() > 0) {
		// Enable skinning
		shader.sendBool("enableSkinning", true);

		// Send joint matrices to shader
		for (size_t i = 0; i < jointMatrices.size() && i < 100; i++) {
			std::string uniformName = "jointMatrices[" + std::to_string(i) + "]";
			shader.sendMat4(uniformName.c_str(), jointMatrices[i]);
		}
	}
	else {
		// Disable skinning for static meshes
		shader.sendBool("enableSkinning", false);
	}

	// Handle each mesh
	for (size_t i = 0; i < meshes.size(); i++) {
		glm::mat4 finalTransform = modelMatrix;

		// If this is a static mesh and has a node associated with it
		if (i < meshNodeIndices.size()) {
			int nodeIndex = meshNodeIndices[i];
			if (nodeIndex >= 0 && nodeIndex < nodes.size() && nodes[nodeIndex]) {
				// Apply node's transform to the model matrix
				finalTransform = modelMatrix * nodes[nodeIndex]->getNodeMatrix();
				shader.sendMat4("model", finalTransform);
			}
		}

		// Draw the mesh
		meshes[i].draw(shader);
	}
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
}

// Support animation functionality
void Model::updateJointMatrices()
{
	if (!rootNode) {
		return;
	}

	std::cout << "[Model] Updating joint matrices" << std::endl;

	// First update all local matrices
	for (auto& node : nodes) {
		if (node) {
			node->calculateLocalTRSMatrix();
		}
	}

	// Now update all global matrices in a hierarchical way
	NodeUtil::updateNodeMatricesRecursive(rootNode, glm::mat4(1.0f));

	// Finally, update the joint matrices
	updateJointMatricesFromNodes();
}

void Model::updateJointMatricesFromNodes()
{
	// Update joint matrices if this node is a joint
	for (auto const& node : nodes) {
		if (!node)
			continue;

		int nodeIndex = node->nodeNum;
		if (nodeIndex < nodeToJointMapping.size()) {
			int jointIndex = nodeToJointMapping[nodeIndex];
			if (jointIndex >= 0 && jointIndex < jointMatrices.size() && jointIndex < inverseBindMatrices.size()) {
				jointMatrices[jointIndex] = node->getNodeMatrix() * inverseBindMatrices[jointIndex];
			}
		}
	}
}

void Model::initializeDefaultPose()
{
	// Make sure all nodes have proper local matrices calculated
	for (auto& node : nodes) {
		if (node) {
			node->calculateLocalTRSMatrix();
		}
	}

	// Update global matrices starting from the root
	if (rootNode) {
		NodeUtil::updateNodeMatricesRecursive(rootNode, glm::mat4(1.0f));
	}

	// Update joint matrices based on the default pose
	updateJointMatricesFromNodes();

	std::cout << "[Model] Initialized default pose with " << jointMatrices.size() << " joint matrices" << std::endl;
}

namespace ModelUtil {
namespace {
BoundingBox transformBBox(BoundingBox const& in, glm::mat4 const& M)
{
	BoundingBox out;
	out.min = glm::vec3(std::numeric_limits<float>::max());
	out.max = glm::vec3(std::numeric_limits<float>::lowest());

	// 8 個 corner
	for (int c = 0; c < 8; ++c) {
		glm::vec3 p = {(c & 1 ? in.max.x : in.min.x), (c & 2 ? in.max.y : in.min.y), (c & 4 ? in.max.z : in.min.z)};
		p = glm::vec3(M * glm::vec4(p, 1.0f));
		out.min = glm::min(out.min, p);
		out.max = glm::max(out.max, p);
	}
	return out;
}
} // namespace

BoundingBox getMeshBBox(Mesh const& mesh)
{
	BoundingBox bbox;
	if (mesh.vertices.empty()) {
		bbox.min = glm::vec3(0.0f);
		bbox.max = glm::vec3(0.0f);
		return bbox;
	}

	bbox.min = glm::vec3(std::numeric_limits<float>::max());
	bbox.max = glm::vec3(std::numeric_limits<float>::lowest());

	for (auto const& vertex : mesh.vertices) {
		bbox.min.x = std::min(bbox.min.x, vertex.position.x);
		bbox.min.y = std::min(bbox.min.y, vertex.position.y);
		bbox.min.z = std::min(bbox.min.z, vertex.position.z);

		bbox.max.x = std::max(bbox.max.x, vertex.position.x);
		bbox.max.y = std::max(bbox.max.y, vertex.position.y);
		bbox.max.z = std::max(bbox.max.z, vertex.position.z);
	}

	return bbox;
}

void setLocalBBox(Model& m)
{
	BoundingBox global;
	global.min = glm::vec3(std::numeric_limits<float>::max());
	global.max = glm::vec3(std::numeric_limits<float>::lowest());

	for (size_t i = 0; i < m.meshes.size(); ++i) {
		BoundingBox local = m.boundingBoxes[i];
		glm::mat4 nodeM(1.0f);

		if (i < m.meshNodeIndices.size()) {
			int nodeIdx = m.meshNodeIndices[i];
			if (nodeIdx >= 0 && nodeIdx < m.nodes.size() && m.nodes[nodeIdx])
				nodeM = m.nodes[nodeIdx]->getNodeMatrix();
		}
		BoundingBox world = transformBBox(local, nodeM);

		global.min = glm::min(global.min, world.min);
		global.max = glm::max(global.max, world.max);
	}
	m.localSpaceBBox = global;
}
} // namespace ModelUtil