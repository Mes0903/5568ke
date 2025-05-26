#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "BoundingBox.hpp"
#include "GameObject.hpp"
#include "include_5568ke.hpp"

class Model;

struct Light {
	glm::vec3 position{2.0f, 5.0f, 2.0f};
	glm::vec3 color{1.0f, 1.0f, 1.0f};
	float intensity{1.0f};
};

class Camera {
public:
	void processKeyboard(float dt, GLFWwindow* w);
	void processMouse(double xpos, double ypos);
	void updateMatrices(GLFWwindow* w); // builds view + proj
	void lookAt(glm::vec3 const& position, glm::vec3 const& target);
	void updateFollow(glm::vec3 const& target, float distance, float height);

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
	static Scene& getInstance();

	// Core scene components
	Camera cam;
	std::vector<std::shared_ptr<GameObject>> gameObjects;
	std::vector<Light> lights;

	// Helper methods for scene management
	std::shared_ptr<GameObject> addGameObject(std::shared_ptr<Model> model);
	void removeGameObject(std::string const& name);
	std::shared_ptr<GameObject> findGameObject(std::string const& name) const;

	void addLight(glm::vec3 const& position, glm::vec3 const& color = glm::vec3(1.0f), float intensity = 1.0f);

	// Position the camera to view the entire scene or a specific game object
	void setupCameraToViewScene(float padding = 1.2f);
	void setupCameraToViewGameObject(std::string const& gameObjectName, float padding = 1.2f);

	// Skybox usage
	bool hasSkybox{false};
	std::string skyboxPath;

	// Scene queries
	size_t getGameObjectCount() const { return gameObjects.size(); }
	size_t getVisibleGameObjectCount() const;

	// Cleanup resources
	void cleanup();

private:
	Scene() = default;
	~Scene();
};