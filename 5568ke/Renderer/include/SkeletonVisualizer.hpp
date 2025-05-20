#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

class Shader;
class Node;
class Model;
class Camera;
class Entity;

/**
 * @brief Helper class for visualizing skeletal animations
 *
 */
class SkeletonVisualizer {
public:
	static SkeletonVisualizer& getInstance();
	void init();
	void cleanup();

	bool hasSkeletonData(std::shared_ptr<Model> model) const;
	void generateSkeletonData(std::shared_ptr<Model> model);
	void draw(Entity const& entity, Camera const& cam); // Draw simple debug lines with axes, grid, etc.

	std::shared_ptr<Shader> skeletonShader;

private:
	SkeletonVisualizer() = default;
	~SkeletonVisualizer() = default;

	// Helper methods for visualization
	void processNodeTreePositionsRecursive(std::shared_ptr<Node> node, std::vector<glm::vec3>& vertices, std::vector<glm::vec3>& colors, float nodePosScale);
	void addDotJoint(glm::vec3 const& position, float radius, glm::vec3 const& color, std::vector<glm::vec3>& vertices, std::vector<glm::vec3>& colors);

	// OpenGL resources
	GLuint vao_{};
	GLuint vbo_{};
	float jointRadius_{0.01f};

	// Skeleton data
	struct SkeletonData {
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> colors;
	};

	// Cache of skeleton data by model
	std::unordered_map<std::shared_ptr<Model>, SkeletonData> skeletonCache;
};
