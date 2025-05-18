#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "include_5568ke.hpp"

class Shader;
class Scene;

class LightVisualizer {
public:
	static LightVisualizer& getInstance(); // singleton
	void init();
	void cleanup();
	void drawLights(Scene const& scene, glm::mat4 const& view, glm::mat4 const& proj, std::shared_ptr<Shader> lineShader);

private:
	GLuint vao_{0}, vbo_{0};
	LightVisualizer() = default;
};
