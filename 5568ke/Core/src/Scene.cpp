#define GLM_ENABLE_EXPERIMENTAL

#include "Scene.hpp"

#include <algorithm>
#include <iostream>
#include <ranges>

#include <glm/gtc/matrix_transform.hpp>

#include "BoundingBox.hpp"
#include "GlobalAnimationState.hpp"
#include "Model.hpp"
#include "include_5568ke.hpp"

void Camera::processKeyboard(float dt, GLFWwindow* w)
{
	glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));

	float camSpeed = GlobalAnimationState::getInstance().camSpeed;

	if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)
		pos += front * camSpeed * dt;
	if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)
		pos -= front * camSpeed * dt;
	if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)
		pos -= right * camSpeed * dt;
	if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)
		pos += right * camSpeed * dt;
}

void Camera::processMouse(double xpos, double ypos)
{
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
	yaw += dx * sensitivity;
	pitch += dy * sensitivity;

	pitch = std::clamp(pitch, -89.0f, 89.0f);

	// update front vector
	front =
			glm::normalize(glm::vec3(cos(glm::radians(pitch)) * cos(glm::radians(yaw)), sin(glm::radians(pitch)), cos(glm::radians(pitch)) * sin(glm::radians(yaw))));
}

void Camera::updateMatrices(GLFWwindow* w)
{
	int fbW, fbH;
	glfwGetFramebufferSize(w, &fbW, &fbH);

	// Prevent division by zero or negative aspect ratio
	if (fbW <= 0 || fbH <= 0) {
		std::cout << "[Camera] Warning: Invalid framebuffer size: " << fbW << "x" << fbH << std::endl;
		fbW = fbW <= 0 ? 1 : fbW;
		fbH = fbH <= 0 ? 1 : fbH;
	}

	view = glm::lookAt(pos, pos + front, glm::vec3(0, 1, 0));
	proj = glm::perspective(glm::radians(45.0f), float(fbW) / fbH, 0.1f, 100.f);
}

// Implementation of new Camera methods
void Camera::lookAt(glm::vec3 const& position, glm::vec3 const& target)
{
	pos = position;

	// Calculate front direction
	front = glm::normalize(target - position);

	// Calculate pitch and yaw from front vector
	pitch = glm::degrees(asin(front.y));
	yaw = glm::degrees(atan2(front.z, front.x));

	// Update matrices
	view = glm::lookAt(pos, pos + front, glm::vec3(0.0f, 1.0f, 0.0f));
}

// Update camera position to orbit around a target at a fixed distance
void Camera::updateFollow(glm::vec3 const& target, float distance, float height)
{
	glm::vec3 center = target + glm::vec3(0.0f, height, 0.0f);

	// Spherical to Cartesian
	glm::vec3 offset(distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw)), distance * sin(glm::radians(pitch)),
									 distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw)));

	pos = center - offset;
	front = glm::normalize(center - pos);
}

Scene& Scene::getInstance()
{
	static Scene instance;
	return instance;
}

// Scene methods implementation for camera setup
void Scene::setupCameraToViewScene(float padding)
{
	if (ents.empty()) {
		cam.pos = glm::vec3(0.0f, 1.6f, 3.0f);
		return;
	}

	// Find the global bounds of all entities
	BoundingBox worldBounds;
	worldBounds.min = glm::vec3(std::numeric_limits<float>::max());
	worldBounds.max = glm::vec3(std::numeric_limits<float>::lowest());

	for (Entity const& entity : ents) {
		if (!entity.visible || !entity.model)
			continue;

		BoundingBox local = entity.model->localSpaceBBox;

		// 8 corner
		glm::vec3 corners[8] = {
				{local.min.x, local.min.y, local.min.z}, {local.max.x, local.min.y, local.min.z}, {local.min.x, local.max.y, local.min.z},
				{local.max.x, local.max.y, local.min.z}, {local.min.x, local.min.y, local.max.z}, {local.max.x, local.min.y, local.max.z},
				{local.min.x, local.max.y, local.max.z}, {local.max.x, local.max.y, local.max.z},
		};

		glm::mat4 toWorldMatrix = entity.transform; // Model matrix of MVP transformation

		for (glm::vec3 c : corners) {
			glm::vec3 wp = glm::vec3(toWorldMatrix * glm::vec4(c, 1.0f));
			worldBounds.min = glm::min(worldBounds.min, wp);
			worldBounds.max = glm::max(worldBounds.max, wp);
		}
	}

	// Calculate scene center and bounding radius
	glm::vec3 worldCenter = (worldBounds.min + worldBounds.max) * 0.5f;
	glm::vec3 size = worldBounds.max - worldBounds.min;
	float radius = std::max({size.x, size.y, size.z}) * 0.5f * padding;

	// Position camera so the entire scene fits the view frustum
	float fov = glm::radians(45.0f);
	float distance = radius / std::tan(fov * 0.5f);
	glm::vec3 cameraPos = worldCenter + glm::vec3(0.0f, radius * 0.1f, distance);

	// Set camera to look at the center of the scene
	cam.lookAt(cameraPos, worldCenter);
}

void Scene::setupCameraToViewEntity(std::string const& entityName, float padding)
{
	auto entOpt = findEntity(entityName);
	if (!entOpt || !entOpt->get().model) {
		setupCameraToViewScene();
		return;
	}

	Entity const& entity = entOpt->get();

	BoundingBox worldBounds;
	worldBounds.min = glm::vec3(std::numeric_limits<float>::max());
	worldBounds.max = glm::vec3(std::numeric_limits<float>::lowest());

	BoundingBox local = entity.model->localSpaceBBox;
	glm::mat4 toWorldMatrix = entity.transform; // Model matrix of MVP transformation

	glm::vec3 corners[8] = {
			{local.min.x, local.min.y, local.min.z}, {local.max.x, local.min.y, local.min.z}, {local.min.x, local.max.y, local.min.z},
			{local.max.x, local.max.y, local.min.z}, {local.min.x, local.min.y, local.max.z}, {local.max.x, local.min.y, local.max.z},
			{local.min.x, local.max.y, local.max.z}, {local.max.x, local.max.y, local.max.z},
	};

	for (glm::vec3 c : corners) {
		glm::vec3 wp = glm::vec3(toWorldMatrix * glm::vec4(c, 1.0f));
		worldBounds.min = glm::min(worldBounds.min, wp);
		worldBounds.max = glm::max(worldBounds.max, wp);
	}

	// Calculate scene center and bounding radius
	glm::vec3 worldCenter = (worldBounds.min + worldBounds.max) * 0.5f;
	glm::vec3 size = worldBounds.max - worldBounds.min;
	float radius = std::max({size.x, size.y, size.z}) * 0.5f * padding;

	// Position camera so the entire scene fits the view frustum
	float fov = glm::radians(45.0f);
	float distance = radius / std::tan(fov * 0.5f);
	glm::vec3 cameraPos = worldCenter + glm::vec3(0.0f, radius * 0.1f, distance);

	// Set camera to look at the center of the scene
	cam.lookAt(cameraPos, worldCenter);
}

// Implementation for finding entity by name
std::optional<std::reference_wrapper<Entity>> Scene::findEntity(std::string const& name)
{
	auto it = std::ranges::find_if(ents, [&](Entity const& e) { return e.model && e.model->modelName == name; });
	if (it == ents.end())
		return std::nullopt;

	return *it; // implicit conversion to std::reference_wrapper
}

// Implementation for adding entity with tracking by name
void Scene::addEntity(std::shared_ptr<Model> model)
{
	if (!model)
		return;

	// Create a new entity
	Entity entity;
	entity.model = model;

	// Add to entities vector
	ents.push_back(entity);
}

// Implementation for removing entity
void Scene::removeEntity(std::string const& name)
{
	std::erase_if(ents, [&](Entity const& e) { return e.model && e.model->modelName == name; });
}

// Implementation for adding light
void Scene::addLight(glm::vec3 const& position, glm::vec3 const& color, float intensity)
{
	Light light;
	light.position = position;
	light.color = color;
	light.intensity = intensity;
	lights.push_back(std::move(light));
}

// Scene cleanup
void Scene::cleanup() {}
Scene::~Scene() { cleanup(); }
