#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec3.hpp>

class Scene;
class Shader;

class Renderer {
public:
	static Renderer& getInstance();

	void setupDefaultRenderer();
	void beginFrame(int w, int h, glm::vec3 const& clear);
	void drawScene(Scene const& scene);
	void endFrame();

	void cleanup();

	// Flag to control skeleton visualization
	bool showSkeletons{true};
	bool showModels{true};
	bool showWireFrame{false};

private:
	Renderer() = default;
	Renderer(Renderer const&) = delete;
	Renderer& operator=(Renderer const&) = delete;

	// Different shaders for different rendering techniques
	std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
	std::shared_ptr<Shader> mainShader_;
	std::shared_ptr<Shader> skyboxShader_;
	std::shared_ptr<Shader> skinnedShader_;
	std::shared_ptr<Shader> lineShader_;
	std::shared_ptr<Shader> lightPointShader_;

	// Helper methods for different rendering passes
	void drawModels_(Scene const& scene);
	void setupLighting_(Scene const& scene, std::shared_ptr<Shader> const& shader);

	// Renderer state
	int viewportWidth_{};
	int viewportHeight_{};

	// Current frame stats
	struct FrameStats {
		int drawCalls{};
		int visibleEntities{};
	};
	FrameStats currentFrameStats_;
};
