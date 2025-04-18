#pragma once

#include <GLFW/glfw3.h>

#include "Camera.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"

// -----------------------------------------------------------------
// Application  — top‑level orchestrator (window, input, main loop)
// -----------------------------------------------------------------
class Application {
public:
	int run();

private:
	void initWindow();
	void initGL();
	void loop();
	void drawFrame();
	void cleanup();

private:
	GLFWwindow* window_ = nullptr;

	Camera cam_;
	Scene scene_;				// entities + lights
	Renderer renderer_; // owns GPU pipeline

	double prevTime_ = 0.0;		 // high‑res frame timer
	float modelScale_ = 0.06f; // scale factor for models
};
