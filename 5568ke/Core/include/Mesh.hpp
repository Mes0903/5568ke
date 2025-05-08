#pragma once

#include <vector>

#include "Primitive.hpp"
#include "Vertex.hpp"

class Mesh {
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	std::vector<Primitive> primitives;

	// Flag to indicate if this mesh has animation data
	bool hasAnimation = false;

	void setup();
	void draw(Shader& shader) const;

	// Cleanup resources
	void cleanup();

private:
	unsigned vao_ = 0, vbo_ = 0, ebo_ = 0;
};