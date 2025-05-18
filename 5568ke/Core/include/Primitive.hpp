#pragma once

class Material;

/**
 * @brief Represents a renderable sub-region within a mesh, using a subset of the index buffer and a material.
 */
struct Primitive {
	unsigned int indexOffset; // Offset into the index buffer (in indices, not bytes)
	unsigned int indexCount;	// Number of indices to draw (usually divisible by 3)
	Material* material;				// Material to bind when drawing this primitive
	bool doubleSided = false;
};
