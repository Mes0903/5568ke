#define GLM_ENABLE_EXPERIMENTAL

#include "Scene.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

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

// Scene methods implementation for camera setup
void Scene::setupCameraToViewScene(float padding)
{
	if (ents.empty()) {
		cam.pos = glm::vec3(0.0f, 1.6f, 3.0f);
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
		BoundingBox modelBounds = entity.model->localSpaceBBox;
		glm::mat4 worldM = entity.transform * glm::scale(glm::mat4(1.0f), glm::vec3(entity.scale));

		glm::vec3 corners[8] = {
				glm::vec3(modelBounds.min.x, modelBounds.min.y, modelBounds.min.z), glm::vec3(modelBounds.max.x, modelBounds.min.y, modelBounds.min.z),
				glm::vec3(modelBounds.min.x, modelBounds.max.y, modelBounds.min.z), glm::vec3(modelBounds.max.x, modelBounds.max.y, modelBounds.min.z),
				glm::vec3(modelBounds.min.x, modelBounds.min.y, modelBounds.max.z), glm::vec3(modelBounds.max.x, modelBounds.min.y, modelBounds.max.z),
				glm::vec3(modelBounds.min.x, modelBounds.max.y, modelBounds.max.z), glm::vec3(modelBounds.max.x, modelBounds.max.y, modelBounds.max.z)};

		// Transform each corner and expand global bounds
		for (int i = 0; i < 8; i++) {
			glm::vec4 transformedCorner = worldM * glm::vec4(corners[i], 1.0f);
			glm::vec3 worldPos = glm::vec3(transformedCorner) / transformedCorner.w;

			globalBounds.min = glm::min(globalBounds.min, worldPos);
			globalBounds.max = glm::max(globalBounds.max, worldPos);
		}
	}

	// Calculate scene center and bounding radius
	glm::vec3 center = (globalBounds.min + globalBounds.max) * 0.5f;
	glm::vec3 size = globalBounds.max - globalBounds.min;
	float radius = std::max({size.x, size.y, size.z}) * 0.5f * padding;

	// Position camera so the entire scene fits the view frustum
	float fov = glm::radians(45.0f);
	float distance = radius / std::tan(fov * 0.5f);
	glm::vec3 cameraPos = center + glm::vec3(0.0f, radius * 0.1f, distance);

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
	BoundingBox bbox = entity->model->localSpaceBBox;
	glm::mat4 worldM = entity->transform * glm::scale(glm::mat4(1.0f), glm::vec3(entity->scale));

	BoundingBox worldBounds;
	worldBounds.min = glm::vec3(std::numeric_limits<float>::max());
	worldBounds.max = glm::vec3(std::numeric_limits<float>::lowest());

	glm::vec3 corners[8] = {glm::vec3(bbox.min.x, bbox.min.y, bbox.min.z), glm::vec3(bbox.max.x, bbox.min.y, bbox.min.z),
													glm::vec3(bbox.min.x, bbox.max.y, bbox.min.z), glm::vec3(bbox.max.x, bbox.max.y, bbox.min.z),
													glm::vec3(bbox.min.x, bbox.min.y, bbox.max.z), glm::vec3(bbox.max.x, bbox.min.y, bbox.max.z),
													glm::vec3(bbox.min.x, bbox.max.y, bbox.max.z), glm::vec3(bbox.max.x, bbox.max.y, bbox.max.z)};

	for (int i = 0; i < 8; ++i) {
		glm::vec4 t = worldM * glm::vec4(corners[i], 1.0f);
		glm::vec3 p = glm::vec3(t) / t.w;
		worldBounds.min = glm::min(worldBounds.min, p);
		worldBounds.max = glm::max(worldBounds.max, p);
	}

	glm::vec3 worldCenter = (worldBounds.min + worldBounds.max) * 0.5f;
	glm::vec3 size = worldBounds.max - worldBounds.min;
	float radius = std::max({size.x, size.y, size.z}) * 0.5f;

	// Adjust distance based on entity size if not explicitly specified
	float viewDistance = (distance <= 0.0f) ? radius / std::tan(glm::radians(45.0f) * 0.5f) : distance;

	// Position camera to view the entity
	glm::vec3 cameraPos = worldCenter + glm::vec3(0.0f, radius * 0.1f, viewDistance);

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
void Scene::addEntity(std::shared_ptr<Model> model, glm::mat4 const& transform, std::string const& name)
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
	lights.push_back(std::move(light));
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
