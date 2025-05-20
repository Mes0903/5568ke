#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "include_5568ke.hpp"

class Shader;
class Scene;
class Model;

class SkyboxVisualizer {
public:
	static SkyboxVisualizer& getInstance();
	void init();
	void cleanup();
	void draw(Scene const& scene); // The of skybox should be done first, with depth test configured

	// Load skybox from gltf file (or directory of 6 images for traditional cubemap)
	bool loadSkyboxFromGltf(std::string const& gltfPath);
	bool loadSkyboxFromCubemap(std::string const& directory);

	std::shared_ptr<Shader> skyboxShader;	 // For GLTF models
	std::shared_ptr<Shader> cubemapShader; // For traditional cubemaps

private:
	SkyboxVisualizer() = default;
	~SkyboxVisualizer() = default;

	// GLTF-based skybox data
	std::shared_ptr<Model> skyboxModel;
	std::string skyboxPath;

	// Traditional cubemap-based skybox data
	GLuint cubemapVAO = 0;
	GLuint cubemapVBO = 0;
	GLuint cubemapTexture = 0;

	// Which type of skybox we're using
	enum class SkyboxType { NONE, GLTF_MODEL, CUBEMAP };
	SkyboxType skyboxType = SkyboxType::NONE;

	// Helper methods
	bool createCubemapFromImages(std::vector<std::string> const& faceImages);
	void setupCubemapMesh();
};