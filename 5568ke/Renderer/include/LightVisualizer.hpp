#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "include_5568ke.hpp"

class Shader;
class Scene;

class LightPointVisualizer {
public:
	static LightPointVisualizer& getInstance();
	void init();
	void cleanup();
	void draw(Scene const& scene);

	std::shared_ptr<Shader> lightPointShader;

private:
	GLuint vao_{0}, vbo_{0};
	LightPointVisualizer() = default;
};
