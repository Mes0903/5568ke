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

	glEnableVertexAttribArray(0); // position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

	glEnableVertexAttribArray(1); // normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	glEnableVertexAttribArray(2); // texcoord
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));

	glBindVertexArray(0);
}

void Mesh::draw(Shader& shader) const
{
	glBindVertexArray(vao_);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

	for (auto const& prim : primitives) {
		if (prim.material)
			prim.material->bind(shader);
		glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, (void*)(prim.indexOffset * sizeof(unsigned)));
	}
	glBindVertexArray(0);
}