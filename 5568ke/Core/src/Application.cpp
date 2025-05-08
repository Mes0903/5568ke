#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Application.hpp"

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
	setupDefaultFormat_();
	setupDefaultScene_();

	// Enter the main loop
	mainLoop_();

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
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	// Toggle UI windows with function keys
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_F1) {
			app->showModelLoader_ = !app->showModelLoader_;
		}
		else if (key == GLFW_KEY_F2) {
			app->showSceneManager_ = !app->showSceneManager_;
		}
		else if (key == GLFW_KEY_F3) {
			app->showStatsWindow_ = !app->showStatsWindow_;
		}
		else if (key == GLFW_KEY_F4) {
			app->showAnimationControls_ = !app->showAnimationControls_;
			ImGuiManager::getInstance().setAnimationControlsVisible(app->showAnimationControls_);
		}
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
	// Handle zoom or other scroll behaviors if needed
	// For example, adjust camera FOV or distance
	Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

	// Can implement camera zoom here
	// app->scene_.cam.processScroll(yoffset);
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

void Application::setupDefaultFormat_()
{
	renderer_.setupDefaultRenderer();

	// Set up format-specific defaults for different model formats
	auto& registry = ModelRegistry::getInstance();

	// GLTF models often use a right-handed coordinate system with Y-up
	registry.setFormatDefaults(ModelFormat::GLTF,
														 1.0f,						 // Default scale
														 glm::vec3(0.0f),	 // No rotation needed
														 glm::vec3(0.0f)); // No translation
}

void Application::setupDefaultScene_()
{
	// Set up a default light (args: position, color, intensity)
	scene_.addLight(glm::vec3(2.0f, 3.0f, 3.0f), glm::vec3(1.0f), 1.0f);

	// Load the character model by default
	std::string const path = "assets/models/smo_ina/scene.gltf";
	std::string const name = "ina";

	// Add a character model
	auto& registry = ModelRegistry::getInstance();

	try {
		// Load the model
		Model* model = registry.loadModel(path, name);

		if (model) {
			std::cout << "[Application] Model loaded successfully with " << model->meshes.size() << " meshes" << std::endl;

			if (!model->meshes.empty())
				std::cout << "[Application] First mesh has " << model->meshes[0].vertices.size() << " vertices" << std::endl;

			// Add to scene with a centered transform at specified position
			registry.addModelToSceneCentered(scene_, model, name, glm::vec3(0.0f, 0.0f, 0.0f), // position
																			 glm::vec3(0.0f),																	 // rotation
																			 1.0f);																						 // scale

			// Position camera to view the model properly
			scene_.setupCameraToViewEntity(name, 3.0f);

			// If model has animations, show animation controls
			if (model->hasAnimations) {
				showAnimationControls_ = true;
				ImGuiManager::getInstance().setAnimationControlsVisible(true);
			}
		}
	} catch (std::runtime_error const& error) {
		std::cerr << error.what() << std::endl;
	}
}

void Application::processInput_(float dt)
{
	// Only process keyboard input for camera if cursor is disabled
	int cursorMode = glfwGetInputMode(window_, GLFW_CURSOR);
	if (cursorMode == GLFW_CURSOR_DISABLED) {
		// Process keyboard input for camera movement
		scene_.cam.processKeyboard(dt, window_);
	}

	// Additional input processing
	if (keys_[GLFW_KEY_R]) {
		// Reset camera position
		scene_.setupCameraToViewScene();
	}

	// Track key state transitions
	for (int i = 0; i < 1024; i++) {
		prevKeys_[i] = keys_[i];
	}
}

void Application::mainLoop_()
{
	prevTime_ = glfwGetTime();

	// Define the fixed time step for updates
	double const fixedTimeStep = 1.0 / 60.0; // 60 FPS
	double accumulator = 0.0;

	while (!glfwWindowShouldClose(window_)) {
		// Measure time
		double currentTime = glfwGetTime();
		double frameTime = currentTime - prevTime_;
		prevTime_ = currentTime;

		// Prevent spiral of death by capping frameTime
		if (frameTime > 0.25) {
			frameTime = 0.25;
		}

		// Accumulate time for fixed updates
		accumulator += frameTime;

		// Poll events before any updates
		glfwPollEvents();

		// Process fixed updates
		while (accumulator >= fixedTimeStep) {
			tick_(fixedTimeStep);
			accumulator -= fixedTimeStep;
		}

		// Render with the interpolation factor
		float alpha = accumulator / fixedTimeStep;
		draw_(alpha);

		// Swap buffers
		glfwSwapBuffers(window_);
	}
}

void Application::tick_(float dt)
{
	// Process input and update game state
	processInput_(dt);

	// Update camera matrices
	scene_.cam.updateMatrices(window_);

	// Update animations for all models in the scene
	for (auto& entity : scene_.ents) {
		if (entity.visible && entity.model && entity.model->hasAnimations) {
			entity.model->updateAnimation(dt);
		}
	}
}

void Application::draw_(float interpolation)
{
	// Start new ImGui frame
	ImGuiManager::getInstance().newFrame();

	// Draw ImGui windows
	if (showModelLoader_) {
		ImGuiManager::getInstance().drawModelLoaderInterface(scene_);
	}

	if (showSceneManager_) {
		ImGuiManager::getInstance().drawSceneEntityManager(scene_);
	}

	if (showAnimationControls_) {
		ImGuiManager::getInstance().drawAnimationControls(scene_);
	}

	if (showStatsWindow_) {
		// Simple stats window
		ImGui::Begin("Statistics");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("Scene entities: %zu", scene_.ents.size());
		ImGui::Text("Press TAB to toggle camera mode");
		ImGui::Text("F1-F3 to toggle UI windows");

		// Show animation stats if any model has animations
		bool hasAnimations = false;
		for (auto& entity : scene_.ents) {
			if (entity.model && entity.model->hasAnimations) {
				hasAnimations = true;
				break;
			}
		}

		if (hasAnimations) {
			ImGui::Text("F4 to toggle animation controls");
		}

		ImGui::End();
	}

	// Draw the 3D scene
	drawScene_();

	// Render ImGui on top of the scene
	ImGuiManager::getInstance().render();
}

void Application::drawScene_()
{
	int w, h;
	glfwGetFramebufferSize(window_, &w, &h);
	renderer_.beginFrame(w, h, {0.1f, 0.11f, 0.13f});
	renderer_.drawScene(scene_);
	renderer_.endFrame();
}

void Application::cleanup_()
{
	// Clean up ImGui
	ImGuiManager::getInstance().cleanup();

	// Clean up model registry resources
	ModelRegistry::getInstance().cleanup();

	// Clean up scene resources
	scene_.cleanup();

	// Clean up GLFW
	glfwDestroyWindow(window_);
	glfwTerminate();
}