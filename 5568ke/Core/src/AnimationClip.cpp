#define GLM_ENABLE_EXPERIMENTAL

#include "AnimationClip.hpp"

#include <algorithm>
#include <iostream>

#include <tiny_gltf.h>

#include "AnimationChannel.hpp"
#include "AnimationTypes.hpp"
#include "Node.hpp"

AnimationClip::AnimationClip(std::string const& name) : clipName(name) {}

void AnimationClip::addChannel(tinygltf::Model const& model, tinygltf::Animation const& anim, tinygltf::AnimationChannel const& channel)
{
	std::cout << "[AnimationClip INFO] AnimationClip::addChannel - Creating channel" << std::endl;
	auto animChannel = std::make_shared<AnimationChannel>();

	try {
		std::cout << "[AnimationClip INFO] AnimationClip::addChannel - Loading channel data" << std::endl;
		animChannel->loadChannelData(model, anim, channel);
		std::cout << "[AnimationClip INFO] AnimationClip::addChannel - Channel data loaded successfully" << std::endl;
		channels_.push_back(animChannel);
	} catch (std::exception const& e) {
		std::cout << "[AnimationClip ERROR] AnimationClip::addChannel - Exception: " << e.what() << std::endl;
		throw;
	} catch (...) {
		std::cout << "[AnimationClip ERROR] AnimationClip::addChannel - Unknown exception" << std::endl;
		throw;
	}
}

void AnimationClip::setAnimationFrame(std::vector<std::shared_ptr<Node>> const& nodes, float time)
{
	if (nodes.empty() || channels_.empty()) {
		return;
	}

	std::cout << "[AnimationClip] Setting frame at time " << time << " for " << clipName << std::endl;

	// Apply all channels
	for (auto const& channel : channels_) {
		if (!channel)
			continue; // Skip invalid channels

		int targetNode = channel->targetNode;
		if (targetNode < 0 || targetNode >= nodes.size() || !nodes[targetNode])
			continue; // Skip invalid target nodes

		auto& node = nodes[targetNode];

		// Get the node's current transforms for debugging
		glm::vec3 oldTranslation = node->translation;
		glm::quat oldRotation = node->rotation;
		glm::vec3 oldScale = node->scale;

		// Apply the animation transforms
		switch (channel->targetPath) {
		case TargetPath::ROTATION:
			node->rotation = channel->getRotation(time);
			break;
		case TargetPath::TRANSLATION:
			node->translation = channel->getTranslation(time);
			break;
		case TargetPath::SCALE:
			node->scale = channel->getScaling(time);
			break;
		}

		// Debug output for significant changes
		if (glm::length(node->translation - oldTranslation) > 1.0f) {
			std::cout << "[AnimationClip WARNING] Large translation change in node " << targetNode << ": from " << glm::length(oldTranslation) << " to "
								<< glm::length(node->translation) << std::endl;
		}

		if (glm::length(node->scale - oldScale) > 1.0f) {
			std::cout << "[AnimationClip WARNING] Large scale change in node " << targetNode << ": from " << glm::length(oldScale) << " to "
								<< glm::length(node->scale) << std::endl;
		}
	}

	// Update all nodes' local matrices
	NodeUtil::updateNodeListLocalTRSMatrix(nodes);

	// Second pass to update global matrices starting from the root
	// First find the root node (usually node 0)
	std::shared_ptr<Node> rootNode;
	for (auto const& node : nodes) {
		if (node && node->nodeNum == 0) {
			rootNode = node;
			break;
		}
	}

	// If root node was found, update matrices starting from it
	if (rootNode) {
		NodeUtil::updateNodeTreeMatricesRecursive(rootNode, glm::mat4(1.0f));
	}
}

float AnimationClip::getDuration() const
{
	if (channels_.empty()) {
		return 0.0f;
	}

	float maxDuration = 0.0f;
	for (auto const& channel : channels_) {
		if (channel) { // Check if channel is valid
			maxDuration = std::max(maxDuration, channel->getMaxTime());
		}
	}
	return maxDuration;
}
