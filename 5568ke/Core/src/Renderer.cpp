#include <glad/glad.h>

#include "Renderer.hpp"

void Renderer::beginFrame(int w, int h, glm::vec3 const& c) {
	glViewport(0, 0, w, h);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ← NEW
	glClearColor(c.r, c.g, c.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::submit(Model const& model, Material const& mat, glm::mat4 const& M, Camera const& cam, std::vector<Light> const& lights) {
	// delegate uniform work to the material
	mat.bind(cam, M, lights);

	// draw every mesh the model owns
	auto const& meshes = model.meshes();
	auto const& textures = model.textures();

	for (size_t i = 0; i < meshes.size(); ++i) {
		// base
		if (i * 2 < textures.size())
			textures[i * 2].bind(0);
		// eye overlay (same mesh index, next texture slot)
		if (i * 2 + 1 < textures.size())
			textures[i * 2 + 1].bind(1);

		meshes[i].bind();
		glDrawElements(GL_TRIANGLES, meshes[i].indexCount(), GL_UNSIGNED_INT, nullptr);
	}
}

void Renderer::endFrame() {
	glBindVertexArray(0);
	glUseProgram(0);
}
