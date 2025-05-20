#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec3.hpp>

#include "BoundingBoxVisualizer.hpp"
#include "LightVisualizer.hpp"
#include "SkeletonVisualizer.hpp"
#include "SkyboxVisualizer.hpp"

class Scene;
class Shader;

class Renderer {
public:
	static Renderer& getInstance();
	void init();
	void cleanup();

	void beginFrame(int w, int h, glm::vec3 const& clear);
	void endFrame();
	void drawScene(Scene const& scene);

	// Flag to control main visualization
	bool showModels{true};
	bool showWireFrame{false};

	// Flag to control call visualizer
	bool showSkybox{true};
	bool showSkeletons{true};
	bool showLightPoint{true};
	bool showBBox{true};

	SkeletonVisualizer& skeletonVisualizerRef = SkeletonVisualizer::getInstance();
	LightPointVisualizer& lightVisualizerRef = LightPointVisualizer::getInstance();
	BoundingBoxVisualizer& boundingBoxVisualizerRef = BoundingBoxVisualizer::getInstance();
	SkyboxVisualizer& skyboxVisualizerRef = SkyboxVisualizer::getInstance();

private:
	Renderer() = default;
	Renderer(Renderer const&) = delete;
	Renderer& operator=(Renderer const&) = delete;

	// Different shaders for different rendering techniques
	std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
	std::shared_ptr<Shader> mainShader_;
	std::shared_ptr<Shader> skinnedShader_;

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
