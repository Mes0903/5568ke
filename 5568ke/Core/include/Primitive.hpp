#pragma once

#include "Material.hpp"

struct Primitive {
	unsigned int indexOffset;
	unsigned int indexCount;
	Material* material;
};
