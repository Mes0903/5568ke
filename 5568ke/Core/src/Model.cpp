/**
 * @file Model.cpp
 * @author Mes (mes900903@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-04-19
 *
 * @copyright Copyright (c) 2025 MIT License
 *
 */

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <iostream>
#include <span>

#include "Model.hpp"

std::vector<Mesh> const& Model::meshes() const { return meshes_; }
std::vector<Texture> const& Model::textures() const { return textures_; }

bool Model::load(std::filesystem::path const& path) {
	Assimp::Importer imp;
	aiScene const* sc = imp.ReadFile(path.string(),
																	 aiProcess_Triangulate							// N‑gons → triangles (required for GL)
																			 | aiProcess_GenSmoothNormals		// compute normals if missing
																			 | aiProcess_FlipUVs						// flip V coordinate for OpenGL
																			 | aiProcess_CalcTangentSpace); // generate tangents/bitangents

	/**
	 * sc == nullptr        → failed outright
	 * INCOMPLETE flag      → half‑parsed file
	 * mRootNode == nullptr → corrupt hierarchy
	 */
	if (!sc || sc->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !sc->mRootNode) {
		std::cerr << imp.GetErrorString() << '\n';
		return false;
	}

	directory_ = std::filesystem::path(path).parent_path();
	processNode(sc->mRootNode, sc);
	return true;
}

void Model::processNode(aiNode* node, aiScene const* sc) {
	/**
	 * Each 'aiNode' stores indices into 'scene->mMeshes'.
	 * Fetch each mesh pointer and convert it with 'processMesh', then push the result into our meshes vector
	 */
	for (unsigned i = 0; i < node->mNumMeshes; ++i)
		meshes_.emplace_back(processMesh(sc->mMeshes[node->mMeshes[i]], sc));

	// Depth‑first recursion through the scene graph.
	for (unsigned i = 0; i < node->mNumChildren; ++i)
		processNode(node->mChildren[i], sc);
}

Mesh Model::processMesh(aiMesh* m, aiScene const* sc) {
	/**
	 * Iterates over every vertex in the Assimp mesh ('mNumVertices').
	 * For each vertex we copy:
	 * - position                        → 'glm::vec3 pos'
	 * - normal                          → 'glm::vec3 normal' (Assimp generated if missing)
	 * - UV (only the first channel '0') → 'glm::vec2 uv'
	 */
	std::vector<Vertex> vs;
	for (unsigned v = 0; v < m->mNumVertices; ++v) {
		Vertex vert;
		vert.pos = {m->mVertices[v].x, m->mVertices[v].y, m->mVertices[v].z};
		vert.normal = {m->mNormals[v].x, m->mNormals[v].y, m->mNormals[v].z};
		if (m->HasTextureCoords(0))
			vert.uv = {m->mTextureCoords[0][v].x, m->mTextureCoords[0][v].y};
		vs.push_back(vert);
	}

	/**
	 * 'vs' now contains tightly‑packed vertices ready for a VBO.
	 *
	 * Assimp guarantees faces are triangles because of 'aiProcess_Triangulate', but the inner loop is generic (works for quads, polygons).
	 * std::span creates a light‑weight view over mIndices so we can use a range for instead of manual indexing.
	 */
	std::vector<uint32_t> idx;
	for (unsigned f = 0; f < m->mNumFaces; ++f) {
		for (unsigned j : std::span(m->mFaces[f].mIndices, m->mFaces[f].mNumIndices))
			idx.push_back(j);
	}

	/**
	 * Each mesh references one material.
	 * We fetch the first diffuse (aiTextureType_DIFFUSE, 0) texture, build its full path (directory_ + "/" + name), and construct a Texture which uploads it to
	 * OpenGL (stored in parallel textures vector). The true flag requests sRGB sampling.
	 */
	if (m->mMaterialIndex >= 0) {
		aiMaterial* mat = sc->mMaterials[m->mMaterialIndex];
		aiString tp;

		// slot 0 : base skin / clothing
		if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &tp) == AI_SUCCESS)
			textures_.emplace_back((directory_ / tp.C_Str()).string(), true);

		// slot 1 : eye overlay (may be absent)
		if (mat->GetTexture(aiTextureType_DIFFUSE, 1, &tp) == AI_SUCCESS)
			textures_.emplace_back((directory_ / tp.C_Str()).string(), true);
		else
			textures_.emplace_back(); // empty placeholder so indices line up
	}

	/**
	 * Mesh’s constructor uploads vertices & indices, sets up VAO attribute pointers, and is stored by value in 'meshes'.
	 */
	return Mesh(vs, idx);
}
