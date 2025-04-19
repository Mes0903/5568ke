#pragma once

#include "Material.hpp"
#include "Texture.hpp"

class BlinnPhongMaterial : public Material {
public:
	glm::vec3 albedo{1.0f};
	float shininess{32.0f};
	Texture* diffuseMap = nullptr;
	Texture* overlayMap = nullptr;

	void bind(Shader& shader) const override;
};
