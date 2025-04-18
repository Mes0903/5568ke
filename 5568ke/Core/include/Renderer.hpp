#pragma once

#include <glm/vec3.hpp>
#include <vector>

#include "Camera.hpp"
#include "Material.hpp"
#include "Model.hpp"

class Renderer {
public:
	void beginFrame(int w, int h, glm::vec3 const& clear);
	void submit(Model const& model, Material const& mat, glm::mat4 const& modelMatrix, Camera const&, std::vector<Light> const&);
	void endFrame();
};
