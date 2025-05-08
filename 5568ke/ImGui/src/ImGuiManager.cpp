#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <iostream>

#include "Animation.hpp"
#include "ImGuiManager.hpp"
#include "Model.hpp"

bool ImGuiManager::init(GLFWwindow* window)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	// Set initial file path to the executable directory
	currentPath = std::filesystem::current_path().string();
	refreshFileList();

	return true;
}

void ImGuiManager::newFrame()
{
	// Start the ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::render()
{
	// Render ImGui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::cleanup()
{
	// Cleanup ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiManager::refreshFileList()
{
	fileList.clear();

	try {
		for (auto const& entry : std::filesystem::directory_iterator(currentPath)) {
			// Add directories
			if (entry.is_directory()) {
				fileList.push_back("[DIR] " + entry.path().filename().string());
			}
			// Add model files
			else {
				std::string ext = entry.path().extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

				if (ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".fbx") {
					fileList.push_back(entry.path().filename().string());
				}
			}
		}
	} catch (std::exception const& e) {
		std::cerr << "Error reading directory: " << e.what() << std::endl;
	}

	// Sort file list (directories first)
	std::sort(fileList.begin(), fileList.end(), [](std::string const& a, std::string const& b) {
		bool aIsDir = a.find("[DIR]") != std::string::npos;
		bool bIsDir = b.find("[DIR]") != std::string::npos;

		if (aIsDir && !bIsDir)
			return true;
		if (!aIsDir && bIsDir)
			return false;
		return a < b;
	});

	// Add parent directory option
	fileList.insert(fileList.begin(), "[DIR] ..");
}

void ImGuiManager::loadSelectedModel(Scene& scene)
{
	if (selectedFile.empty() || selectedFile.find("[DIR]") != std::string::npos) {
		return;
	}

	// Construct full path
	std::string fullPath = (std::filesystem::path(currentPath) / selectedFile).string();

	// Use model name or filename if not provided
	std::string name = modelName.empty() ? std::filesystem::path(selectedFile).stem().string() : modelName;

	// Load the model
	auto& registry = ModelRegistry::getInstance();
	Model* model = registry.loadModel(fullPath, name);

	if (model) {
		// Create transformation matrix
		glm::mat4 transform = glm::mat4(1.0f);

		// Apply transforms in order: scale, rotate, translate
		transform = glm::scale(transform, glm::vec3(modelScale));

		// Apply rotations in XYZ order
		transform = glm::rotate(transform, modelRotation[0], glm::vec3(1.0f, 0.0f, 0.0f));
		transform = glm::rotate(transform, modelRotation[1], glm::vec3(0.0f, 1.0f, 0.0f));
		transform = glm::rotate(transform, modelRotation[2], glm::vec3(0.0f, 0.0f, 1.0f));

		// Apply translation
		transform = glm::translate(transform, glm::vec3(modelPosition[0], modelPosition[1], modelPosition[2]));

		// Add to scene
		registry.addModelToScene(scene, model, name, transform);

		std::cout << "Model '" << name << "' loaded successfully from " << fullPath << std::endl;

		// Reset input fields
		modelName = "";
		modelScale = 1.0f;
		modelRotation[0] = modelRotation[1] = modelRotation[2] = 0.0f;
		modelPosition[0] = modelPosition[1] = modelPosition[2] = 0.0f;
	}
	else {
		std::cerr << "Failed to load model from " << fullPath << std::endl;
	}
}

void ImGuiManager::drawTransformEditor(glm::mat4& transform)
{
	// Extract scale, rotation and translation from the matrix
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;

	glm::decompose(transform, scale, rotation, translation, skew, perspective);

	// Convert quaternion to Euler angles in radians
	glm::vec3 eulerAngles = glm::eulerAngles(rotation);

	// Display and edit position
	float position[3] = {translation.x, translation.y, translation.z};
	if (ImGui::DragFloat3("Position", position, 0.1f)) {
		// Update translation
		transform[3][0] = position[0];
		transform[3][1] = position[1];
		transform[3][2] = position[2];
	}

	// Display and edit scale
	float scaleValues[3] = {scale.x, scale.y, scale.z};
	if (ImGui::DragFloat3("Scale", scaleValues, 0.01f, 0.01f, 100.0f)) {
		// Create a new matrix to avoid issues with decomposition
		glm::mat4 newTransform = glm::mat4(1.0f);

		// Apply scale
		newTransform = glm::scale(newTransform, glm::vec3(scaleValues[0], scaleValues[1], scaleValues[2]));

		// Apply rotation (from the original matrix)
		glm::mat4 rotMat = glm::toMat4(rotation);
		newTransform = newTransform * rotMat;

		// Apply translation
		newTransform[3][0] = position[0];
		newTransform[3][1] = position[1];
		newTransform[3][2] = position[2];

		transform = newTransform;
	}

	// Display and edit rotation (in degrees for UI)
	float rotationDegrees[3] = {glm::degrees(eulerAngles.x), glm::degrees(eulerAngles.y), glm::degrees(eulerAngles.z)};

	if (ImGui::DragFloat3("Rotation", rotationDegrees, 1.0f, -360.0f, 360.0f)) {
		// Create a new matrix to avoid issues with decomposition
		glm::mat4 newTransform = glm::mat4(1.0f);

		// Apply scale
		newTransform = glm::scale(newTransform, scale);

		// Apply rotation
		newTransform = glm::rotate(newTransform, glm::radians(rotationDegrees[0]), glm::vec3(1.0f, 0.0f, 0.0f));
		newTransform = glm::rotate(newTransform, glm::radians(rotationDegrees[1]), glm::vec3(0.0f, 1.0f, 0.0f));
		newTransform = glm::rotate(newTransform, glm::radians(rotationDegrees[2]), glm::vec3(0.0f, 0.0f, 1.0f));

		// Apply translation
		newTransform[3][0] = position[0];
		newTransform[3][1] = position[1];
		newTransform[3][2] = position[2];

		transform = newTransform;
	}
}

void ImGuiManager::drawModelLoaderInterface(Scene& scene)
{
	ImGui::Begin("Model Loader");

	// File Browser Section
	if (ImGui::CollapsingHeader("File Browser", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Display current path
		ImGui::Text("Current Path: %s", currentPath.c_str());

		// Refresh button
		if (ImGui::Button("Refresh")) {
			refreshFileList();
		}

		ImGui::Separator();

		// File List
		ImGui::BeginChild("Files", ImVec2(0, 200), true);
		for (auto const& file : fileList) {
			bool isSelected = (file == selectedFile);
			if (ImGui::Selectable(file.c_str(), isSelected)) {
				selectedFile = file;

				// Handle directory navigation
				if (file.find("[DIR]") != std::string::npos) {
					std::string dirName = file.substr(6); // Remove "[DIR] " prefix

					if (dirName == "..") {
						// Navigate to parent directory
						currentPath = std::filesystem::path(currentPath).parent_path().string();
					}
					else {
						// Navigate to subdirectory
						currentPath = (std::filesystem::path(currentPath) / dirName).string();
					}

					refreshFileList();
					selectedFile = ""; // Clear selection after navigation
				}
			}
		}
		ImGui::EndChild();
	}

	ImGui::Separator();

	// Model Loading Section
	if (ImGui::CollapsingHeader("Model Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Display selected file
		ImGui::Text("Selected File: %s", selectedFile.c_str());

		// Model name input
		char nameBuffer[128] = "";
		if (!modelName.empty()) {
			std::strncpy(nameBuffer, modelName.c_str(), sizeof(nameBuffer) - 1);
		}
		if (ImGui::InputText("Model Name", nameBuffer, sizeof(nameBuffer))) {
			modelName = nameBuffer;
		}

		// Transform controls
		ImGui::SliderFloat("Scale", &modelScale, 0.01f, 10.0f);

		ImGui::Text("Rotation (radians):");
		ImGui::SliderFloat("Rotation X", &modelRotation[0], -3.14159f, 3.14159f);
		ImGui::SliderFloat("Rotation Y", &modelRotation[1], -3.14159f, 3.14159f);
		ImGui::SliderFloat("Rotation Z", &modelRotation[2], -3.14159f, 3.14159f);

		ImGui::Text("Position:");
		ImGui::DragFloat3("Position", modelPosition, 0.1f);

		// Load button
		if (ImGui::Button("Load Model") && !selectedFile.empty() && selectedFile.find("[DIR]") == std::string::npos) {
			loadSelectedModel(scene);
		}
	}

	ImGui::End();
}

void ImGuiManager::drawAnimationControls(Scene& scene)
{
	ImGui::Begin("Animation Controls");

	// If no entity is selected, show a message
	if (selectedEntityIndex < 0 || selectedEntityIndex >= static_cast<int>(scene.ents.size())) {
		ImGui::Text("Select an entity to control its animations");
		ImGui::End();
		return;
	}

	// Get the selected entity
	Entity& entity = scene.ents[selectedEntityIndex];
	if (!entity.model || !entity.model->hasAnimations) {
		ImGui::Text("Selected entity has no animations");
		ImGui::End();
		return;
	}

	// Get animation player from the model
	AnimationPlayer& player = entity.model->animationPlayer;

	// Display animation name and controls
	ImGui::Text("Model: %s", entity.name.c_str());

	// Animation selection dropdown
	size_t animCount = player.getAnimationCount();
	if (animCount > 0) {
		std::string currentAnim = player.getCurrentAnimationName();
		if (ImGui::BeginCombo("Animation", currentAnim.c_str())) {
			for (size_t i = 0; i < animCount; i++) {
				std::string animName = player.getAnimationName(static_cast<int>(i));
				bool isSelected = (currentAnim == animName);
				if (ImGui::Selectable(animName.c_str(), isSelected)) {
					player.setAnimation(static_cast<int>(i));
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		// Playback controls
		bool isPlaying = player.isPlaying();
		if (ImGui::Button(isPlaying ? "Pause" : "Play")) {
			if (isPlaying) {
				player.pause();
			}
			else {
				player.play();
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Stop")) {
			player.stop();
		}

		ImGui::SameLine();

		// Loop toggle
		static bool loop = true;
		if (ImGui::Checkbox("Loop", &loop)) {
			player.setLooping(loop);
		}

		// Animation progress slider
		float progress = player.getProgress();
		if (ImGui::SliderFloat("Progress", &progress, 0.0f, 1.0f, "%.2f")) {
			player.setProgress(progress);
		}

		// Animation speed slider
		static float speed = 1.0f;
		if (ImGui::SliderFloat("Speed", &speed, 0.1f, 3.0f, "%.2f")) {
			player.setSpeed(speed);
		}

		// Animation duration display
		float duration = player.getCurrentDuration();
		ImGui::Text("Duration: %.2f seconds", duration);
	}
	else {
		ImGui::Text("No animations available");
	}

	ImGui::End();
}

void ImGuiManager::drawSceneEntityManager(Scene& scene)
{
	ImGui::Begin("Scene Entities");

	// Entity list
	ImGui::Text("Loaded Entities:");
	ImGui::BeginChild("Entities", ImVec2(0, 200), true);

	for (size_t i = 0; i < scene.ents.size(); i++) {
		auto const& entity = scene.ents[i];
		bool isSelected = (selectedEntityIndex == static_cast<int>(i));

		if (ImGui::Selectable(entity.name.c_str(), isSelected)) {
			selectedEntityIndex = static_cast<int>(i);
		}
	}
	ImGui::EndChild();

	ImGui::Separator();

	// Entity controls (only show if an entity is selected)
	if (selectedEntityIndex >= 0 && selectedEntityIndex < static_cast<int>(scene.ents.size())) {
		Entity& entity = scene.ents[selectedEntityIndex];

		ImGui::Text("Entity: %s", entity.name.c_str());

		// Visibility toggle
		bool visible = entity.visible;
		if (ImGui::Checkbox("Visible", &visible)) {
			entity.visible = visible;
		}

		// Transform editor
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			drawTransformEditor(entity.transform);
		}

		// Animation info if available
		if (entity.model && entity.model->hasAnimations) {
			ImGui::Text("Has animations: %zu", entity.model->animations.size());

			// Show a button to open animation controls
			if (ImGui::Button("Animation Controls")) {
				showAnimationControls_ = true;
			}
		}

		// Remove entity button
		if (ImGui::Button("Remove Entity")) {
			ModelRegistry::getInstance().removeModelFromScene(scene, entity.name);
			selectedEntityIndex = -1; // Reset selection
		}

		// Focus camera on entity button
		if (ImGui::Button("Focus Camera")) {
			scene.setupCameraToViewEntity(entity.name);
		}
	}

	ImGui::End();
}