#pragma once

#include <vector>

#include "Vertex.hpp"

class Mesh {
public:
	Mesh(std::vector<Vertex> const& v, std::vector<uint32_t> const& i);
	void draw() const;
	void bind() const;
	unsigned indexCount() const;

private:
	unsigned vao_ = 0, vbo_ = 0, ebo_ = 0;
	unsigned indexCount_ = 0;
};
