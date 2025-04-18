/* include/Material.h */
#pragma once
#include <glm/mat4x4.hpp>
#include <vector>
#include "Camera.hpp"
#include "Shader.hpp"

struct Light {
	glm::vec3 position{2, 5, 2};
};

class Material {
public:
	virtual ~Material() = default;
	virtual void bind(Camera const& cam, glm::mat4 const& model, std::vector<Light> const& lights) const = 0;
};
