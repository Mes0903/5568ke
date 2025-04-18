#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
struct GLFWwindow;

class Camera {
public:
	void processKeyboard(float dt, GLFWwindow* w);
	void processMouse(double xpos, double ypos);
	void updateMatrices(GLFWwindow* w); // builds view + proj

	glm::mat4 view() const { return view_; }
	glm::mat4 proj() const { return proj_; }
	glm::vec3 position() const { return pos_; }

private:
	// ---- state ----
	glm::vec3 pos_{0.0f, 1.6f, 3.0f};
	float yaw_ = -90.0f; // look -Z in OpenGL
	float pitch_ = 0.0f;
	glm::vec3 front_{0.0f, 0.0f, -1.0f};

	// ---- cached matrices ----
	glm::mat4 view_{1.0f};
	glm::mat4 proj_{1.0f};
};
