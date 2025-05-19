#define GLM_ENABLE_EXPERIMENTAL

#include "Application.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "AnimationClip.hpp"
#include "GlobalAnimationState.hpp"
#include "ImGuiManager.hpp"
#include "Model.hpp"
#include "ModelRegistry.hpp"
#include "Renderer.hpp"

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

void Application::initImGui_() { ImGuiManager::getInstance().init(window_); }

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
			Camera& cam = static_cast<Application*>(glfwGetWindowUserPointer(window))->scene_.cam;
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
		app->scene_.cam.processMouse(xpos, ypos);
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
	scene_.addLight(glm::vec3(2.0f, 3.0f, 3.0f), glm::vec3(1.0f), 1.0f);

	// Load the character model by default
	// std::string const path = "assets/models/japanese_classroom/scene.gltf";
	std::string const path = "assets/models/smo_ina/scene.gltf";
	std::string const name = "ina";

	// Add a character model
	auto& registry = ModelRegistry::getInstance();

	try {
		// Load the model
		std::shared_ptr<Model> model = registry.loadModel(path, name);

		if (model) {
			// Initialize the default pose
			model->initializeDefaultPose();

			// Add to scene
			registry.addModelToSceneCentered(scene_, model, name, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f));

			// Store entity name in animation state
			auto& animState = GlobalAnimationState::getInstance();
			animState.entityName = name;

			// Set up camera
			scene_.setupCameraToViewEntity(name);
		}

		// Initialize renderer
		Renderer::getInstance().setupDefaultRenderer();

	} catch (std::runtime_error const& error) {
		std::cout << error.what() << std::endl;
	}
}

void Application::processInput_(float dt)
{
	static bool isAnimating = false;
	static std::string currentAnimatingEntity;
	static int currentAnimationClip = 0;
	static float currentAnimationTime = 0.0f;
	static float animationSpeed = 1.0f;

	// Only process keyboard input for camera if cursor is disabled
	int cursorMode = glfwGetInputMode(window_, GLFW_CURSOR);
	if (cursorMode == GLFW_CURSOR_DISABLED) {
		// Process keyboard input for camera movement
		scene_.cam.processKeyboard(dt, window_);
	}

	auto& animState = GlobalAnimationState::getInstance();

	// Update animation if playing
	if (animState.isAnimating) {
		// Find entity if not set
		if (animState.entityName.empty()) {
			for (auto& entity : scene_.ents) {
				if (entity.model && !entity.model->animations.empty()) {
					animState.entityName = entity.name;
					std::cout << "[Animation] Found entity: " << animState.entityName << std::endl;
					break;
				}
			}
		}

		// Update animation
		Entity* entity = scene_.findEntity(animState.entityName);
		if (entity && entity->model && !entity->model->animations.empty()) {
			// Ensure valid clip index
			if (animState.clipIndex >= entity->model->animations.size()) {
				animState.clipIndex = 0;
			}

			// Get clip
			auto& clip = entity->model->animations[animState.clipIndex];

			// Advance time
			animState.currentTime += dt * animState.getAnimateSpeed();

			// Loop if needed
			float duration = clip->getDuration();
			if (animState.currentTime > duration) {
				animState.currentTime = std::fmod(animState.currentTime, duration);
			}

			// Update animation frame
			std::cout << "[Animation] Updating frame: time=" << animState.currentTime << ", entity=" << animState.entityName << ", clip=" << animState.clipIndex
								<< std::endl;

			clip->setAnimationFrame(entity->model->nodes, animState.currentTime);
			entity->model->updateMatrices();
		}
	}
}

void Application::tick_(float dt)
{
	// Process input (keyboard, mouse)
	processInput_(dt);

	// Get global animation state
	auto& animState = GlobalAnimationState::getInstance();

	// Update animation state
	if (animState.isAnimating && !animState.entityName.empty()) {
		// Find the entity
		Entity* entity = scene_.findEntity(animState.entityName);
		if (entity && entity->model && !entity->model->animations.empty()) {
			// Make sure clip index is valid
			if (animState.clipIndex >= entity->model->animations.size()) {
				animState.clipIndex = 0;
			}

			// Get animation clip
			auto& clip = entity->model->animations[animState.clipIndex];

			// Update animation time
			animState.currentTime += dt * animState.getAnimateSpeed();

			// Loop if needed
			float duration = clip->getDuration();
			if (duration > 0 && animState.currentTime > duration) {
				animState.currentTime = std::fmod(animState.currentTime, duration);
			}

			// Apply animation
			clip->setAnimationFrame(entity->model->nodes, animState.currentTime);
			entity->model->updateMatrices();
		}
	}

	// Update camera matrices
	scene_.cam.updateMatrices(window_);

	// Update ImGui windows
	ImGuiManager::getInstance().newFrame();

	if (showModelLoader_) {
		// ImGuiManager::getInstance().drawModelLoaderInterface(scene_);
	}

	if (showSceneManager_)
		ImGuiManager::getInstance().drawSceneEntityManager(scene_);

	if (showAnimationUI_)
		ImGuiManager::getInstance().drawAnimationControlPanel(scene_);

	if (showStatsWindow_)
		ImGuiManager::getInstance().drawStatusWindow(scene_);

	if (showSceneControlsWindow_)
		ImGuiManager::getInstance().drawSceneControlWindow(scene_);
}

void Application::render_()
{
	int w, h;
	glfwGetFramebufferSize(window_, &w, &h);

	// Begin the frame
	Renderer::getInstance().beginFrame(w, h, {0.1f, 0.11f, 0.13f});

	// Draw 3D scene
	Renderer::getInstance().drawScene(scene_);

	// End frame (cleanup rendering state)
	Renderer::getInstance().endFrame();

	// Render ImGui on top of the scene
	ImGuiManager::getInstance().render();

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
	ImGuiManager::getInstance().cleanup();

	// Clean up model registry resources
	ModelRegistry::getInstance().cleanup();

	// Clean up scene resources
	scene_.cleanup();

	// Clean up renderer (add this)
	Renderer::getInstance().cleanup();

	// Clean up GLFW
	glfwDestroyWindow(window_);
	glfwTerminate();
}
