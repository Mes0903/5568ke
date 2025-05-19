#pragma once

#include <glm/glm.hpp>

class Mesh;
class Model;

struct BoundingBox {
	glm::vec3 min;
	glm::vec3 max;
};

namespace BBoxUtil {
BoundingBox getMeshBBox(Mesh const& mesh);
void updateLocalBBox(Model& m);
} // namespace BBoxUtil
