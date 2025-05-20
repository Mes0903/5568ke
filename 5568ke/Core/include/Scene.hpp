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
#include "include_5568ke.hpp"

class Model;

struct Light {
	glm::vec3 position{2.0f, 5.0f, 2.0f};
	glm::vec3 color{1.0f, 1.0f, 1.0f};
	float intensity{1.0f};

	// Additional light properties if needed
	bool castsShadows{false};
};

class Entity {
public:
	std::shared_ptr<Model> model;
	glm::vec3 position{0.0f};
	glm::vec3 rotationDeg{0.0f}; // degree
	float scale{1.0f};
	glm::mat4 transform{1.0f};

	// Additional entity properties
	bool visible{true};

	void rebuildTransform()
	{
		glm::mat4 t(1.0f);
		t = glm::translate(t, position);
		t = glm::rotate(t, glm::radians(rotationDeg.x), {1, 0, 0});
		t = glm::rotate(t, glm::radians(rotationDeg.y), {0, 1, 0});
		t = glm::rotate(t, glm::radians(rotationDeg.z), {0, 0, 1});
		t = glm::scale(t, glm::vec3(scale));
		transform = t;
	}
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
	static Scene& getInstance();

	// Core scene components
	Camera cam;
	std::vector<Entity> ents;
	std::vector<Light> lights;

	// Helper methods for scene management
	void addEntity(std::shared_ptr<Model> model);
	void removeEntity(std::string const& name);
	std::optional<std::reference_wrapper<Entity>> findEntity(std::string const& name);

	void addLight(glm::vec3 const& position, glm::vec3 const& color = glm::vec3(1.0f), float intensity = 1.0f);

	// Position the camera to view the entire scene
	void setupCameraToViewScene(float padding = 1.2f);

	// Position the camera to view a specific entity
	void setupCameraToViewEntity(std::string const& entityName, float padding = 1.2f);

	// Skybox usage
	bool hasSkybox{false};
	std::string skyboxPath;

	// Cleanup resources
	void cleanup();

private:
	Scene() = default;
	~Scene();
};