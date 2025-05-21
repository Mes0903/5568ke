#define GLM_ENABLE_EXPERIMENTAL

#include "Application.hpp"

#include <cmath>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "AnimationClip.hpp"
#include "Model.hpp"

Application::Application() {}
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
	bool charMode = animStateRef.characterMoveMode;
	if (cursorMode == GLFW_CURSOR_DISABLED) {
		if (charMode) {
			auto entOpt = sceneRef.findEntity(animStateRef.entityName);
			if (entOpt) {
				Entity& entity = entOpt->get();
				glm::vec3 forward = glm::normalize(glm::vec3(sceneRef.cam.front.x, 0.0f, sceneRef.cam.front.z));
				glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

				glm::vec3 move(0.0f);
				float speed = animStateRef.camSpeed;
				if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
					move += forward;
				if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
					move -= forward;
				if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
					move -= right;
				if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
					move += right;

				// Jump input
				if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS && animStateRef.onGround) {
					animStateRef.verticalVelocity = animStateRef.jumpSpeed;
					animStateRef.onGround = false;
				}

				auto resetClipToFirstFrame = [&](Entity& e) {
					if (!e.model || e.model->animations.empty())
						return;

					int clip = animStateRef.clipIndex;
					if (clip >= e.model->animations.size())
						clip = 0;

					e.model->animations[clip]->setAnimationFrame(e.model->nodes, 0.0f);
					e.model->updateLocalMatrices();
				};

				bool isMoving = glm::length(move) > 0.0f;
				bool startedMoving = isMoving && !animStateRef.wasMoving;
				bool stoppedMoving = !isMoving && animStateRef.wasMoving;

				if (isMoving) {
					// Update position and direction
					move = glm::normalize(move) * speed * dt;
					entity.position += move;

					glm::vec2 dir2D(move.x, move.z);
					if (glm::length(dir2D) > 0.0f)
						entity.rotationDeg.y = glm::degrees(atan2(move.x, move.z));

					entity.rebuildTransform();
				}

				// Changed the statue of animation
				if (startedMoving) {
					animStateRef.play(std::min<int>(animStateRef.clipIndex, entity.model->animations.size() - 1), 0.0f);
					resetClipToFirstFrame(entity); // Start from frame 0
				}
				else if (stoppedMoving) {
					animStateRef.stop();
					resetClipToFirstFrame(entity); // Reset to frame 0
				}

				animStateRef.wasMoving = isMoving;
			}
		}
		else {
			// Camera free movement
			sceneRef.cam.processKeyboard(dt, window_);
		}
	}

	// Update animation if playing
	if (animStateRef.isAnimating) {
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
		model->updateLocalMatrices();
	}
}

void Application::tick_(float dt)
{
	// Process input (keyboard, mouse)
	processInput_(dt);

	// Look up the target entity
	auto entOpt = sceneRef.findEntity(animStateRef.entityName);
	if (!entOpt)
		return;

	Entity& entity = entOpt->get();

	// Update animation state
	if (animStateRef.isAnimating && !animStateRef.entityName.empty()) {
		auto model = entity.model;
		if (!model || model->animations.empty()) {
			// If the selected entity has no animations, stop the animation playback instead of skipping the rest of this tick to keep camera and UI responsive.
			animStateRef.stop();
		}
		else {
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
			model->updateLocalMatrices();
		}
	}

	// Simple gravity and jump physics
	if (animStateRef.characterMoveMode) {
		animStateRef.verticalVelocity -= animStateRef.gravity * dt;
		entity.position.y += animStateRef.verticalVelocity * dt;

		// Ground collision at y = 0
		if (entity.position.y <= 0.0f) {
			entity.position.y = 0.0f;
			animStateRef.verticalVelocity = 0.0f;
			animStateRef.onGround = true;
		}

		entity.rebuildTransform();
	}

	// Update camera position and matrices
	if (animStateRef.characterMoveMode && !animStateRef.entityName.empty())
		sceneRef.cam.updateFollow(entity.position, animStateRef.followDistance, animStateRef.followHeight);

	sceneRef.cam.updateMatrices(window_);
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
