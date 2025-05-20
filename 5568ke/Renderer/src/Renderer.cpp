#define GLM_ENABLE_EXPERIMENTAL

#include "Renderer.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "Model.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "include_5568ke.hpp"

Renderer& Renderer::getInstance()
{
	static Renderer instance;
	return instance;
}

void Renderer::init()
{
	// Create default shaders
	auto blinnPhongShader = std::make_unique<Shader>();
	blinnPhongShader->resetShaderPath("assets/shaders/blinn.vert", "assets/shaders/blinn.frag");
	shaders_["blinn"] = std::move(blinnPhongShader);

	// Create skinned shader for animated models
	auto skinnedShader = std::make_unique<Shader>();
	skinnedShader->resetShaderPath("assets/shaders/skinned.vert", "assets/shaders/blinn.frag");
	shaders_["skinned"] = std::move(skinnedShader);

	// Set default main shader
	mainShader_ = shaders_["blinn"];
	skinnedShader_ = shaders_["skinned"];

	// Initialize skeleton visualizer
	skeletonVisualizerRef.init();
	shaders_["skeleton"] = skeletonVisualizerRef.skeletonShader;
	std::cout << "[Renderer] SkeletonVisualizer initialized" << std::endl;

	lightVisualizerRef.init();
	shaders_["lightPoint"] = lightVisualizerRef.lightPointShader;
	std::cout << "[Renderer] LightPointVisualizer initialized" << std::endl;

	boundingBoxVisualizerRef.init();
	shaders_["boundingBox"] = boundingBoxVisualizerRef.boxShader;
	std::cout << "[Renderer] BoundingBoxVisualizer initialized" << std::endl;

	skyboxVisualizerRef.init();
	shaders_["skybox_model"] = skyboxVisualizerRef.skyboxShader;
	shaders_["skybox_cubemap"] = skyboxVisualizerRef.cubemapShader;
	std::cout << "[Renderer] SkyboxVisualizer initialized" << std::endl;
}

void Renderer::beginFrame(int w, int h, glm::vec3 const& c)
{
	viewportWidth_ = w;
	viewportHeight_ = h;

	glViewport(0, 0, w, h);

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance and avoid interior fragments
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Clear the screen
	glClearColor(c.r, c.g, c.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset frame stats
	currentFrameStats_ = FrameStats();
}

void Renderer::drawScene(Scene const& scene)
{
	if (showSkybox)
		skyboxVisualizerRef.draw(scene);

	if (showModels)
		drawModels_(scene);

	if (showLightPoint)
		lightVisualizerRef.draw(scene);

	if (showBBox)
		boundingBoxVisualizerRef.draw(scene);
}

void Renderer::drawModels_(Scene const& scene)
{
	if (!mainShader_)
		return;

	// Bind the main shader
	mainShader_->bind();

	// Set camera-related uniforms
	mainShader_->sendMat4("view", scene.cam.view);
	mainShader_->sendMat4("proj", scene.cam.proj);

	// Setup lighting
	setupLighting_(scene, mainShader_);

	// If we have a skinned shader, set up its uniforms too
	if (skinnedShader_) {
		skinnedShader_->bind();
		skinnedShader_->sendMat4("view", scene.cam.view);
		skinnedShader_->sendMat4("proj", scene.cam.proj);
		setupLighting_(scene, skinnedShader_);
	}

	// Draw all visible entities
	for (auto const& entity : scene.ents) {
		if (!entity.visible || !entity.model)
			continue;

		if (showWireFrame) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		// Choose shader based on if the model has joint matrices
		Shader const* shaderToUse = mainShader_.get();

		if (skinnedShader_ && !entity.model->jointMatrices.empty() && entity.model->animations.size() > 0)
			shaderToUse = skinnedShader_.get();

		shaderToUse->bind();																// Bind the appropriate shader
		entity.model->draw(*shaderToUse, entity.transform); // Draw the model with the scaled model matrix

		if (skeletonVisualizerRef.hasSkeletonData(entity.model)) {

			// Draw skeleton if enabled
			if (showSkeletons) {
				// Draw debug visualization
				skeletonVisualizerRef.draw(entity, scene.cam);

				// Rebind main shader after drawing lines
				mainShader_->bind();
			}
		}

		// Update stats
		currentFrameStats_.drawCalls++;
		currentFrameStats_.visibleEntities++;
	}
}

void Renderer::setupLighting_(Scene const& scene, std::shared_ptr<Shader> const& shader)
{
	if (!shader)
		return;

	// Set light positions and properties
	// This implementation assumes a simple lighting model like in the original code
	if (!scene.lights.empty()) {
		shader->sendVec3("lightPos", scene.lights[0].position);
		// shader->sendVec3("lightColor", scene.lights[0].color);
		// shader->sendFloat("lightIntensity", scene.lights[0].intensity);
	}

	shader->sendVec3("viewPos", scene.cam.pos);
}

void Renderer::endFrame()
{
	glBindVertexArray(0);
	glUseProgram(0);
}

void Renderer::cleanup()
{
	// Clean up skeleton visualizer
	skeletonVisualizerRef.cleanup();
	lightVisualizerRef.cleanup();
	boundingBoxVisualizerRef.cleanup();
	skyboxVisualizerRef.cleanup();
}
