#define GLM_ENABLE_EXPERIMENTAL

#include "SkyboxVisualizer.hpp"

#include <filesystem>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Model.hpp"
#include "ModelRegistry.hpp"
#include "Scene.hpp"
#include "Shader.hpp"

SkyboxVisualizer& SkyboxVisualizer::getInstance()
{
	static SkyboxVisualizer instance;
	return instance;
}

void SkyboxVisualizer::init()
{
	// Create skybox shader for GLTF models
	skyboxShader = std::make_shared<Shader>();
	skyboxShader->resetShaderPath("assets/shaders/skybox_model.vert", "assets/shaders/skybox_model.frag");

	// Create cubemap shader
	cubemapShader = std::make_shared<Shader>();
	cubemapShader->resetShaderPath("assets/shaders/skybox.vert", "assets/shaders/skybox.frag");

	std::cout << "[SkyboxVisualizer] Initialized with custom skybox shaders" << std::endl;

	// Hard-code the skybox now.
	std::string skyboxPath = "assets/models/fantasy_landscape_3/scene.gltf";
	std::cout << "[SkyboxVisualizer] Loading skybox from: " << skyboxPath << std::endl;

	bool skyboxLoaded = loadSkyboxFromGltf(skyboxPath);
	std::cout << "[SkyboxVisualizer] Skybox loaded: " << (skyboxLoaded ? "SUCCESS" : "FAILED") << std::endl;

	// Set up the standard cubemap mesh (even if we don't use it initially)
	setupCubemapMesh();
}

void SkyboxVisualizer::cleanup()
{
	// Clean up OpenGL resources
	if (cubemapVAO) {
		glDeleteVertexArrays(1, &cubemapVAO);
		cubemapVAO = 0;
	}

	if (cubemapVBO) {
		glDeleteBuffers(1, &cubemapVBO);
		cubemapVBO = 0;
	}

	if (cubemapTexture) {
		glDeleteTextures(1, &cubemapTexture);
		cubemapTexture = 0;
	}

	// Model will be cleaned up by shared_ptr
	skyboxModel = nullptr;
	skyboxType = SkyboxType::NONE;
}

bool SkyboxVisualizer::loadSkyboxFromGltf(std::string const& gltfPath)
{
	try {
		// Use ModelRegistry to load the model
		auto& registry = ModelRegistry::getInstance();

		// Try to load model, but don't add it to the scene
		skyboxModel = registry.loadModel(gltfPath, "skybox_model");

		if (!skyboxModel) {
			std::cout << "[SkyboxVisualizer ERROR] Failed to load GLTF skybox: " << gltfPath << std::endl;
			return false;
		}

		// Successfully loaded
		std::cout << "[SkyboxVisualizer] Successfully loaded GLTF skybox: " << gltfPath << std::endl;
		skyboxPath = gltfPath;
		skyboxType = SkyboxType::GLTF_MODEL;

		return true;
	} catch (std::exception const& e) {
		std::cout << "[SkyboxVisualizer ERROR] Exception loading GLTF skybox: " << e.what() << std::endl;
		return false;
	}
}

// Due to we used the glTF skybox for now, this function havent been tested
bool SkyboxVisualizer::loadSkyboxFromCubemap(std::string const& directory)
{
	std::filesystem::path dirPath(directory);
	if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
		std::cout << "[SkyboxVisualizer ERROR] Invalid cubemap directory: " << directory << std::endl;
		return false;
	}

	// Expected face filenames for a cubemap
	std::vector<std::string> const faceNames = {"right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"};

	// Build full paths
	std::vector<std::string> facePaths;
	for (auto const& face : faceNames) {
		std::filesystem::path facePath = dirPath / face;
		if (!std::filesystem::exists(facePath)) {
			std::cout << "[SkyboxVisualizer ERROR] Missing cubemap face: " << facePath.string() << std::endl;
			return false;
		}
		facePaths.push_back(facePath.string());
	}

	// Create the cubemap texture
	if (!createCubemapFromImages(facePaths)) {
		std::cout << "[SkyboxVisualizer ERROR] Failed to create cubemap texture" << std::endl;
		return false;
	}

	skyboxType = SkyboxType::CUBEMAP;
	skyboxPath = directory;
	std::cout << "[SkyboxVisualizer] Successfully loaded cubemap skybox from: " << directory << std::endl;
	return true;
}

void SkyboxVisualizer::draw(Scene const& scene)
{
	if (skyboxType == SkyboxType::NONE) {
		return; // Nothing to draw
	}

	if (skyboxType == SkyboxType::GLTF_MODEL && skyboxModel) {
		// Save current GL state
		GLboolean depthTest;
		glGetBooleanv(GL_DEPTH_TEST, &depthTest);

		GLint depthFunc;
		glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);

		GLboolean depthMask;
		glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

		GLboolean cullFace;
		glGetBooleanv(GL_CULL_FACE, &cullFace);

		// We need to draw the skybox first, with depth write disabled to ensure it goes behind everything
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);	// Disable depth writes
		glDepthFunc(GL_LEQUAL); // Use LEQUAL for depth test

		// Disable face culling to see all sides of the skybox
		glDisable(GL_CULL_FACE);

		// Use skybox shader
		skyboxShader->bind();

		// Create a scaled model matrix to make the skybox large enough and center it on the camera
		glm::mat4 skyboxModelMat = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f)); // Make it larger

		// Remove translation from view matrix to keep skybox centered on camera
		glm::mat4 skyboxView = glm::mat4(glm::mat3(scene.cam.view)); // Remove translation
		skyboxShader->sendMat4("view", skyboxView);
		skyboxShader->sendMat4("proj", scene.cam.proj);
		skyboxShader->sendMat4("model", skyboxModelMat);

		// Draw the skybox model
		this->skyboxModel->draw(*skyboxShader, skyboxModelMat);

		// Restore previous GL state
		if (cullFace)
			glEnable(GL_CULL_FACE);

		glDepthFunc(depthFunc); // Restore original depth function
		glDepthMask(depthMask); // Restore original depth mask

		if (!depthTest)
			glDisable(GL_DEPTH_TEST);
	}
	else if (skyboxType == SkyboxType::CUBEMAP && cubemapTexture && cubemapVAO) {
		// Save current GL state
		GLboolean depthTest;
		glGetBooleanv(GL_DEPTH_TEST, &depthTest);

		GLint depthFunc;
		glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);

		GLboolean depthMask;
		glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

		// Set up GL state for skybox rendering
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);	// Disable depth writes
		glDepthFunc(GL_LEQUAL); // Use LEQUAL for the depth test

		// Use cubemap shader
		cubemapShader->bind();

		// Remove translation from view matrix to keep skybox centered on camera
		glm::mat4 skyboxView = glm::mat4(glm::mat3(scene.cam.view)); // Remove translation
		cubemapShader->sendMat4("view", skyboxView);
		cubemapShader->sendMat4("proj", scene.cam.proj);

		// Bind the cubemap texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		cubemapShader->sendInt("skybox", 0);

		// Render the cubemap
		glBindVertexArray(cubemapVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		// Restore previous GL state
		glDepthFunc(depthFunc); // Restore original depth function
		glDepthMask(depthMask); // Restore original depth mask

		if (!depthTest)
			glDisable(GL_DEPTH_TEST);
	}
}

// Due to we used the glTF skybox for now, this function havent been tested
void SkyboxVisualizer::setupCubemapMesh()
{
	// Skybox vertices (a cube with positions only)
	float skyboxVertices[] = {// Positions
														-1.0f, 1.0f,	-1.0f, -1.0f, -1.0f, -1.0f, 1.0f,	 -1.0f, -1.0f, 1.0f,	-1.0f, -1.0f, 1.0f,	 1.0f,	-1.0f, -1.0f, 1.0f,	 -1.0f,
														-1.0f, -1.0f, 1.0f,	 -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,	-1.0f, -1.0f, 1.0f,	 -1.0f, -1.0f, 1.0f,	1.0f,	 -1.0f, -1.0f, 1.0f,
														1.0f,	 -1.0f, -1.0f, 1.0f,	-1.0f, 1.0f,	1.0f,	 1.0f,	1.0f,	 1.0f,	1.0f,	 1.0f,	1.0f,	 1.0f,	-1.0f, 1.0f,	-1.0f, -1.0f,
														-1.0f, -1.0f, 1.0f,	 -1.0f, 1.0f,	 1.0f,	1.0f,	 1.0f,	1.0f,	 1.0f,	1.0f,	 1.0f,	1.0f,	 -1.0f, 1.0f,	 -1.0f, -1.0f, 1.0f,
														-1.0f, 1.0f,	-1.0f, 1.0f,	1.0f,	 -1.0f, 1.0f,	 1.0f,	1.0f,	 1.0f,	1.0f,	 1.0f,	-1.0f, 1.0f,	1.0f,	 -1.0f, 1.0f,	 -1.0f,
														-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,	1.0f,	 -1.0f, -1.0f, 1.0f,	-1.0f, -1.0f, -1.0f, -1.0f, 1.0f,	 1.0f,	-1.0f, 1.0f};

	// Create VAO and VBO for the skybox
	glGenVertexArrays(1, &cubemapVAO);
	glGenBuffers(1, &cubemapVBO);

	glBindVertexArray(cubemapVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubemapVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindVertexArray(0);
}

// Due to we used the glTF skybox for now, this function havent been tested
bool SkyboxVisualizer::createCubemapFromImages(std::vector<std::string> const& faceImages)
{
	// Make sure we have exactly 6 images for the cubemap
	if (faceImages.size() != 6) {
		std::cout << "[SkyboxVisualizer ERROR] Cubemap requires exactly 6 images" << std::endl;
		return false;
	}

	// Create and bind a cubemap texture
	if (cubemapTexture) {
		glDeleteTextures(1, &cubemapTexture);
	}

	glGenTextures(1, &cubemapTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

	// In a real implementation, we would use stb_image or another library to load the images
	// This is simplified as a placeholder
	std::cout << "[SkyboxVisualizer] Cubemap textures would be loaded from: " << std::endl;
	for (size_t i = 0; i < faceImages.size(); i++) {
		std::cout << "  - Face " << i << ": " << faceImages[i] << std::endl;
	}

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return true;
}
