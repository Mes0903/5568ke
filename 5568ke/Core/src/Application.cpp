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

void Application::initImGui_()
{
	// Initialize ImGui
	ImGuiManager::getInstance().init(window_);
}

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
}

void Application::loop_()
{
	prevTime_ = glfwGetTime();
	while (!glfwWindowShouldClose(window_)) {
		double now = glfwGetTime();
		float dt = float(now - prevTime_);
		prevTime_ = now;

		// Start new ImGui frame
		ImGuiManager::getInstance().newFrame();

		processInput_(dt);
		scene_.cam.updateMatrices(window_);

		// Draw ImGui windows
		if (showModelLoader_) {
			ImGuiManager::getInstance().drawModelLoaderInterface(scene_);
		}

		if (showSceneManager_) {
			ImGuiManager::getInstance().drawSceneEntityManager(scene_);
		}

		if (showStatsWindow_) {
			// Simple stats window
			ImGui::Begin("Statistics");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("Scene entities: %zu", scene_.ents.size());
			ImGui::Text("Press TAB to toggle camera mode");
			ImGui::Text("F1-F3 to toggle UI windows");
			ImGui::End();
		}

		drawFrame_();

		// Render ImGui on top of the scene
		ImGuiManager::getInstance().render();

		glfwSwapBuffers(window_);
		glfwPollEvents();
	}
}

void Application::drawFrame_()
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