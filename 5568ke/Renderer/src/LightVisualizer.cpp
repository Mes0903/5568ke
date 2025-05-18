#include "LightVisualizer.hpp"

#include "Scene.hpp"
#include "Shader.hpp"
#include "include_5568ke.hpp"

LightVisualizer& LightVisualizer::getInstance()
{
	static LightVisualizer viz;
	return viz;
}

void LightVisualizer::init()
{
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

void LightVisualizer::cleanup()
{
	if (vbo_)
		glDeleteBuffers(1, &vbo_);
	if (vao_)
		glDeleteVertexArrays(1, &vao_);
}

void LightVisualizer::drawLights(Scene const& scene, glm::mat4 const& view, glm::mat4 const& proj, std::shared_ptr<Shader> shader)
{
	if (!shader || scene.lights.empty())
		return;

	std::vector<glm::vec3> pts;
	for (auto const& l : scene.lights)
		pts.emplace_back(l.position);

	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(glm::vec3), pts.data(), GL_DYNAMIC_DRAW);

	shader->bind();
	shader->sendMat4("view", view);
	shader->sendMat4("proj", proj);
	shader->sendFloat("pointSize", 50.0f); // see shader below

	// disable depth just like skeleton (so gizmos are always visible)
	GLboolean depth;
	glGetBooleanv(GL_DEPTH_TEST, &depth);
	glDisable(GL_DEPTH_TEST);

	glDrawArrays(GL_POINTS, 0, (GLsizei)pts.size());

	if (depth)
		glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
}
