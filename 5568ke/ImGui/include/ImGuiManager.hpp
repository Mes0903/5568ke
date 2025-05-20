#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "GlobalAnimationState.hpp"
#include "ModelRegistry.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "include_5568ke.hpp"

class Node;
class Scene;
class Entity;

class ImGuiManager {
public:
	static ImGuiManager& getInstance();
	bool init(GLFWwindow* window);
	void cleanup();

	void newFrame();
	void render();

	void drawStatusWindow(Scene& scene);					// Info window
	void drawModelLoaderInterface(Scene& scene);	// Model loader interface
	void drawSceneEntityManager(Scene& scene);		// Scene entity manager interface
	void drawAnimationControlPanel(Scene& scene); // Animation UI and skeleton control
	void drawSceneControlWindow(Scene& scene);

public:
	Scene& sceneRef = Scene::getInstance();
	ModelRegistry& registryRef = ModelRegistry::getInstance();
	Renderer& rendererRef = Renderer::getInstance();
	GlobalAnimationState& animStateRef = GlobalAnimationState::getInstance();

private:
	ImGuiManager() = default;
	~ImGuiManager() = default;

	// File browser state
	std::string currentPath_;
	std::string selectedFile_;

	// Model loading state
	std::string targetModelName_;
	std::array<float, 3> targetModelRotation_{};
	std::array<float, 3> targetModelPosition_{};

	// Scene management state
	int selectedEntityIndex_{-1};
	int selectedAnimEntityIndex_{-1};
	int selectedClipIndex_{-1};

	// Utility functions
	void loadSelectedModel_(Scene& scene);
	void drawTransformEditor_(Entity& entity);
	void drawNodeTree_(std::shared_ptr<Node> node, int depth);
};
