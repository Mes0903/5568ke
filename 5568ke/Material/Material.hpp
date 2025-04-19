#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "Shader.hpp"
#include "Texture.hpp"

class Material {
public:
	virtual void bind(Shader& shader) const = 0;
};
