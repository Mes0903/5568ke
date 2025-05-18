#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "include_5568ke.hpp"

class Shader;
class Scene;
class Model;
struct Entity;

class BoundingBoxVisualizer {
public:
	static BoundingBoxVisualizer& get();

	void init();		// create VAO / VBO
	void cleanup(); // destroy GL objects
	void draw(Scene const& scene, glm::mat4 const& view, glm::mat4 const& proj);

	std::shared_ptr<Shader> boxShader;

private:
	BoundingBoxVisualizer() = default;
	GLuint vao_{0}, vbo_{0};
};
