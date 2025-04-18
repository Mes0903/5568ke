#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Application.hpp"
#include "BlinnPhongMaterial.hpp"
#include "Texture.hpp"

int Application::run() {
	initWindow();
	initGL();

	/* -------- load resources once -------- */
	static Model shion;
	shion.load("assets/models/sion/sion.pmx");
	static BlinnPhongMaterial blinn;

	scene_.entities().push_back({&shion, &blinn, glm::scale(glm::mat4(1), glm::vec3(modelScale_))});

	loop();
	cleanup();
	return 0;
}

void Application::initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window_ = glfwCreateWindow(1280, 720, "5568ke", nullptr, nullptr);
	glfwMakeContextCurrent(window_);
	glfwSwapInterval(1);

	glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(window_, this);
	glfwSetCursorPosCallback(window_, [](GLFWwindow* win, double x, double y) {
		auto* app = static_cast<Application*>(glfwGetWindowUserPointer(win));
		app->cam_.processMouse(x, y);
	});
}

void Application::initGL() { gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); }

void Application::loop() {
	prevTime_ = glfwGetTime();
	while (!glfwWindowShouldClose(window_)) {
		double now = glfwGetTime();
		float dt = float(now - prevTime_);
		prevTime_ = now;
		cam_.processKeyboard(dt, window_); // move
		cam_.updateMatrices(window_);			 // build view/proj
		drawFrame();
		glfwSwapBuffers(window_);
		glfwPollEvents();
	}
}

void Application::drawFrame() {
	int w, h;
	glfwGetFramebufferSize(window_, &w, &h);
	renderer_.beginFrame(w, h, {0.1f, 0.11f, 0.13f});

	for (auto const& e : scene_.entities())
		renderer_.submit(*e.model, *e.mat, e.transform, cam_, scene_.lights());

	renderer_.endFrame();
}

void Application::cleanup() {
	glfwDestroyWindow(window_);
	glfwTerminate();
}
