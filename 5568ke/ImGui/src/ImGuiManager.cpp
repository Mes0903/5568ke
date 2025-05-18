#define GLM_ENABLE_EXPERIMENTAL

#include "ImGuiManager.hpp"

#include <algorithm>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "AnimationClip.hpp"
#include "GlobalAnimationState.hpp"
#include "Model.hpp"
#include "ModelRegistry.hpp"
#include "Node.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"

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
	// Check if ImGui is already cleaned up
	ImGuiContext* context = ImGui::GetCurrentContext();
	if (!context)
		return;

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
		std::cout << "Error reading directory: " << e.what() << std::endl;
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
	std::shared_ptr<Model> model = registry.loadModel(fullPath, name);

	if (model) {
		// Create transformation matrix
		glm::mat4 transform = glm::mat4(1.0f);

		// Apply transforms in order: scale, rotate, translate
		transform = glm::scale(transform, glm::vec3(1.0f));

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
		modelRotation[0] = modelRotation[1] = modelRotation[2] = 0.0f;
		modelPosition[0] = modelPosition[1] = modelPosition[2] = 0.0f;
	}
	else {
		std::cout << "Failed to load model from " << fullPath << std::endl;
	}
}

void ImGuiManager::drawTransformEditor(Entity& entity)
{
	// Display and edit scale
	ImGui::Text("Model Scaling:");

	// Add scale slider with logarithmic scale for better control
	ImGui::SliderFloat("Model Scale", &entity.scale, 0.01f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic);

	// Add a reset button for convenience
	if (ImGui::Button("Reset Scale to 1")) {
		entity.scale = 1.0f;
	}
}

void ImGuiManager::drawModelLoaderInterface(Scene& scene) {}

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

	// Skeleton visualization controls
	ImGui::Text("Visualization Options:");
	auto& renderer = Renderer::getInstance();
	bool showSkeletons = renderer.showSkeletons;
	bool showModels = renderer.showModels;

	// Create checkbox for skeleton visibility
	if (ImGui::Checkbox("Show Skeleton", &showSkeletons)) {
		std::cout << "[ImGui] Setting skeleton visibility to: " << (showSkeletons ? "ON" : "OFF") << std::endl;
		renderer.showSkeletons = showSkeletons;
	}

	// Add checkbox for model visibility
	if (ImGui::Checkbox("Show Model", &showModels)) {
		std::cout << "[ImGui] Setting model visibility to: " << (showModels ? "ON" : "OFF") << std::endl;
		renderer.showModels = showModels;
	}

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
			drawTransformEditor(entity);
		}

		// Remove entity button
		if (ImGui::Button("Remove Entity")) {
			ModelRegistry::getInstance().removeModelFromScene(scene, entity.name);
			selectedEntityIndex = -1; // Reset selection
		}

		// Focus camera on entity button
		if (ImGui::Button("Focus Camera")) {
			scene.setupCameraToViewEntity(entity.name, 3.0 * entity.scale);
		}
	}

	ImGui::Separator();
	// Show bone hierarchy
	if (selectedEntityIndex >= 0 && selectedEntityIndex < static_cast<int>(scene.ents.size()) && ImGui::CollapsingHeader("Bone Hierarchy")) {
		Entity& entity = scene.ents[selectedEntityIndex];
		if (entity.model->rootNode) {
			// Debug info about the node count
			ImGui::Text("Model has %zu nodes", entity.model->nodes.size());

			// Find the root node and all top-level nodes
			std::shared_ptr<Node> rootNode = entity.model->rootNode;
			ImGui::Text("Root node ID: %d, Name: %s", rootNode->nodeNum, rootNode->nodeName.empty() ? "<unnamed>" : rootNode->nodeName.c_str());

			// Show full hierarchy starting at root
			drawNodeHierarchy(rootNode, 0);

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
					drawNodeHierarchy(entity.model->nodes[i], 0);
				}
			}
		}
		else {
			ImGui::Text("No node hierarchy available");
		}
	}

	ImGui::End();
}

void ImGuiManager::drawAnimationControlPanel(Scene& scene)
{
	auto& animState = GlobalAnimationState::getInstance();

	ImGui::Begin("Animation Controls");

	// Find entities with animations
	std::vector<std::string> entitiesWithAnimations;
	for (auto const& ent : scene.ents) {
		if (ent.model && !ent.model->animations.empty()) {
			entitiesWithAnimations.push_back(ent.name);
		}
	}

	if (entitiesWithAnimations.empty()) {
		ImGui::Text("No animated entities in scene");
		ImGui::End();
		return;
	}

	// Entity selection
	static int selectedEntityIndex = 0;
	if (selectedEntityIndex >= entitiesWithAnimations.size()) {
		selectedEntityIndex = 0;
	}

	if (ImGui::BeginCombo("Entity", entitiesWithAnimations[selectedEntityIndex].c_str())) {
		for (int i = 0; i < entitiesWithAnimations.size(); i++) {
			bool isSelected = (selectedEntityIndex == i);
			if (ImGui::Selectable(entitiesWithAnimations[i].c_str(), isSelected)) {
				selectedEntityIndex = i;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	std::string entityName = entitiesWithAnimations[selectedEntityIndex];
	Entity* entity = scene.findEntity(entityName);

	if (!entity || !entity->model) {
		ImGui::End();
		return;
	}

	// Display debug info
	ImGui::Text("Model has %zu animations", entity->model->animations.size());

	// Animation clips
	std::vector<std::string> clipNames;
	for (auto const& clip : entity->model->animations) {
		clipNames.push_back(clip->clipName);
	}

	static int selectedClipIndex = 0;
	if (selectedClipIndex >= clipNames.size()) {
		selectedClipIndex = 0;
	}

	if (ImGui::BeginCombo("Animation", clipNames.empty() ? "None" : clipNames[selectedClipIndex].c_str())) {
		for (int i = 0; i < clipNames.size(); i++) {
			bool isSelected = (selectedClipIndex == i);
			if (ImGui::Selectable(clipNames[i].c_str(), isSelected)) {
				selectedClipIndex = i;

				// Update global animation state
				animState.clipIndex = selectedClipIndex;

				// Reset animation visually
				if (entity->model->animations.size() > selectedClipIndex) {
					entity->model->animations[selectedClipIndex]->setAnimationFrame(entity->model->nodes, 0.0f);
					entity->model->updateJointMatrices();
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
		float speed = animState.getAnimateSpeed();
		if (ImGui::SliderFloat("Speed", &speed, 0.1f, 2.0f)) {
			animState.setAnimateSpeed(speed);
		}
	}

	// Get duration
	float duration = 0.0f;
	if (entity->model->animations.size() > selectedClipIndex) {
		duration = entity->model->animations[selectedClipIndex]->getDuration();
	}

	// Time slider
	if (duration > 0.0f) {
		float currentTime = animState.entityName == entityName ? animState.currentTime : 0.0f;

		if (ImGui::SliderFloat("Time", &currentTime, 0.0f, duration)) {
			// Set animation time for preview
			animState.currentTime = currentTime;

			// Update animation frame if this is the current entity
			if (entity->model->animations.size() > selectedClipIndex) {
				entity->model->animations[selectedClipIndex]->setAnimationFrame(entity->model->nodes, currentTime);
				entity->model->updateJointMatrices();
			}
		}
	}

	ImGui::Separator();

	// Playback buttons
	if (ImGui::Button("Play", ImVec2(60, 30))) {
		std::cout << "[ImGui] Play button pressed for " << entityName << ", clip " << selectedClipIndex << std::endl;

		// Start animation
		animState.play(entityName, selectedClipIndex);

		// Apply initial frame for visual feedback
		if (entity->model->animations.size() > selectedClipIndex) {
			entity->model->animations[selectedClipIndex]->setAnimationFrame(entity->model->nodes, 0.0f);
			entity->model->updateJointMatrices();
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Pause", ImVec2(60, 30))) {
		std::cout << "[ImGui] Pause button pressed" << std::endl;
		animState.pause();
	}
	ImGui::SameLine();

	if (ImGui::Button("Resume", ImVec2(70, 30))) {
		std::cout << "[ImGui] Resume button pressed" << std::endl;

		if (animState.entityName != entityName || animState.clipIndex != selectedClipIndex) {
			// If different entity/clip, start animation
			animState.play(entityName, selectedClipIndex, animState.currentTime);
		}
		else {
			// Otherwise just resume
			animState.resume();
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Stop", ImVec2(60, 30))) {
		std::cout << "[ImGui] Stop button pressed" << std::endl;
		animState.stop();

		// Reset visually
		if (entity->model->animations.size() > selectedClipIndex) {
			entity->model->animations[selectedClipIndex]->setAnimationFrame(entity->model->nodes, 0.0f);
			entity->model->updateJointMatrices();
		}
	}

	// Animation state display
	ImGui::Text("Animation State: %s", animState.isAnimating && animState.entityName == entityName ? "Playing" : "Stopped");

	if (animState.isAnimating && animState.entityName == entityName) {
		ImGui::Text("Current Time: %.2f / %.2f", animState.currentTime, duration);

		// Progress bar
		float progress = duration > 0.0f ? (animState.currentTime / duration) : 0.0f;
		std::string progressStr = std::to_string(static_cast<int>(progress * 100)) + "%";
		ImGui::ProgressBar(progress, ImVec2(-1, 0), progressStr.c_str());
	}

	ImGui::End();
}

// Implement the node hierarchy display function
void ImGuiManager::drawNodeHierarchy(std::shared_ptr<Node> node, int depth)
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
			drawNodeHierarchy(child, depth + 1);
		}

		ImGui::Unindent();
		ImGui::TreePop();
	}
}

void ImGuiManager::drawStatusWindow(Scene& scene)
{
	ImGui::Begin("Statistics");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("Scene entities: %zu", scene.ents.size());
	ImGui::Text("Press TAB to toggle camera mode");
	ImGui::Text("F1-F4 to toggle UI windows");

	// Show animation state if active
	auto& animState = GlobalAnimationState::getInstance();
	if (animState.isAnimating) {
		ImGui::Text("Animating: %s (clip %d, time %.2f)", animState.entityName.c_str(), animState.clipIndex, animState.currentTime);
	}

	ImGui::End();
}

void ImGuiManager::drawSceneControlWindow(Scene& scene)
{
	ImGui::Begin("Scene Controls");

	// Camera section
	if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Camera position
		glm::vec3 camPos = scene.cam.pos;
		if (ImGui::DragFloat3("Position", glm::value_ptr(camPos), 0.1f)) {
			scene.cam.pos = camPos;
		}

		// Camera speed
		float& camSpeed = GlobalAnimationState::getInstance().camSpeed;
		ImGui::SliderFloat("Camera Speed", &camSpeed, 0.5f, 10.0f);

		// Camera direction (read-only)
		ImGui::Text("Direction: (%.2f, %.2f, %.2f)", scene.cam.front.x, scene.cam.front.y, scene.cam.front.z);

		// Camera view reset buttons
		if (ImGui::Button("Reset Camera")) {
			scene.setupCameraToViewScene(1.2f);
		}

		ImGui::SameLine();

		if (ImGui::Button("View Selected Entity")) {
			auto& entityManager = ImGuiManager::getInstance();
			if (entityManager.selectedEntityIndex >= 0 && entityManager.selectedEntityIndex < scene.ents.size()) {
				scene.setupCameraToViewEntity(scene.ents[entityManager.selectedEntityIndex].name);
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