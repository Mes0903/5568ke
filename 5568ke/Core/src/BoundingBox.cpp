#define GLM_ENABLE_EXPERIMENTAL

#include "BoundingBox.hpp"

#include "Mesh.hpp"
#include "Model.hpp"
#include "Node.hpp"

namespace BBoxUtil {
namespace {
BoundingBox transformBBox(BoundingBox const& in, glm::mat4 const& M)
{
	BoundingBox out;
	out.min = glm::vec3(std::numeric_limits<float>::max());
	out.max = glm::vec3(std::numeric_limits<float>::lowest());

	// 8 個 corner
	for (int c = 0; c < 8; ++c) {
		glm::vec3 p = {(c & 1 ? in.max.x : in.min.x), (c & 2 ? in.max.y : in.min.y), (c & 4 ? in.max.z : in.min.z)};
		p = glm::vec3(M * glm::vec4(p, 1.0f));
		out.min = glm::min(out.min, p);
		out.max = glm::max(out.max, p);
	}
	return out;
}

BoundingBox getSkinnedMeshBBox(Mesh const& mesh, Model const& model)
{
	BoundingBox bbox;
	bbox.min = glm::vec3(std::numeric_limits<float>::max());
	bbox.max = glm::vec3(std::numeric_limits<float>::lowest());

	for (auto const& v : mesh.vertices) {
		glm::vec4 pos(v.position, 1.0f);
		glm::vec4 skinned(0.0f);
		float total = 0.0f;

		for (int i = 0; i < 4; ++i) {
			float w = v.boneWeights[i];
			int id = v.boneIds[i];

			// do the linear blend skinning
			if (w > 0.0f && id >= 0 && static_cast<std::size_t>(id) < model.jointMatrices.size()) {
				skinned += w * model.jointMatrices[id] * pos;
				total += w;
			}
		}

		if (total > 0.0f)
			pos = skinned / total;

		glm::vec3 p = glm::vec3(pos);
		bbox.min = glm::min(bbox.min, p);
		bbox.max = glm::max(bbox.max, p);
	}

	return bbox;
}

BoundingBox getStaticMeshBox(Model const& model, size_t meshIndex)
{
	BoundingBox local = model.boundingBoxes[meshIndex];
	glm::mat4 nodeM(1.0f);

	if (meshIndex < model.meshNodeIndices.size()) {
		int nodeIdx = model.meshNodeIndices[meshIndex];
		if (nodeIdx >= 0 && static_cast<std::size_t>(nodeIdx) < model.nodes.size() && model.nodes[nodeIdx])
			nodeM = model.nodes[nodeIdx]->getNodeMatrix();
	}

	return transformBBox(local, nodeM);
}
} // namespace

BoundingBox getMeshBBox(Mesh const& mesh)
{
	BoundingBox bbox;
	if (mesh.vertices.empty()) {
		bbox.min = glm::vec3(0.0f);
		bbox.max = glm::vec3(0.0f);
		return bbox;
	}

	bbox.min = glm::vec3(std::numeric_limits<float>::max());
	bbox.max = glm::vec3(std::numeric_limits<float>::lowest());

	for (auto const& vertex : mesh.vertices) {
		bbox.min.x = std::min(bbox.min.x, vertex.position.x);
		bbox.min.y = std::min(bbox.min.y, vertex.position.y);
		bbox.min.z = std::min(bbox.min.z, vertex.position.z);

		bbox.max.x = std::max(bbox.max.x, vertex.position.x);
		bbox.max.y = std::max(bbox.max.y, vertex.position.y);
		bbox.max.z = std::max(bbox.max.z, vertex.position.z);
	}

	return bbox;
}

void updateLocalBBox(Model& model)
{
	BoundingBox global;
	global.min = glm::vec3(std::numeric_limits<float>::max());
	global.max = glm::vec3(std::numeric_limits<float>::lowest());

	// When a model has skinning data, the mesh's node transform is typically baked into the inverse bind matrices.
	// Applying the node matrix again would result in an oversized bounding box. Detect this case and avoid applying the extra transform.
	bool hasSkinning = !model.jointMatrices.empty();

	for (size_t i = 0; i < model.meshes.size(); ++i) {
		BoundingBox local;

		if (hasSkinning)
			local = getSkinnedMeshBBox(model.meshes[i], model);
		else
			local = getStaticMeshBox(model, i);

		global.min = glm::min(global.min, local.min);
		global.max = glm::max(global.max, local.max);
	}

	model.localSpaceBBox = global;
}

// Fast overlap test (inclusive)
bool isIntersectBBox(BoundingBox const& a, BoundingBox const& b)
{
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// Combine two boxes (useful for hierarchy/BVH later)
BoundingBox mergeBBox(BoundingBox const& a, BoundingBox const& b) { return {glm::min(a.min, b.min), glm::max(a.max, b.max)}; }
} // namespace BBoxUtil
