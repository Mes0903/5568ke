#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>

#include "BoundingBox.hpp"
#include "include_5568ke.hpp"

class Model;

struct Light {
	glm::vec3 position{2.0f, 5.0f, 2.0f};
	glm::vec3 color{1.0f, 1.0f, 1.0f};
	float intensity{1.0f};

	// Additional light properties if needed
	bool castsShadows{false};
};

struct Entity {
	std::shared_ptr<Model> model;
	glm::mat4 transform;
	BoundingBox worldBoundingBox;
	float scale{1.0f};

	// Additional entity properties
	std::string name;
	bool visible{true};
	bool castsShadow{true};
};

class Camera {
public:
	void processKeyboard(float dt, GLFWwindow* w);
	void processMouse(double xpos, double ypos);
	void updateMatrices(GLFWwindow* w); // builds view + proj
	void lookAt(glm::vec3 const& position, glm::vec3 const& target);

	// State
	glm::vec3 pos{0.0f, 1.6f, 3.0f};
	float yaw{-90.0f}; // look -Z in OpenGL
	float pitch{0.0f};
	glm::vec3 front{0.0f, 0.0f, -1.0f};

	// Cached matrices
	glm::mat4 view{1.0f};
	glm::mat4 proj{1.0f};

	// For camera update
	bool firstMouse{true};
	double lastX;
	double lastY;
};

class Scene {
public:
	Scene() = default;
	~Scene();

	// Core scene components
	Camera cam;
	std::vector<Entity> ents;
	std::vector<Light> lights;

	// Helper methods for scene management
	void addEntity(std::shared_ptr<Model> model, glm::mat4 const& transform, std::string const& name = "");
	void removeEntity(std::string const& name);
	Entity* findEntity(std::string const& name);

	void addLight(glm::vec3 const& position, glm::vec3 const& color = glm::vec3(1.0f), float intensity = 1.0f);

	// Position the camera to view the entire scene
	void setupCameraToViewScene(float padding = 1.2f);

	// Position the camera to view a specific entity
	void setupCameraToViewEntity(std::string const& entityName, float distance = 3.0f);

	// Load a skybox
	void loadSkybox(std::string const& directory);

	// Cleanup resources
	void cleanup();

private:
	// Map for quick entity lookup by name
	std::unordered_map<std::string, size_t> entityMap_;

	// Skybox resources
	unsigned int skyboxVAO_{0};
	unsigned int skyboxVBO_{0};
	unsigned int skyboxTexture_{0};
	bool hasSkybox_ = false;
};
