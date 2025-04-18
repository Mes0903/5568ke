#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

#include "Camera.hpp"

static float const CAM_SPEED = 3.0f; // m/s
static bool firstMouse = true;
static double lastX, lastY;

void Camera::processKeyboard(float dt, GLFWwindow* w) {
	glm::vec3 right = glm::normalize(glm::cross(front_, glm::vec3(0, 1, 0)));

	if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)
		pos_ += front_ * CAM_SPEED * dt;
	if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)
		pos_ -= front_ * CAM_SPEED * dt;
	if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)
		pos_ -= right * CAM_SPEED * dt;
	if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)
		pos_ += right * CAM_SPEED * dt;
}

void Camera::processMouse(double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
		return;
	}

	float dx = float(xpos - lastX);
	float dy = float(lastY - ypos); // reversed: y grows down
	lastX = xpos;
	lastY = ypos;

	float const sensitivity = 0.1f;
	yaw_ += dx * sensitivity;
	pitch_ += dy * sensitivity;

	pitch_ = std::clamp(pitch_, -89.0f, 89.0f);

	// update front vector
	front_ = glm::normalize(
			glm::vec3(cos(glm::radians(pitch_)) * cos(glm::radians(yaw_)), sin(glm::radians(pitch_)), cos(glm::radians(pitch_)) * sin(glm::radians(yaw_))));
}

void Camera::updateMatrices(GLFWwindow* w) {
	int fbW, fbH;
	glfwGetFramebufferSize(w, &fbW, &fbH);
	view_ = glm::lookAt(pos_, pos_ + front_, glm::vec3(0, 1, 0));
	proj_ = glm::perspective(glm::radians(45.0f), float(fbW) / fbH, 0.1f, 100.f);
}
