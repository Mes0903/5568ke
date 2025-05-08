#include "include_5568ke.hpp"

#include "Renderer.hpp"

void Renderer::setupDefaultRenderer()
{
	// Create default shaders_
	auto blinnPhongShader = std::make_unique<Shader>();
	blinnPhongShader->resetShader("assets/shaders/blinn.vert", "assets/shaders/blinn.frag");

	// Create animated shaders
	auto animatedBlinnShader = std::make_unique<Shader>();
	animatedBlinnShader->resetShader("assets/shaders/animated_blinn.vert", "assets/shaders/blinn.frag");

	// Store in the shader map
	shaders_["blinn"] = std::move(blinnPhongShader);
	shaders_["animated_blinn"] = std::move(animatedBlinnShader);

	// Set default main shader
	mainShader_ = shaders_["blinn"].get();
	animatedShader_ = shaders_["animated_blinn"].get();
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
	// Draw opaque models
	drawModels_(scene);

	// Print frame stats (could be toggled with a debug flag)
	// std::cout << "Frame stats: " << currentFrameStats_.drawCalls << " draw calls, "
	//           << currentFrameStats_.visibleEntities << " visible entities" << std::endl;
}

void Renderer::drawModels_(Scene const& scene)
{
	if (!mainShader_ || !animatedShader_)
		return;

	// Draw all visible entities
	for (auto const& entity : scene.ents) {
		if (!entity.visible || !entity.model)
			continue;

		// Choose the appropriate shader based on whether the model has animations
		Shader* selectedShader = entity.model->hasAnimations ? animatedShader_ : mainShader_;

		// Bind the selected shader
		selectedShader->bind();

		// Set camera-related uniforms
		selectedShader->setMat4("view", scene.cam.view());
		selectedShader->setMat4("proj", scene.cam.proj());

		// Setup lighting
		setupLighting_(scene, selectedShader);

		// Draw the model with its transform
		entity.model->draw(*selectedShader, entity.transform);

		// Update stats
		currentFrameStats_.drawCalls++;
		currentFrameStats_.visibleEntities++;
	}
}

void Renderer::setupLighting_(Scene const& scene, Shader* shader)
{
	if (!shader)
		return;

	// Set light positions and properties
	// This implementation assumes a simple lighting model like in the original code
	if (!scene.lights.empty()) {
		shader->setVec3("lightPos", scene.lights[0].position);
		shader->setVec3("lightColor", scene.lights[0].color);
		shader->setFloat("lightIntensity", scene.lights[0].intensity);
	}

	shader->setVec3("viewPos", scene.cam.position());

	// For more complex lighting, you could iterate through lights and set arrays of uniforms
}

void Renderer::endFrame()
{
	glBindVertexArray(0);
	glUseProgram(0);
}

Renderer::~Renderer()
{
	// Clean up shader resources
	shaders_.clear();
	mainShader_ = nullptr;
	animatedShader_ = nullptr;
}