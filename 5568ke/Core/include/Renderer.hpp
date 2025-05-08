#pragma once

#include <glm/vec3.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Scene.hpp"
#include "Shader.hpp"

class Renderer {
public:
	Renderer() = default;
	~Renderer();

	void setupDefaultRenderer();
	void beginFrame(int w, int h, glm::vec3 const& clear);
	void drawScene(Scene const& scene);
	void endFrame();

private:
	// Different shaders for different rendering techniques
	std::unordered_map<std::string, std::unique_ptr<Shader>> shaders_;
	Shader* mainShader_ = nullptr;
	Shader* animatedShader_ = nullptr; // For skinned mesh animation
	Shader* skyboxShader_ = nullptr;

	// Helper methods for different rendering passes
	void drawModels_(Scene const& scene);
	void setupLighting_(Scene const& scene, Shader* shader);

	// Renderer state
	int viewportWidth_ = 0;
	int viewportHeight_ = 0;

	// Current frame stats
	struct FrameStats {
		int drawCalls = 0;
		int visibleEntities = 0;
	};
	FrameStats currentFrameStats_;
};