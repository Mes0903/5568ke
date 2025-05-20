#define GLM_ENABLE_EXPERIMENTAL

#include "Application.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "AnimationClip.hpp"
#include "Model.hpp"

Application::Application()
{
	// Initialize to false
	for (int i = 0; i < 1024; i++)
		keys_[i] = false;
}

Application::~Application() { cleanup_(); }

int Application::run()
{
	initWindow_();
	initGL_();
	initImGui_();
	setupDefaultScene_();
	loop_();
	cleanup_();
	return 0;
}

void Application::initWindow_()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window_ = glfwCreateWindow(1280, 720, "5568ke Model Viewer", nullptr, nullptr);
	glfwMakeContextCurrent(window_);
	glfwSwapInterval(1);

	// Set input mode and callbacks
	glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Changed to CURSOR_NORMAL for UI interaction
	glfwSetWindowUserPointer(window_, this);

	glfwSetKeyCallback(window_, keyCallback_);
	glfwSetCursorPosCallback(window_, mouseCallback_);
	glfwSetScrollCallback(window_, scrollCallback_);
}

void Application::initImGui_() { ImGuiManagerRef.init(window_); }

void Application::keyCallback_(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS)
			app->keys_[key] = true;
		else if (action == GLFW_RELEASE)
			app->keys_[key] = false;
	}

	// Toggle cursor mode for camera control
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		int cursorMode = glfwGetInputMode(window, GLFW_CURSOR);
		if (cursorMode == GLFW_CURSOR_NORMAL) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			// reset first-mouse flag so next movement starts clean
			Camera& cam = static_cast<Application*>(glfwGetWindowUserPointer(window))->sceneRef.cam;
			cam.firstMouse = true;
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	if (key == GLFW_KEY_F4 && action == GLFW_PRESS) {
		app->showAnimationUI_ = !app->showAnimationUI_;
	}
}

void Application::mouseCallback_(GLFWwindow* window, double xpos, double ypos)
{
	Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	// Only process mouse movement for camera if cursor is disabled
	int cursorMode = glfwGetInputMode(window, GLFW_CURSOR);
	if (cursorMode == GLFW_CURSOR_DISABLED) {
		app->sceneRef.cam.processMouse(xpos, ypos);
	}
}

void Application::scrollCallback_(GLFWwindow* window, double xoffset, double yoffset)
{
	// TODO: Handle zoom or other scroll behaviors if needed.
	//       For example, adjust camera FOV or distance
}

void Application::initGL_()
{
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Print some OpenGL information
	std::cout << "[Application] OpenGL version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "[Application] GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "[Application] Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "[Application] Renderer: " << glGetString(GL_RENDERER) << std::endl;
}

void Application::setupDefaultScene_()
{
	// Set up a default light (args: position, color, intensity)
	sceneRef.addLight(glm::vec3(2.0f, 3.0f, 3.0f), glm::vec3(1.0f), 1.0f);

	// Load the character model by default
	// std::string const path = "assets/models/japanese_classroom/scene.gltf";
	std::string const path = "assets/models/smo_ina/scene.gltf";
	std::string const name = "ina";

	// Add a character model
	auto& registry = registryRef;

	try {
		// Load the model
		std::shared_ptr<Model> model = registry.loadModel(path, name);

		if (model) {
			// Add to scene
			registry.addModelToScene(sceneRef, model);

			// Store entity name in animation state
			animStateRef.entityName = name;

			// Set up camera
			sceneRef.setupCameraToViewEntity(name);
		}

		// Initialize renderer
		rendererRef.init();

	} catch (std::runtime_error const& error) {
		std::cout << error.what() << std::endl;
	}
}

void Application::processInput_(float dt)
{
	// Only process keyboard input for camera if cursor is disabled
	int cursorMode = glfwGetInputMode(window_, GLFW_CURSOR);
	if (cursorMode == GLFW_CURSOR_DISABLED) {
		// Process keyboard input for camera movement
		sceneRef.cam.processKeyboard(dt, window_);
	}

	// Update animation if playing
	if (animStateRef.isAnimating) {
		// Find entity if not set
		if (animStateRef.entityName.empty()) {
			for (auto& entity : sceneRef.ents) {
				if (entity.model && !entity.model->animations.empty()) {
					animStateRef.entityName = entity.model->modelName;
					std::cout << "[Animation] Found entity: " << animStateRef.entityName << std::endl;
					break;
				}
			}
		}

		// Update animation
		auto entOpt = sceneRef.findEntity(animStateRef.entityName);
		if (!entOpt)
			return;

		Entity& entity = entOpt->get();
		auto model = entity.model;
		if (!model || model->animations.empty())
			return;

		// Ensure valid clip index
		if (animStateRef.clipIndex >= model->animations.size())
			animStateRef.clipIndex = 0;

		// Get clip
		auto& clip = model->animations[animStateRef.clipIndex];

		// Advance time
		animStateRef.currentTime += dt * animStateRef.getAnimateSpeed();

		// Loop if needed
		float duration = clip->getDuration();
		if (animStateRef.currentTime > duration) {
			animStateRef.currentTime = std::fmod(animStateRef.currentTime, duration);
		}

		// Update animation frame
		std::cout << "[Animation] Updating frame: time=" << animStateRef.currentTime << ", entity=" << animStateRef.entityName
							<< ", clip=" << animStateRef.clipIndex << std::endl;

		clip->setAnimationFrame(model->nodes, animStateRef.currentTime);
		model->updateMatrices();
	}
}

void Application::tick_(float dt)
{
	// Process input (keyboard, mouse)
	processInput_(dt);

	// Update animation state
	if (animStateRef.isAnimating && !animStateRef.entityName.empty()) {
		// Look up the target entity
		auto entOpt = sceneRef.findEntity(animStateRef.entityName);
		if (!entOpt)
			return;

		Entity& entity = entOpt->get();
		auto model = entity.model;
		if (!model || model->animations.empty())
			return;

		// Ensure clip index is within bounds
		if (animStateRef.clipIndex >= model->animations.size())
			animStateRef.clipIndex = 0;

		// Advance animation time
		auto& clip = model->animations[animStateRef.clipIndex];
		animStateRef.currentTime += dt * animStateRef.getAnimateSpeed();

		// Wrap time if the clip should loop
		float duration = clip->getDuration();
		if (duration > 0.0f && animStateRef.currentTime > duration) {
			animStateRef.currentTime = std::fmod(animStateRef.currentTime, duration);
		}

		// Apply the animation pose and update matrices
		clip->setAnimationFrame(model->nodes, animStateRef.currentTime);
		model->updateMatrices();
	}

	// Update camera matrices
	sceneRef.cam.updateMatrices(window_);

	// Update ImGui windows
	ImGuiManagerRef.newFrame();

	if (showSceneManager_)
		ImGuiManagerRef.drawSceneEntityManager(sceneRef);

	if (showAnimationUI_)
		ImGuiManagerRef.drawAnimationControlPanel(sceneRef);

	if (showStatsWindow_)
		ImGuiManagerRef.drawStatusWindow(sceneRef);

	if (showSceneControlsWindow_)
		ImGuiManagerRef.drawSceneControlWindow(sceneRef);
}

void Application::render_()
{
	int w, h;
	glfwGetFramebufferSize(window_, &w, &h);

	// Begin the frame
	rendererRef.beginFrame(w, h, {0.1f, 0.11f, 0.13f});

	// Draw 3D scene
	rendererRef.drawScene(sceneRef);

	// End frame (cleanup rendering state)
	rendererRef.endFrame();

	// Render ImGui on top of the scene
	ImGuiManagerRef.render();

	// Swap buffers
	glfwSwapBuffers(window_);
}

// Modified loop_ method to use tick_ and render_
void Application::loop_()
{
	prevTime_ = glfwGetTime();

	while (!glfwWindowShouldClose(window_)) {
		// Calculate delta time
		double now = glfwGetTime();
		float dt = float(now - prevTime_);
		prevTime_ = now;

		tick_(dt);
		render_();
		glfwPollEvents();
	}
}

void Application::cleanup_()
{
	// Clean up ImGui
	ImGuiManagerRef.cleanup();

	// Clean up scene resources
	sceneRef.cleanup();

	// Clean up renderer (add this)
	rendererRef.cleanup();

	// Clean up GLFW
	glfwDestroyWindow(window_);
	glfwTerminate();
}
