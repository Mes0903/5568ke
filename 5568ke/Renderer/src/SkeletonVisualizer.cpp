#define GLM_ENABLE_EXPERIMENTAL

#include "SkeletonVisualizer.hpp"

#include <iostream>

#include "Model.hpp"
#include "Node.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "Shader.hpp"

SkeletonVisualizer& SkeletonVisualizer::getInstance()
{
	static SkeletonVisualizer instance;
	return instance;
}

void SkeletonVisualizer::init()
{
	// Create line shader for debug visualization
	skeletonShader = std::make_unique<Shader>();
	skeletonShader->resetShaderPath("assets/shaders/skeleton.vert", "assets/shaders/skeleton.frag");

	// Create OpenGL resources
	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);

	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);

	// Position attribute (for line positions)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, (void*)0);

	// Color attribute (for line colors)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, (void*)(sizeof(glm::vec3)));

	glBindVertexArray(0);
}

void SkeletonVisualizer::cleanup()
{
	skeletonCache.clear();

	if (vao_ != 0) {
		glDeleteVertexArrays(1, &vao_);
		vao_ = 0;
	}

	if (vbo_ != 0) {
		glDeleteBuffers(1, &vbo_);
		vbo_ = 0;
	}
}

bool SkeletonVisualizer::hasSkeletonData(std::shared_ptr<Model> model) const
{
	if (!model)
		return false;

	// A model has skeleton data if it has animations
	// Even if it has no joint matrices yet (they might be created when animation plays)
	return !model->animations.empty();
}

void SkeletonVisualizer::generateSkeletonData(std::shared_ptr<Model> model)
{
	if (!model) {
		std::cout << "[SkeletonVisualizer] Cannot generate skeleton data: null model" << std::endl;
		return;
	}

	if (!model->rootNode) {
		std::cout << "[SkeletonVisualizer] Cannot generate skeleton data: null rootNode" << std::endl;
		return;
	}

	// Check if we already have data for this model
	auto it = skeletonCache.find(model);
	if (it != skeletonCache.end()) {
		std::cout << "[SkeletonVisualizer] Using cached skeleton data for model" << std::endl;
		return; // Already generated
	}

	// Create new skeleton data
	SkeletonData skeletonData;
	std::cout << "[SkeletonVisualizer] Generating skeleton data for model with " << model->nodes.size() << " nodes" << std::endl;

	// Process the node hierarchy recursively starting from the root
	float nodePosScale = 0.005f;
	processNodeTreePositionsRecursive(model->rootNode, skeletonData.vertices, skeletonData.colors, nodePosScale);

	std::cout << "[SkeletonVisualizer] Generated " << skeletonData.vertices.size() << " vertices for skeleton lines" << std::endl;

	// Cache the data
	skeletonCache[model] = skeletonData;
}

void SkeletonVisualizer::processNodeTreePositionsRecursive(std::shared_ptr<Node> node, std::vector<glm::vec3>& vertices, std::vector<glm::vec3>& colors,
																													 float nodePosScale)
{
	if (!node)
		return;

	// Get node position
	glm::mat4 nodeMatrix = node->getNodeMatrix();
	glm::vec3 nodePos = glm::vec3(nodeMatrix[3]);

	// Skip nodes with zero position (might be invalid)
	if (glm::length(nodePos) < 0.001f) {
		// Process children anyway
		for (auto& child : node->children) {
			processNodeTreePositionsRecursive(child, vertices, colors, nodePosScale);
		}
		return;
	}

	// Scale position for visualization
	glm::vec3 scaledPos = nodePos * nodePosScale;

	// Determine joint color based on node type
	glm::vec3 color;
	std::string nodeName = node->nodeName;

	// More distinctive colors
	if (nodeName.find("spine") != std::string::npos) {
		color = glm::vec3(0.0f, 1.0f, 0.0f); // Bright green for spine
	}
	else if (nodeName.find("arm") != std::string::npos || nodeName.find("hand") != std::string::npos) {
		color = glm::vec3(0.0f, 0.6f, 1.0f); // Bright blue for arms
	}
	else if (nodeName.find("leg") != std::string::npos || nodeName.find("foot") != std::string::npos) {
		color = glm::vec3(1.0f, 0.5f, 0.0f); // Orange for legs
	}
	else if (nodeName.find("head") != std::string::npos || nodeName.find("hair") != std::string::npos) {
		color = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow for head
	}
	else {
		color = glm::vec3(1.0f, 0.4f, 0.7f); // Pink for other bones
	}

	addDotJoint(scaledPos, jointRadius_, color, vertices, colors);

	// Add lines connecting this node to its children
	for (auto& child : node->children) {
		if (!child)
			continue;

		glm::mat4 childMatrix = child->getNodeMatrix();
		glm::vec3 childPos = glm::vec3(childMatrix[3]);

		// Only draw connections to nodes with valid positions
		if (glm::length(childPos) < 0.001f)
			continue;

		glm::vec3 scaledChildPos = childPos * nodePosScale;

		// Add line from node to child
		vertices.push_back(scaledPos);
		vertices.push_back(scaledChildPos);

		// Add bone colors - make the bone colors brigher
		glm::vec3 boneColor = color * 0.8f + glm::vec3(0.2f); // Brighten
		colors.push_back(boneColor);
		colors.push_back(boneColor);
	}

	// Process children recursively
	for (auto& child : node->children) {
		processNodeTreePositionsRecursive(child, vertices, colors, nodePosScale);
	}
}

void SkeletonVisualizer::draw(GameObject const& gameObject, Camera const& cam)
{
	auto model = gameObject.getModel();

	if (!model || !skeletonShader) {
		std::cout << "[SkeletonVisualizer ERROR] Model or skeleton shader is null!" << std::endl;
		return;
	}

	// Clear the cached skeleton data for this model to regenerate it with new parameters
	auto it = skeletonCache.find(gameObject.getModel());
	if (it != skeletonCache.end()) {
		skeletonCache.erase(it);
	}

	// Create diagnostic lines
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> colors;

	// Add node visualization
	if (model->rootNode) {
		// Use the same scale factor for skeleton as for the model
		float nodePosScale = 1.0f; // This will be applied with the model matrix
		processNodeTreePositionsRecursive(model->rootNode, vertices, colors, nodePosScale);
	}

	// Skip if no vertices
	if (vertices.empty()) {
		std::cout << "[SkeletonVisualizer] No vertices to draw" << std::endl;
		return;
	}

	// Prepare interleaved data
	std::vector<glm::vec3> interleavedData;
	for (size_t i = 0; i < vertices.size(); i++) {
		interleavedData.push_back(vertices[i]);
		if (i < colors.size()) {
			interleavedData.push_back(colors[i]);
		}
		else {
			interleavedData.push_back(glm::vec3(1.0f, 1.0f, 1.0f)); // Default white
		}
	}

	// Update buffer data
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(glm::vec3), interleavedData.data(), GL_DYNAMIC_DRAW);

	// Bind shader and set uniforms
	skeletonShader->bind();
	skeletonShader->sendMat4("view", cam.view);
	skeletonShader->sendMat4("proj", cam.proj);
	skeletonShader->sendMat4("model", gameObject.getTransform());

	// Draw lines with wider lines for better visibility
	glLineWidth(3.0f); // Make lines thicker

	// Disable depth testing temporarily to ensure skeleton is visible
	GLboolean depthTestEnabled;
	glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);

	// Draw once with depth test to place lines correctly
	glDrawArrays(GL_LINES, 0, vertices.size());

	// Draw again without depth test to ensure visibility
	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_LINES, 0, vertices.size());

	// Restore original depth test state
	if (depthTestEnabled)
		glEnable(GL_DEPTH_TEST);

	glLineWidth(1.0f); // Reset line width

	// Unbind
	glBindVertexArray(0);
}

void SkeletonVisualizer::addDotJoint(glm::vec3 const& position, float radius, glm::vec3 const& color, std::vector<glm::vec3>& vertices,
																		 std::vector<glm::vec3>& colors)
{
	// Create a simple "+" marker at each joint position
	// The size of the marker is controlled by radius

	// Horizontal line (small)
	vertices.push_back(position - glm::vec3(radius, 0.0f, 0.0f));
	vertices.push_back(position + glm::vec3(radius, 0.0f, 0.0f));

	// Vertical line (small)
	vertices.push_back(position - glm::vec3(0.0f, radius, 0.0f));
	vertices.push_back(position + glm::vec3(0.0f, radius, 0.0f));

	// Add colors
	for (int i = 0; i < 4; i++) {
		colors.push_back(color);
	}

	// Optionally add a third axis for more visibility
	vertices.push_back(position - glm::vec3(0.0f, 0.0f, radius));
	vertices.push_back(position + glm::vec3(0.0f, 0.0f, radius));

	// Add colors
	colors.push_back(color);
	colors.push_back(color);
}
