#define GLM_ENABLE_EXPERIMENTAL

#include "GameObject.hpp"

#include <iostream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>

#include "BoundingBox.hpp"
#include "Model.hpp"

// Constructors
GameObject::GameObject(std::shared_ptr<Model> model) : name(model->modelName), model_(model) { updateTransformMatrix(); }

// Model operations
void GameObject::setModel(std::shared_ptr<Model> newModel)
{
	model_ = newModel;

	// If we don't have a name and the model has one, use it
	if (name.empty() && model_ && !model_->modelName.empty()) {
		name = model_->modelName;
	}
}

// Transform setters that need to update the matrix
void GameObject::translate(glm::vec3 const& translation) { position += translation; }
void GameObject::rotate(glm::vec3 const& rotationDelta) { rotationDeg += rotationDelta; }
void GameObject::scaleBy(glm::vec3 const& scaleFactor) { scale *= scaleFactor; }
void GameObject::scaleBy(float uniformScale) { scale *= uniformScale; }

// Transform matrix operations
void GameObject::updateTransformMatrix()
{
	transform_ = calculateTransformMatrix_();

	// model_->localSpaceBBox is an AABB in model space
	if (model_) {
		BoundingBox const& local = model_->localSpaceBBox;

		// Transform the 8 corners to world space, then clamp to AABB
		glm::vec3 worldMin(std::numeric_limits<float>::max());
		glm::vec3 worldMax(std::numeric_limits<float>::lowest());

		for (int i = 0; i < 8; ++i) {
			glm::vec3 corner((i & 1) ? local.max.x : local.min.x, (i & 2) ? local.max.y : local.min.y, (i & 4) ? local.max.z : local.min.z);

			glm::vec3 worldCorner = glm::vec3(transform_ * glm::vec4(corner, 1.0f));

			worldMin = glm::min(worldMin, worldCorner);
			worldMax = glm::max(worldMax, worldCorner);
		}
		worldBBox.min = worldMin;
		worldBBox.max = worldMax;
	}
}

void GameObject::setTransform(glm::mat4 const& newTransform)
{
	transform_ = newTransform;

	// Decompose the matrix to update position, rotation, and scale
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::quat rotation;

	if (glm::decompose(transform_, scale, rotation, position, skew, perspective)) {
		// Convert quaternion to Euler angles in degrees
		glm::vec3 eulerRadians = glm::eulerAngles(rotation);
		rotationDeg = glm::degrees(eulerRadians);
	}
}

glm::mat4 GameObject::calculateTransformMatrix_() const
{
	glm::mat4 t = glm::mat4(1.0f);

	// Apply transformations in the order: Scale -> Rotate -> Translate
	t = glm::translate(t, position);
	t = glm::rotate(t, glm::radians(rotationDeg.x), glm::vec3(1, 0, 0));
	t = glm::rotate(t, glm::radians(rotationDeg.y), glm::vec3(0, 1, 0));
	t = glm::rotate(t, glm::radians(rotationDeg.z), glm::vec3(0, 0, 1));
	t = glm::scale(t, scale);

	return t;
}

// World space operations
glm::vec3 GameObject::getWorldPosition() const { return glm::vec3(transform_[3]); }

glm::vec3 GameObject::getForward() const
{
	// Forward is typically -Z in OpenGL coordinate system
	return glm::normalize(-glm::vec3(transform_[2]));
}

glm::vec3 GameObject::getRight() const
{
	// Right is typically +X
	return glm::normalize(glm::vec3(transform_[0]));
}

glm::vec3 GameObject::getUp() const
{
	// Up is typically +Y
	return glm::normalize(glm::vec3(transform_[1]));
}

// Distance and direction calculations
float GameObject::distanceTo(GameObject const& other) const { return glm::distance(getWorldPosition(), other.getWorldPosition()); }

glm::vec3 GameObject::directionTo(GameObject const& other) const
{
	glm::vec3 direction = other.getWorldPosition() - getWorldPosition();
	return glm::normalize(direction);
}

// Bounds and collision
bool GameObject::isInFrustum(glm::mat4 const& viewProjectionMatrix) const
{
	if (!model_) {
		return false;
	}

	// Get the model's bounding box
	BoundingBox bbox = model_->localSpaceBBox;

	// Transform the 8 corners of the bounding box to world space
	glm::vec3 corners[8] = {{bbox.min.x, bbox.min.y, bbox.min.z}, {bbox.max.x, bbox.min.y, bbox.min.z}, {bbox.min.x, bbox.max.y, bbox.min.z},
													{bbox.max.x, bbox.max.y, bbox.min.z}, {bbox.min.x, bbox.min.y, bbox.max.z}, {bbox.max.x, bbox.min.y, bbox.max.z},
													{bbox.min.x, bbox.max.y, bbox.max.z}, {bbox.max.x, bbox.max.y, bbox.max.z}};

	// Check if any corner is inside the frustum
	for (auto const& corner : corners) {
		glm::vec4 worldPos = transform_ * glm::vec4(corner, 1.0f);
		glm::vec4 clipPos = viewProjectionMatrix * worldPos;

		// Check if the point is within the clip space
		if (clipPos.w > 0.0f) {
			glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
			if (ndc.x >= -1.0f && ndc.x <= 1.0f && ndc.y >= -1.0f && ndc.y <= 1.0f && ndc.z >= -1.0f && ndc.z <= 1.0f) {
				return true;
			}
		}
	}

	return false;
}

// Debug and utility
void GameObject::printInfo() const
{
	std::cout << "GameObject Info:" << std::endl;
	std::cout << "  Name: " << (name.empty() ? "Unnamed" : name) << std::endl;
	std::cout << "  Tag: " << (tag.empty() ? "None" : tag) << std::endl;
	std::cout << "  Layer: " << layer << std::endl;
	std::cout << "  Visible: " << (visible ? "Yes" : "No") << std::endl;
	std::cout << "  Active: " << (active ? "Yes" : "No") << std::endl;
	std::cout << "  Position: " << glm::to_string(position) << std::endl;
	std::cout << "  Rotation: " << glm::to_string(rotationDeg) << std::endl;
	std::cout << "  Scale: " << glm::to_string(scale) << std::endl;
	std::cout << "  Has Model: " << (model_ ? "Yes" : "No") << std::endl;
}

std::string GameObject::toString() const
{
	std::stringstream ss;
	ss << "GameObject{";
	ss << "name='" << name << "', ";
	ss << "tag='" << tag << "', ";
	ss << "layer=" << layer << ", ";
	ss << "visible=" << (visible ? "true" : "false") << ", ";
	ss << "active=" << (active ? "true" : "false") << ", ";
	ss << "pos=" << glm::to_string(position) << ", ";
	ss << "rot=" << glm::to_string(rotationDeg) << ", ";
	ss << "scale=" << glm::to_string(scale) << ", ";
	ss << "hasModel=" << (model_ ? "true" : "false");
	ss << "}";
	return ss.str();
}
