#include "BoundingBoxVisualizer.hpp"

#include "Model.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "include_5568ke.hpp"

BoundingBoxVisualizer& BoundingBoxVisualizer::getInstance()
{
	static BoundingBoxVisualizer instance;
	return instance;
}

void BoundingBoxVisualizer::init()
{
	boxShader = std::make_shared<Shader>();
	boxShader->resetShaderPath("assets/shaders/boundingBox.vert", "assets/shaders/boundingBox.frag");

	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);

	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);

	// pos only (vec3) – colour is set in boxShader constant
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindVertexArray(0);
}

void BoundingBoxVisualizer::cleanup()
{
	if (vbo_)
		glDeleteBuffers(1, &vbo_);
	if (vao_)
		glDeleteVertexArrays(1, &vao_);
}

static void buildBoxLines(glm::vec3 const& min, glm::vec3 const& max, std::vector<glm::vec3>& out)
{
	using glm::vec3;
	vec3 v000(min.x, min.y, min.z);
	vec3 v001(min.x, min.y, max.z);
	vec3 v010(min.x, max.y, min.z);
	vec3 v011(min.x, max.y, max.z);
	vec3 v100(max.x, min.y, min.z);
	vec3 v101(max.x, min.y, max.z);
	vec3 v110(max.x, max.y, min.z);
	vec3 v111(max.x, max.y, max.z);

	// 12 edges – push as line-list
	auto push = [&](vec3 const& a, vec3 const& b) {
		out.push_back(a);
		out.push_back(b);
	};
	push(v000, v001);
	push(v001, v011);
	push(v011, v010);
	push(v010, v000); // left
	push(v100, v101);
	push(v101, v111);
	push(v111, v110);
	push(v110, v100); // right
	push(v000, v100);
	push(v001, v101);
	push(v011, v111);
	push(v010, v110); // connect
}

void BoundingBoxVisualizer::draw(Scene const& scene)
{
	if (!boxShader)
		return;

	std::vector<glm::vec3> verts;
	verts.reserve(scene.gameObjects.size() * 24);

	for (auto const& goPtr : scene.gameObjects) {
		if (!goPtr->visible || !goPtr->getModel())
			continue;

		auto const& bb = goPtr->worldBBox;
		buildBoxLines(bb.min, bb.max, verts);
	}

	if (verts.empty())
		return;

	// upload once per frame
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), verts.data(), GL_DYNAMIC_DRAW);

	// simple colour – bright magenta
	boxShader->bind();
	boxShader->sendMat4("view", scene.cam.view);
	boxShader->sendMat4("proj", scene.cam.proj);
	boxShader->sendMat4("model", glm::mat4(1.0f));
	boxShader->sendVec3("uColor", glm::vec3(1, 1, 1));

	GLboolean depth;
	glGetBooleanv(GL_DEPTH_TEST, &depth);
	glDisable(GL_DEPTH_TEST);

	glDrawArrays(GL_LINES, 0, (GLsizei)verts.size());

	if (depth)
		glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
}
