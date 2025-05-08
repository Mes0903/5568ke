#include "include_5568ke.hpp"

#include "Mesh.hpp"

void Mesh::setup()
{
	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);
	glGenBuffers(1, &ebo_);

	glBindVertexArray(vao_);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

	// Normal attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	// Texcoord attribute
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));

	// If the mesh has animation data, set up bone attributes
	if (hasAnimation) {
		// Bone IDs attribute (as integers)
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneData.boneIds));

		// Bone weights attribute
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneData.weights));
	}

	glBindVertexArray(0);
}

void Mesh::draw(Shader& shader) const
{
	glBindVertexArray(vao_);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

	// Set animation flag in shader
	shader.setBool("hasAnimation", hasAnimation);

	for (auto const& prim : primitives) {
		if (prim.material)
			prim.material->bind(shader);
		glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, (void*)(prim.indexOffset * sizeof(unsigned)));
	}
	glBindVertexArray(0);
}

void Mesh::cleanup()
{
	if (vao_ != 0) {
		glDeleteVertexArrays(1, &vao_);
		vao_ = 0;
	}

	if (vbo_ != 0) {
		glDeleteBuffers(1, &vbo_);
		vbo_ = 0;
	}

	if (ebo_ != 0) {
		glDeleteBuffers(1, &ebo_);
		ebo_ = 0;
	}

	vertices.clear();
	indices.clear();
	primitives.clear();
}