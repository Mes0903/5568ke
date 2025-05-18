#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "include_5568ke.hpp"

class Node;
class Scene;
struct Entity;

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

private:
	ImGuiManager() = default;
	~ImGuiManager() = default;

	// File browser state
	std::string currentPath;
	std::string selectedFile;
	std::vector<std::string> fileList;

	// Model loading state
	std::string modelName;
	std::array<float, 3> modelRotation{};
	std::array<float, 3> modelPosition{};

	// Scene management state
	int selectedEntityIndex{-1};

	// Utility functions
	void refreshFileList();
	void loadSelectedModel(Scene& scene);
	void drawTransformEditor(Entity& entity);
	void drawNodeHierarchy(std::shared_ptr<Node> node, int depth);
};
