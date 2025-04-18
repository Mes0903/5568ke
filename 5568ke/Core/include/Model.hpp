// include/Model.h
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Mesh.hpp"
#include "Shader.hpp"
#include "Texture.hpp"

struct aiNode;
struct aiScene;

class Model {
public:
	std::vector<Mesh> const& meshes() const;
	std::vector<Texture> const& textures() const;
	bool load(std::filesystem::path const& path);

private:
	void processNode(aiNode*, aiScene const*);
	Mesh processMesh(struct aiMesh*, aiScene const*);

	// data
	std::vector<Mesh> meshes_;
	std::vector<Texture> textures_;
	std::filesystem::path directory_;
};
