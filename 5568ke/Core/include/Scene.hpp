#pragma once

#include <glm/mat4x4.hpp>
#include <vector>

#include "Material.hpp"
#include "Model.hpp"

struct Entity {
	Model* model;
	Material* mat;
	glm::mat4 transform{1.0f};
};

class Scene {
public:
	std::vector<Entity>& entities() { return ents_; }
	std::vector<Entity> const& entities() const { return ents_; }

	std::vector<Light>& lights() { return lights_; }
	std::vector<Light> const& lights() const { return lights_; }

private:
	std::vector<Entity> ents_;
	std::vector<Light> lights_{{{2, 5, 2}}};
};
