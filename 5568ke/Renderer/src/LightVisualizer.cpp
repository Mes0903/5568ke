#include "LightVisualizer.hpp"

#include "Scene.hpp"
#include "Shader.hpp"
#include "include_5568ke.hpp"

LightPointVisualizer& LightPointVisualizer::getInstance()
{
	static LightPointVisualizer viz;
	return viz;
}

void LightPointVisualizer::init()
{
	lightPointShader = std::make_unique<Shader>();
	lightPointShader->resetShaderPath("assets/shaders/point.vert", "assets/shaders/point.frag");

	glEnable(GL_PROGRAM_POINT_SIZE);
	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	// pos
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindVertexArray(0);
}

void LightPointVisualizer::cleanup()
{
	if (vbo_)
		glDeleteBuffers(1, &vbo_);
	if (vao_)
		glDeleteVertexArrays(1, &vao_);
}

void LightPointVisualizer::draw(Scene const& scene)
{
	if (!lightPointShader || scene.lights.empty())
		return;

	std::vector<glm::vec3> pts;
	for (auto const& l : scene.lights)
		pts.emplace_back(l.position);

	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(glm::vec3), pts.data(), GL_DYNAMIC_DRAW);

	lightPointShader->bind();
	lightPointShader->sendMat4("view", scene.cam.view);
	lightPointShader->sendMat4("proj", scene.cam.proj);
	lightPointShader->sendFloat("pointSize", 50.0f); // see lightPointShader below

	// disable depth just like skeleton (so gizmos are always visible)
	GLboolean depth;
	glGetBooleanv(GL_DEPTH_TEST, &depth);
	glDisable(GL_DEPTH_TEST);

	glDrawArrays(GL_POINTS, 0, (GLsizei)pts.size());

	if (depth)
		glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
}
