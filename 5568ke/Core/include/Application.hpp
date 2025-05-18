#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include "Scene.hpp"
#include "include_5568ke.hpp"

class Application {
public:
	Application();
	~Application();

	int run();

private:
	// Initialization methods
	void initWindow_();
	void initGL_();
	void initImGui_();

	// Scene setup methods
	void setupDefaultScene_();

	// Main loop methods
	void loop_();
	void processInput_(float dt);

	// New tick and render methods
	void tick_(float dt);
	void render_();

	// Cleanup
	void cleanup_();

private:
	GLFWwindow* window_{nullptr};
	Scene scene_;
	double prevTime_{0.0};

	// ImGui management
	bool showModelLoader_{true};
	bool showSceneManager_{true};
	bool showAnimationUI_{true};
	bool showStatsWindow_{true};
	bool showSceneControlsWindow_{true};

	// Key state tracking
	std::array<bool, 1024> keys_{};
	std::array<bool, 1024> prevKeys_{}; // For detecting key press events

	// Track which scene is currently loaded
	std::string currentScene_{"default"};

	// Callbacks
	static void keyCallback_(GLFWwindow* window, int key, int scancode, int action, int mode);
	static void mouseCallback_(GLFWwindow* window, double xpos, double ypos);
	static void scrollCallback_(GLFWwindow* window, double xoffset, double yoffset);

	int selectedAnimationClip_{};
	float animationSpeed_{1.0f};
};
