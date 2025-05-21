#define GLM_ENABLE_EXPERIMENTAL

#include "ImGuiManager.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "AnimationClip.hpp"
#include "ImGuiFileDialog.h"
#include "Model.hpp"
#include "Node.hpp"

ImGuiManager& ImGuiManager::getInstance()
{
	static ImGuiManager instance;
	return instance;
}

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
	currentPath_ = std::filesystem::current_path().string();

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
	// Check if ImGui is already cleaned up
	ImGuiContext* context = ImGui::GetCurrentContext();
	if (!context)
		return;

	// Cleanup ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiManager::loadSelectedModel_(Scene& scene)
{
	if (selectedFile_.empty()) {
		return;
	}

	// Construct full path
	std::string fullPath = (std::filesystem::path(currentPath_) / selectedFile_).string();

	// Use model name or filename if not provided
	// Since most of the model are called scene.gltf, so we used the folder name that contained the model as the default name.
	std::string name = targetModelName_.empty() ? std::filesystem::path(selectedFile_).parent_path().stem().string() // Name of parent folder
																							: targetModelName_;

	// Load the model
	std::shared_ptr<Model> model = registryRef.loadModel(fullPath, name);

	if (model) {
		// Add to scene
		registryRef.addModelToScene(scene, model);

		std::cout << "Model '" << model->modelName << "' loaded successfully from " << fullPath << std::endl;

		// Reset input fields
		targetModelName_.clear();
	}
	else {
		std::cout << "Failed to load model from " << fullPath << std::endl;
	}
}

void ImGuiManager::drawTransformEditor_(Entity& entity)
{
	bool changedPos = ImGui::DragFloat3("Position", glm::value_ptr(entity.position), 0.1f);
	bool changedRot = ImGui::DragFloat3("Rotation (deg)", glm::value_ptr(entity.rotationDeg), 1.0f);
	bool changedScl = ImGui::SliderFloat("Model Scale", &entity.scale, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
	if (ImGui::Button("Reset Scale to 1"))
		entity.scale = 1.0f;

	if (changedPos || changedRot || changedScl)
		entity.rebuildTransform();
}

void ImGuiManager::drawModelLoaderInterface(Scene& scene)
{
	if (ImGui::Button("Load Model")) {
		ImGui::SetNextWindowSize(ImVec2(600, 250), ImGuiCond_FirstUseEver);
		IGFD::FileDialogConfig config;
		config.path = currentPath_;
		config.flags = ImGuiFileDialogFlags_ReadOnlyFileNameField;
		config.sidePane = [this](char const*, IGFD::UserDatas, bool*) {
			char nameBuffer[128];
			std::strncpy(nameBuffer, targetModelName_.c_str(), sizeof(nameBuffer));
			nameBuffer[sizeof(nameBuffer) - 1] = '\0';

			if (ImGui::InputText("Model Name", nameBuffer, IM_ARRAYSIZE(nameBuffer)))
				targetModelName_ = nameBuffer;
		};

		ImGuiFileDialog::Instance()->OpenDialog("ChooseModelDlg", "Choose Model", ".gltf,.glb", config);
	}

	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		// Tell the user they can type the model name in the dialog's file-name field
		ImGui::TextUnformatted("You can type the desired model name directly in the side panel field of the dialog.");
		ImGui::EndTooltip();
	}
	ImGui::Separator();

	if (ImGuiFileDialog::Instance()->Display("ChooseModelDlg")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			selectedFile_ = ImGuiFileDialog::Instance()->GetFilePathName();
			currentPath_ = ImGuiFileDialog::Instance()->GetCurrentPath();
			if (!selectedFile_.empty()) {
				loadSelectedModel_(scene);
			}
		}
		ImGuiFileDialog::Instance()->Close();
	}
}

void ImGuiManager::drawSceneEntityManager(Scene& scene)
{
	ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

	ImGui::Begin("Scene Entities");
	drawModelLoaderInterface(scene);

	{
		float row_h = ImGui::GetFrameHeightWithSpacing();
		int rows = static_cast<int>(scene.ents.size());
		float pad = ImGui::GetStyle().WindowPadding.y * 2.0f;
		float max_h = 300.0f;
		float child_h = std::min(rows * row_h + pad, max_h);

		// Entity list
		ImGui::Text("Loaded Entities:");
		ImGui::BeginChild("Entities", ImVec2(0, child_h), true);

		for (size_t i = 0; i < scene.ents.size(); i++) {
			auto const& entity = scene.ents[i];
			bool isSelected = (selectedEntityIndex_ == static_cast<int>(i));

			if (ImGui::Selectable(entity.model->modelName.c_str(), isSelected)) {
				selectedEntityIndex_ = static_cast<int>(i);
				animStateRef.entityName = entity.model->modelName;
			}
		}
		ImGui::EndChild();
	}

	// Entity controls (only show if an entity is selected)
	if (selectedEntityIndex_ >= 0 && selectedEntityIndex_ < static_cast<int>(scene.ents.size())) {
		ImGui::Separator();
		Entity& entity = scene.ents[selectedEntityIndex_];

		ImGui::Text("Entity: %s", entity.model->modelName.c_str());

		// Visibility toggle
		bool visible = entity.visible;
		if (ImGui::Checkbox("Visible", &visible)) {
			entity.visible = visible;
		}

		// Remove entity buttonS
		ImGui::SameLine();
		if (ImGui::Button("Remove Entity")) {
			registryRef.removeModelFromScene(scene, entity.model->modelName);
			selectedEntityIndex_ = -1; // Reset selection
		}

		ImGui::SameLine();
		if (ImGui::Button("View Selected Entity")) {
			if (selectedEntityIndex_ >= 0 && selectedEntityIndex_ < scene.ents.size()) {
				scene.setupCameraToViewEntity(scene.ents[selectedEntityIndex_].model->modelName);
			}
		}

		// Transform editor
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
			drawTransformEditor_(entity);
	}

	// Show bone hierarchy
	if (selectedEntityIndex_ >= 0 && selectedEntityIndex_ < static_cast<int>(scene.ents.size())) {
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Bone Hierarchy")) {
			Entity& entity = scene.ents[selectedEntityIndex_];
			if (entity.model->rootNode) {
				// Debug info about the node count
				ImGui::Text("Model has %zu nodes", entity.model->nodes.size());

				// Find the root node and all top-level nodes
				std::shared_ptr<Node> rootNode = entity.model->rootNode;
				ImGui::Text("Root node ID: %d, Name: %s", rootNode->nodeNum, rootNode->nodeName.empty() ? "<unnamed>" : rootNode->nodeName.c_str());

				// Show full hierarchy starting at root
				drawNodeTree_(rootNode, 0);

				// If root node doesn't have all nodes as descendants,
				// find potential other top-level nodes
				std::set<int> processedNodes;
				std::function<void(std::shared_ptr<Node>)> collectNodes = [&processedNodes, &collectNodes](std::shared_ptr<Node> node) {
					if (!node)
						return;

					processedNodes.insert(node->nodeNum);
					for (auto& child : node->children) {
						collectNodes(child);
					}
				};

				// Collect all nodes in the hierarchy
				collectNodes(rootNode);

				// Check for disconnected nodes (not in the main hierarchy)
				bool foundDisconnected = false;
				for (size_t i = 0; i < entity.model->nodes.size(); i++) {
					if (entity.model->nodes[i] && processedNodes.find(entity.model->nodes[i]->nodeNum) == processedNodes.end()) {

						if (!foundDisconnected) {
							ImGui::Separator();
							ImGui::Text("Additional nodes not connected to root:");
							foundDisconnected = true;
						}

						// Show each disconnected node
						drawNodeTree_(entity.model->nodes[i], 0);
					}
				}
			}
			else {
				ImGui::Text("No node hierarchy available");
			}
		}
	}

	ImGui::End();
}

void ImGuiManager::drawAnimationControlPanel(Scene& scene)
{
	ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(420, 10), ImGuiCond_FirstUseEver);

	ImGui::Begin("Animation Controls");

	auto entOpt = sceneRef.findEntity(animStateRef.entityName);
	if (!entOpt) {
		ImGui::End();
		return;
	}

	Entity& entity = entOpt->get();
	auto model = entity.model;
	if (!entity.model) {
		ImGui::End();
		return;
	}

	ImGui::Text("Selected Model: %s", entity.model->modelName.c_str());
	ImGui::Text("Model has %zu animations", entity.model->animations.size());

	if (animStateRef.entityName != entity.model->modelName) {
		ImGui::Text("animStateRef.entityName != entity.model->modelName");
		ImGui::End();
		return;
	}

	// Animation clips
	std::vector<std::string> clipNames;
	for (auto const& clip : entity.model->animations)
		clipNames.push_back(clip->clipName);

	if (selectedClipIndex_ >= clipNames.size())
		selectedClipIndex_ = 0;

	if (ImGui::BeginCombo("Animation", clipNames.empty() ? "None" : clipNames[selectedClipIndex_].c_str())) {
		for (int i = 0; i < clipNames.size(); i++) {
			bool isSelected = (selectedClipIndex_ == i);
			if (ImGui::Selectable(clipNames[i].c_str(), isSelected)) {
				selectedClipIndex_ = i;

				// Update global animation state
				animStateRef.clipIndex = selectedClipIndex_;

				// Reset animation visually
				if (model->animations.size() > selectedClipIndex_) {
					model->animations[selectedClipIndex_]->setAnimationFrame(model->nodes, 0.0f);
					model->updateLocalMatrices();
				}
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (clipNames.empty()) {
		ImGui::End();
		return;
	}

	// Speed control
	{
		float speed = animStateRef.getAnimateSpeed();
		if (ImGui::SliderFloat("Speed", &speed, 0.1f, 2.0f)) {
			animStateRef.setAnimateSpeed(speed);
		}
	}

	// Get duration
	float duration = 0.0f;
	if (model->animations.size() > selectedClipIndex_) {
		duration = model->animations[selectedClipIndex_]->getDuration();
	}

	// Time slider
	if (duration > 0.0f) {
		float& currentTime = animStateRef.currentTime;

		if (ImGui::SliderFloat("Time", &currentTime, 0.0f, duration)) {
			// Update animation frame if this is the current entity
			if (model->animations.size() > selectedClipIndex_) {
				model->animations[selectedClipIndex_]->setAnimationFrame(model->nodes, currentTime);
				model->updateLocalMatrices();
			}
		}
	}

	ImGui::Separator();

	// Playback buttons
	if (ImGui::Button("Play", ImVec2(60, 30))) {
		std::cout << "[ImGui] Play button pressed for " << animStateRef.entityName << ", clip " << selectedClipIndex_ << std::endl;

		// Start animation
		animStateRef.play(selectedClipIndex_);

		// Apply initial frame for visual feedback
		if (model->animations.size() > selectedClipIndex_) {
			model->animations[selectedClipIndex_]->setAnimationFrame(model->nodes, 0.0f);
			model->updateLocalMatrices();
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Pause", ImVec2(60, 30))) {
		std::cout << "[ImGui] Pause button pressed" << std::endl;
		animStateRef.pause();
	}
	ImGui::SameLine();

	if (ImGui::Button("Resume", ImVec2(70, 30))) {
		std::cout << "[ImGui] Resume button pressed" << std::endl;

		if (animStateRef.clipIndex != selectedClipIndex_) {
			// If different entity/clip, start animation
			animStateRef.play(selectedClipIndex_, animStateRef.currentTime);
		}
		else {
			// Otherwise just resume
			animStateRef.resume();
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Stop", ImVec2(60, 30))) {
		std::cout << "[ImGui] Stop button pressed" << std::endl;
		animStateRef.stop();

		// Reset visually
		if (model->animations.size() > selectedClipIndex_) {
			model->animations[selectedClipIndex_]->setAnimationFrame(model->nodes, 0.0f);
			model->updateLocalMatrices();
		}
	}

	// Animation state display
	ImGui::Text("Animation State: %s", animStateRef.isAnimating ? "Playing" : "Stopped");

	if (animStateRef.isAnimating) {
		ImGui::Text("Current Time: %.2f / %.2f", animStateRef.currentTime, duration);

		// Progress bar
		float progress = duration > 0.0f ? (animStateRef.currentTime / duration) : 0.0f;
		std::string progressStr = std::to_string(static_cast<int>(progress * 100)) + "%";
		ImGui::ProgressBar(progress, ImVec2(-1, 0), progressStr.c_str());
	}

	ImGui::End();
}

// Implement the node hierarchy display function
void ImGuiManager::drawNodeTree_(std::shared_ptr<Node> node, int depth)
{
	if (!node) {
		return;
	}

	// Get node information
	int nodeNum = node->nodeNum;
	std::string nodeName = node->nodeName.empty() ? "Node_" + std::to_string(nodeNum) : node->nodeName;

	// Format position for display
	glm::vec3 translation = node->translation;
	std::string posStr = "(" + std::to_string(translation.x).substr(0, 5) + ", " + std::to_string(translation.y).substr(0, 5) + ", " +
											 std::to_string(translation.z).substr(0, 5) + ")";

	// Colorize by node type or position
	if (nodeNum == 0) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f)); // Red for root
	}
	else if (nodeName.find("spine") != std::string::npos || nodeName.find("arm") != std::string::npos || nodeName.find("leg") != std::string::npos ||
					 nodeName.find("hand") != std::string::npos || nodeName.find("foot") != std::string::npos || nodeName.find("head") != std::string::npos ||
					 nodeName.find("joint") != std::string::npos) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f)); // Green for joints
	}
	else if (translation.y > 100.0f) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.5f, 1.0f)); // Yellow for high positions (head)
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 1.0f, 1.0f)); // Light blue for others
	}

	// Create label with node info
	std::string label = nodeName + " [" + std::to_string(nodeNum) + "] ";

	// Open/closed state of tree node
	bool opened = ImGui::TreeNode((label + "###node" + std::to_string(nodeNum)).c_str());
	ImGui::PopStyleColor();

	// Always show position info
	ImGui::SameLine();
	ImGui::TextDisabled("Pos: %s", posStr.c_str());

	if (opened) {
		// Show detailed node info
		ImGui::Indent();

		// Show rotation
		glm::quat rotation = node->rotation;
		ImGui::Text("Rotation: (w=%.2f, x=%.2f, y=%.2f, z=%.2f)", rotation.w, rotation.x, rotation.y, rotation.z);

		// Show scale
		glm::vec3 scale = node->scale;
		ImGui::Text("Scale: (%.2f, %.2f, %.2f)", scale.x, scale.y, scale.z);

		// Show child count
		size_t childCount = node->children.size();
		ImGui::Text("Children: %zu", childCount);

		// Process all children
		for (auto const& child : node->children) {
			drawNodeTree_(child, depth + 1);
		}

		ImGui::Unindent();
		ImGui::TreePop();
	}
}

void ImGuiManager::drawStatusWindow(Scene& scene)
{
	ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(10, 470), ImGuiCond_FirstUseEver);

	ImGui::Begin("Statistics");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("Scene entities: %zu", scene.ents.size());
	ImGui::Text("Press TAB to toggle camera mode");
	ImGui::Text("F1-F4 to toggle UI windows");

	// Show animation state if active
	if (animStateRef.isAnimating)
		ImGui::Text("Animating: %s (clip %d, time %.2f)", animStateRef.entityName.c_str(), animStateRef.clipIndex, animStateRef.currentTime);

	ImGui::End();
}

void ImGuiManager::drawSceneControlWindow(Scene& scene)
{
	ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(830, 10), ImGuiCond_FirstUseEver);

	ImGui::Begin("Scene Controls");

	// Skeleton visualization controls
	ImGui::Text("Visualization Options:");
	bool& showModels = rendererRef.showModels;
	bool& showWireFrame = rendererRef.showWireFrame;
	bool& showSkybox = rendererRef.showSkybox;
	bool& showSkeletons = rendererRef.showSkeletons;
	bool& showLightPoint = rendererRef.showLightPoint;
	bool& showBBox = rendererRef.showBBox;

	if (ImGui::Checkbox("Show Model", &showModels))
		std::cout << "[ImGui INFO] Setting model visibility to: " << (showModels ? "ON" : "OFF") << std::endl;

	if (ImGui::Checkbox("Show Wire Frame", &showWireFrame))
		std::cout << "[ImGui INFO] Setting Wire Frame visibility to: " << (showWireFrame ? "ON" : "OFF") << std::endl;

	if (ImGui::Checkbox("Show Skybox", &showSkybox))
		std::cout << "[ImGui INFO] Setting Skybox visibility to: " << (showSkybox ? "ON" : "OFF") << std::endl;

	if (ImGui::Checkbox("Show Skeleton", &showSkeletons))
		std::cout << "[ImGui INFO] Setting skeleton visibility to: " << (showSkeletons ? "ON" : "OFF") << std::endl;

	if (ImGui::Checkbox("Show Light Point", &showLightPoint))
		std::cout << "[ImGui INFO] Setting skeleton visibility to: " << (showLightPoint ? "ON" : "OFF") << std::endl;

	if (ImGui::Checkbox("Show AABB Bounding Box", &showBBox))
		std::cout << "[ImGui INFO] Setting skeleton visibility to: " << (showBBox ? "ON" : "OFF") << std::endl;

	ImGui::Separator();

	// Camera section
	if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Camera position
		glm::vec3 camPos = scene.cam.pos;
		if (ImGui::DragFloat3("Position", glm::value_ptr(camPos), 0.1f)) {
			scene.cam.pos = camPos;
		}

		// Camera speed
		float& camSpeed = animStateRef.camSpeed;
		ImGui::SliderFloat("Camera Speed", &camSpeed, 0.5f, 10.0f);

		// Camera direction (read-only)
		ImGui::Text("Direction: (%.2f, %.2f, %.2f)", scene.cam.front.x, scene.cam.front.y, scene.cam.front.z);

		// Character mode, make camera following entity
		bool& charMode = animStateRef.characterMoveMode;
		ImGui::Checkbox("Enable Character Move", &charMode);
		if (charMode) {
			ImGui::SliderFloat("Follow Distance", &animStateRef.followDistance, 1.0f, 50.0f);
			ImGui::SliderFloat("Follow Height", &animStateRef.followHeight, 1.0f, 50.0f);
		}

		// Camera view reset buttons
		if (ImGui::Button("Reset Camera")) {
			scene.setupCameraToViewScene(1.2f);
		}

		ImGui::SameLine();

		if (ImGui::Button("View Selected Entity")) {
			if (selectedEntityIndex_ >= 0 && selectedEntityIndex_ < scene.ents.size()) {
				scene.setupCameraToViewEntity(scene.ents[selectedEntityIndex_].model->modelName);
			}
		}
	}

	// Lighting section
	if (ImGui::CollapsingHeader("Lighting Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (size_t i = 0; i < scene.lights.size(); i++) {
			ImGui::PushID(static_cast<int>(i));

			std::string lightLabel = "Light " + std::to_string(i + 1);
			if (ImGui::TreeNode(lightLabel.c_str())) {
				// Light position
				glm::vec3 lightPos = scene.lights[i].position;
				if (ImGui::DragFloat3("Position", glm::value_ptr(lightPos), 0.1f)) {
					scene.lights[i].position = lightPos;
				}

				// Light color with color picker
				glm::vec3 lightColor = scene.lights[i].color;
				if (ImGui::ColorEdit3("Color", glm::value_ptr(lightColor))) {
					scene.lights[i].color = lightColor;
				}

				// Light intensity
				float lightIntensity = scene.lights[i].intensity;
				if (ImGui::SliderFloat("Intensity", &lightIntensity, 0.0f, 5.0f)) {
					scene.lights[i].intensity = lightIntensity;
				}

				// Add new light button
				if (ImGui::Button("Add Light") && scene.lights.size() < 10) {
					scene.addLight(glm::vec3(0.0f, 5.0f, 0.0f), // default position
												 glm::vec3(1.0f, 1.0f, 1.0f), // default color (white)
												 1.0f													// default intensity
					);
				}

				// Remove light button
				ImGui::SameLine();
				if (ImGui::Button("Remove Light") && scene.lights.size() > 1) {
					// Remove this light (if there's more than one)
					scene.lights.erase(scene.lights.begin() + i);
					ImGui::TreePop();
					ImGui::PopID();
					break; // Exit the loop since we've modified the array
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		// If there are no lights, provide a button to add one
		if (scene.lights.empty()) {
			if (ImGui::Button("Add Light")) {
				scene.addLight(glm::vec3(2.0f, 3.0f, 3.0f), // default position
											 glm::vec3(1.0f, 1.0f, 1.0f), // default color (white)
											 1.0f													// default intensity
				);
			}
		}
	}

	ImGui::End();
}
