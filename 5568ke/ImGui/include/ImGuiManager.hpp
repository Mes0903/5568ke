#pragma once

#include "include_5568ke.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "ModelRegistry.hpp"
#include "Scene.hpp"

class ImGuiManager {
public:
	static ImGuiManager& getInstance()
	{
		static ImGuiManager instance;
		return instance;
	}

	// Initialize ImGui
	bool init(GLFWwindow* window);

	// Start a new ImGui frame
	void newFrame();

	// Render ImGui
	void render();

	// Cleanup ImGui
	void cleanup();

	// Draw the model loader interface
	void drawModelLoaderInterface(Scene& scene);

	// Draw the scene entity manager interface
	void drawSceneEntityManager(Scene& scene);

private:
	ImGuiManager() = default;
	~ImGuiManager() = default;

	// File browser state
	std::string currentPath = ".";
	std::string selectedFile;
	std::vector<std::string> fileList;

	// Model loading state
	std::string modelName;
	float modelScale = 1.0f;
	float modelRotation[3] = {0.0f, 0.0f, 0.0f};
	float modelPosition[3] = {0.0f, 0.0f, 0.0f};

	// Scene management state
	int selectedEntityIndex = -1;

	// Utility functions
	void refreshFileList();
	void loadSelectedModel(Scene& scene);
	void drawTransformEditor(glm::mat4& transform);
};