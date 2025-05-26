#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "BoundingBox.hpp"

class Model;

/**
 * @brief GameObject class that replaces the GameObject class
 * Represents a game object in the 3D scene with transform, visibility, and model data
 */
class GameObject {
public:
	// Constructors
	GameObject() = default;
	GameObject(std::shared_ptr<Model> model);

	// Destructor
	~GameObject() = default;

	// Copy/Move constructors and assignment operators
	GameObject(GameObject const&) = default;
	GameObject& operator=(GameObject const&) = default;
	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	// Model operations (needs validation)
	void setModel(std::shared_ptr<Model> newModel);
	std::shared_ptr<Model> getModel() const { return model_; }
	bool hasModel() const { return model_ != nullptr; }

	void translate(glm::vec3 const& translation);
	void rotate(glm::vec3 const& rotationDelta);
	void scaleBy(glm::vec3 const& scaleFactor);
	void scaleBy(float uniformScale);

	// Transform matrix operations (computed property)
	glm::mat4 const& getTransform() const { return transform_; }
	void updateTransformMatrix();
	void setTransform(glm::mat4 const& newTransform);

	// World space operations (computed properties)
	glm::vec3 getWorldPosition() const;
	glm::vec3 getForward() const;
	glm::vec3 getRight() const;
	glm::vec3 getUp() const;

	// Distance and direction calculations
	float distanceTo(GameObject const& other) const;
	glm::vec3 directionTo(GameObject const& other) const;

	// Bounds and collision
	bool isInFrustum(glm::mat4 const& viewProjectionMatrix) const;

	// Debug and utility
	void printInfo() const;
	std::string toString() const;

public:
	// Public properties that don't need additional logic
	std::string_view name;
	std::string tag;
	bool visible{true};
	bool active{true};
	int layer{0};

	// Transform properties - public for direct access and efficiency
	glm::vec3 position{0.0f};
	glm::vec3 rotationDeg{0.0f}; // Rotation in degrees
	glm::vec3 scale{1.0f};

	// For collision
	BoundingBox worldBBox;
	// collision event callback — override in derived class or assign externally 'other' is the object this collided with
	std::function<void(std::shared_ptr<GameObject> other)> onCollisionEnter = [&](std::shared_ptr<GameObject> other) {
		std::cout << "[GameObject]" << this->name << " hit " << other->name << "\n";
	};

private:
	// Private members that need controlled access
	std::shared_ptr<Model> model_{nullptr};
	glm::mat4 transform_{1.0f};

	// Internal helper methods
	glm::mat4 calculateTransformMatrix_() const;
};
