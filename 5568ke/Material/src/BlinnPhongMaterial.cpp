#include <glm/gtc/type_ptr.hpp>

#include "BlinnPhongMaterial.hpp"

BlinnPhongMaterial::BlinnPhongMaterial() { shader_.resetShader("assets/shaders/blinn.vert", "assets/shaders/blinn.frag"); }

void BlinnPhongMaterial::bind(Camera const& cam, glm::mat4 const& model, std::vector<Light> const& lights) const {
	shader_.bind();
	shader_.setMat4("model", glm::value_ptr(model));
	shader_.setMat4("view", glm::value_ptr(cam.view()));
	shader_.setMat4("proj", glm::value_ptr(cam.proj()));
	shader_.setVec3("viewPos", glm::value_ptr(cam.position()));
	shader_.setVec3("lightPos", glm::value_ptr(lights[0].position));
}
