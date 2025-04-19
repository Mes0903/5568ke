#pragma once

#include "include_5568ke.hpp"

#include <glm/mat4x4.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Model.hpp"

class Material;

struct Light {
	glm::vec3 position{2.0f, 5.0f, 2.0f};
	glm::vec3 color{1.0f, 1.0f, 1.0f};
	float intensity{1.0f};

	// Additional light properties if needed
	bool castsShadows{false};
};

struct Entity {
	Model* model;
	glm::mat4 transform;

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
	glm::mat4 view() const { return view_; }
	glm::mat4 proj() const { return proj_; }
	glm::vec3 position() const { return pos_; }

	// Set position and target
	void lookAt(glm::vec3 const& position, glm::vec3 const& target);

	// private:
	// ---- state ----
	glm::vec3 pos_{0.0f, 1.6f, 3.0f};
	float yaw_{-90.0f}; // look -Z in OpenGL
	float pitch_{0.0f};
	glm::vec3 front_{0.0f, 0.0f, -1.0f};
	// ---- cached matrices ----
	glm::mat4 view_{1.0f};
	glm::mat4 proj_{1.0f};
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
	void addEntity(Model* model, glm::mat4 const& transform, std::string const& name = "");
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
	unsigned int skyboxVAO_ = 0, skyboxVBO_ = 0;
	unsigned int skyboxTexture_ = 0;
	bool hasSkybox_ = false;
};
