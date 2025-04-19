#pragma once

#include "include_5568ke.hpp"

#include <array>
#include <string>
#include <unordered_map>

#include "ImGuiManager.hpp"
#include "ModelRegistry.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"

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
	void setupDefaultFormat_();

	// Main loop_ methods
	void loop_();
	void drawFrame_();
	void processInput_(float dt);

	// Cleanup
	void cleanup_();

private:
	GLFWwindow* window_ = nullptr;
	Scene scene_;
	Renderer renderer_;
	double prevTime_ = 0.0;

	// ImGui management
	bool showModelLoader_ = true;
	bool showSceneManager_ = true;
	bool showStatsWindow_ = true;

	// Key state tracking
	std::array<bool, 1024> keys_ = {false};
	std::array<bool, 1024> prevKeys_ = {false}; // For detecting key press events

	// Track which scene is currently loaded
	std::string currentScene_ = "default";

	// Callbacks
	static void keyCallback_(GLFWwindow* window, int key, int scancode, int action, int mode);
	static void mouseCallback_(GLFWwindow* window, double xpos, double ypos);
	static void scrollCallback_(GLFWwindow* window, double xoffset, double yoffset);
};