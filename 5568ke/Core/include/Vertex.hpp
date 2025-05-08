#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "Animation.hpp"

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texcoord;

	// Bone data for skeletal animation
	VertexBoneData boneData;
};