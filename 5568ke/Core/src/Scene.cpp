#include "include_5568ke.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

#include "BoundingBox.hpp"
#include "Model.hpp"
#include "Scene.hpp"

static float const CAM_SPEED = 3.0f; // m/s
static bool firstMouse = true;
static double lastX, lastY;

void Camera::processKeyboard(float dt, GLFWwindow* w)
{
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
	yaw_ += dx * sensitivity;
	pitch_ += dy * sensitivity;

	pitch_ = std::clamp(pitch_, -89.0f, 89.0f);

	// update front vector
	front_ = glm::normalize(
			glm::vec3(cos(glm::radians(pitch_)) * cos(glm::radians(yaw_)), sin(glm::radians(pitch_)), cos(glm::radians(pitch_)) * sin(glm::radians(yaw_))));
}

void Camera::updateMatrices(GLFWwindow* w)
{
	int fbW, fbH;
	glfwGetFramebufferSize(w, &fbW, &fbH);
	view_ = glm::lookAt(pos_, pos_ + front_, glm::vec3(0, 1, 0));
	proj_ = glm::perspective(glm::radians(45.0f), float(fbW) / fbH, 0.1f, 100.f);
}

// Implementation of new Camera methods
void Camera::lookAt(glm::vec3 const& position, glm::vec3 const& target)
{
	pos_ = position;

	// Calculate front direction
	front_ = glm::normalize(target - position);

	// Calculate pitch and yaw from front vector
	pitch_ = glm::degrees(asin(front_.y));
	yaw_ = glm::degrees(atan2(front_.z, front_.x));

	// Update matrices
	view_ = glm::lookAt(pos_, pos_ + front_, glm::vec3(0.0f, 1.0f, 0.0f));
}

// Scene methods implementation for camera setup
void Scene::setupCameraToViewScene(float padding)
{
	if (ents.empty()) {
		cam.pos_ = glm::vec3(0.0f, 1.6f, 3.0f);
		return;
	}

	// Find the global bounds of all entities
	BoundingBox globalBounds;
	globalBounds.min = glm::vec3(std::numeric_limits<float>::max());
	globalBounds.max = glm::vec3(std::numeric_limits<float>::lowest());

	for (Entity const& entity : ents) {
		if (!entity.visible || !entity.model)
			continue;

		// Transform the model's bounding box by the entity transform
		BoundingBox modelBounds = entity.model->globalBoundingBox;
		glm::vec3 corners[8] = {
				glm::vec3(modelBounds.min.x, modelBounds.min.y, modelBounds.min.z), glm::vec3(modelBounds.max.x, modelBounds.min.y, modelBounds.min.z),
				glm::vec3(modelBounds.min.x, modelBounds.max.y, modelBounds.min.z), glm::vec3(modelBounds.max.x, modelBounds.max.y, modelBounds.min.z),
				glm::vec3(modelBounds.min.x, modelBounds.min.y, modelBounds.max.z), glm::vec3(modelBounds.max.x, modelBounds.min.y, modelBounds.max.z),
				glm::vec3(modelBounds.min.x, modelBounds.max.y, modelBounds.max.z), glm::vec3(modelBounds.max.x, modelBounds.max.y, modelBounds.max.z)};

		// Transform each corner and expand global bounds
		for (int i = 0; i < 8; i++) {
			glm::vec4 transformedCorner = entity.transform * glm::vec4(corners[i], 1.0f);
			glm::vec3 worldPos = glm::vec3(transformedCorner) / transformedCorner.w;

			globalBounds.min = glm::min(globalBounds.min, worldPos);
			globalBounds.max = glm::max(globalBounds.max, worldPos);
		}
	}

	// Calculate scene center and dimensions
	glm::vec3 center = (globalBounds.min + globalBounds.max) * 0.5f;
	glm::vec3 size = globalBounds.max - globalBounds.min;
	float maxDim = std::max(std::max(size.x, size.y), size.z) * padding;

	// Position camera to view the entire scene
	float distance = maxDim;
	glm::vec3 cameraPos = center + glm::vec3(0.0f, size.y * 0.25f, distance);

	// Set camera to look at the center of the scene
	cam.lookAt(cameraPos, center);
}

void Scene::setupCameraToViewEntity(std::string const& entityName, float distance)
{
	Entity* entity = findEntity(entityName);
	if (!entity || !entity->model) {
		// Fall back to viewing the entire scene
		setupCameraToViewScene();
		return;
	}

	// Calculate entity center in world space
	BoundingBox& bbox = entity->model->globalBoundingBox;
	glm::vec3 modelCenter = (bbox.min + bbox.max) * 0.5f;
	glm::vec4 worldCenterHomogeneous = entity->transform * glm::vec4(modelCenter, 1.0f);
	glm::vec3 worldCenter = glm::vec3(worldCenterHomogeneous) / worldCenterHomogeneous.w;

	// Calculate entity size
	glm::vec3 size = bbox.max - bbox.min;
	float maxDim = std::max(std::max(size.x, size.y), size.z);

	// Adjust distance based on entity size if not explicitly specified
	float viewDistance = (distance <= 0.0f) ? maxDim * 2.0f : distance;

	// Position camera to view the entity
	glm::vec3 cameraPos = worldCenter + glm::vec3(0.0f, size.y * 0.1f, viewDistance);

	// Set camera to look at the entity center
	cam.lookAt(cameraPos, worldCenter);
}

// Implementation for finding entity by name
Entity* Scene::findEntity(std::string const& name)
{
	auto it = entityMap_.find(name);
	if (it != entityMap_.end() && it->second < ents.size()) {
		return &ents[it->second];
	}
	return nullptr;
}

// Implementation for adding entity with tracking by name
void Scene::addEntity(Model* model, glm::mat4 const& transform, std::string const& name)
{
	if (!model)
		return;

	// Create a new entity
	Entity entity;
	entity.model = model;
	entity.transform = transform;

	// Set name (use auto-generated if empty)
	entity.name = name.empty() ? "entity_" + std::to_string(ents.size()) : name;

	// Add to entities vector
	size_t index = ents.size();
	ents.push_back(entity);

	// Add to name lookup map
	entityMap_[entity.name] = index;
}

// Implementation for removing entity
void Scene::removeEntity(std::string const& name)
{
	auto it = entityMap_.find(name);
	if (it != entityMap_.end() && it->second < ents.size()) {
		size_t index = it->second;

		// Remove from entities vector
		ents.erase(ents.begin() + index);
		entityMap_.erase(it);

		// Update indices in the map
		for (auto& pair : entityMap_) {
			if (pair.second > index) {
				pair.second--;
			}
		}
	}
}

// Implementation for adding light
void Scene::addLight(glm::vec3 const& position, glm::vec3 const& color, float intensity)
{
	Light light;
	light.position = position;
	light.color = color;
	light.intensity = intensity;
	lights.push_back(light);
}

// Skybox implementation
void Scene::loadSkybox(std::string const& directory)
{
	// TODO: This should load 6 textures for a skybox. For now, just setting a flag
	hasSkybox_ = false;

	std::cout << "[Scene] Loading skybox from: " << directory << std::endl;
	// Actual implementation would load the 6 faces and set up the skybox texture
}

// Scene cleanup
void Scene::cleanup()
{
	// Clear all entities (but don't delete models - ModelLoader owns them)
	ents.clear();
	entityMap_.clear();

	// Clear lights
	lights.clear();

	// Clean up skybox if needed
	if (skyboxVAO_ != 0) {
		glDeleteVertexArrays(1, &skyboxVAO_);
		skyboxVAO_ = 0;
	}

	if (skyboxVBO_ != 0) {
		glDeleteBuffers(1, &skyboxVBO_);
		skyboxVBO_ = 0;
	}

	if (skyboxTexture_ != 0) {
		glDeleteTextures(1, &skyboxTexture_);
		skyboxTexture_ = 0;
	}

	hasSkybox_ = false;
}

Scene::~Scene() { cleanup(); }