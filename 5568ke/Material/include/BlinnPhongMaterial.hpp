/* include/BlinnPhongMaterial.h */
#pragma once

#include "Material.hpp"
#include "Texture.hpp"

class BlinnPhongMaterial : public Material {
public:
	BlinnPhongMaterial();
	void bind(Camera const&, glm::mat4 const&, std::vector<Light> const&) const override;

private:
	Shader shader_;
};
