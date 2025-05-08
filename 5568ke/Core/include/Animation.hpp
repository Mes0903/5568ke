#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
class Model;

// Bone influence for vertex skinning
struct VertexBoneData {
	int boneIds[4] = {-1, -1, -1, -1};
	float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	void addBoneData(int boneId, float weight)
	{
		// Find first available slot
		for (int i = 0; i < 4; i++) {
			if (weights[i] == 0.0f) {
				boneIds[i] = boneId;
				weights[i] = weight;
				return;
			}
		}

		// If we get here, we need to replace the smallest weight
		int minIndex = 0;
		for (int i = 1; i < 4; i++) {
			if (weights[i] < weights[minIndex]) {
				minIndex = i;
			}
		}

		// Replace if new weight is larger than smallest
		if (weight > weights[minIndex]) {
			boneIds[minIndex] = boneId;
			weights[minIndex] = weight;
		}
	}

	void normalize()
	{
		float sum = weights[0] + weights[1] + weights[2] + weights[3];
		if (sum > 0.0f) {
			float invSum = 1.0f / sum;
			weights[0] *= invSum;
			weights[1] *= invSum;
			weights[2] *= invSum;
			weights[3] *= invSum;
		}
	}
};

// Key frame for a bone animation
struct KeyPosition {
	glm::vec3 position;
	float timeStamp;
};

struct KeyRotation {
	glm::quat orientation;
	float timeStamp;
};

struct KeyScale {
	glm::vec3 scale;
	float timeStamp;
};

// Bone structure
struct Bone {
	std::string name;
	int id;
	glm::mat4 offsetMatrix;
	glm::mat4 localTransform;

	std::vector<KeyPosition> positions;
	std::vector<KeyRotation> rotations;
	std::vector<KeyScale> scales;

	// Find appropriate position keyframe at given animation time
	int getPositionIndex(float animationTime) const;

	// Find appropriate rotation keyframe at given animation time
	int getRotationIndex(float animationTime) const;

	// Find appropriate scale keyframe at given animation time
	int getScaleIndex(float animationTime) const;

	// Interpolate between position keyframes
	glm::vec3 interpolatePosition(float animationTime) const;

	// Interpolate between rotation keyframes
	glm::quat interpolateRotation(float animationTime) const;

	// Interpolate between scale keyframes
	glm::vec3 interpolateScale(float animationTime) const;

	// Calculate the local transform matrix at the given animation time
	glm::mat4 calculateLocalTransform(float animationTime);
};

// Skeletal structure
struct Skeleton {
	std::vector<Bone> bones;
	std::unordered_map<std::string, int> boneNameToIndex;
	std::vector<glm::mat4> finalBoneMatrices;
	int boneCount = 0;

	// Max bones supported by shader
	static int const MAX_BONES = 100;

	// Initialize bone matrices
	void initBoneMatrices() { finalBoneMatrices.resize(MAX_BONES, glm::mat4(1.0f)); }

	// Get bone index by name, returns -1 if not found
	int getBoneIndex(std::string const& name) const
	{
		auto it = boneNameToIndex.find(name);
		if (it != boneNameToIndex.end()) {
			return it->second;
		}
		return -1;
	}
};

// Node in the skeleton hierarchy
struct SkeletonNode {
	std::string name;
	int boneIndex = -1; // -1 if this node doesn't correspond to a bone
	glm::mat4 transformation;
	std::vector<SkeletonNode*> children;

	~SkeletonNode()
	{
		for (auto child : children) {
			delete child;
		}
	}
};

// Single animation clip
struct Animation {
	std::string name;
	float duration = 0.0f;
	float ticksPerSecond = 25.0f;
	SkeletonNode* rootNode = nullptr;

	// Map from node name to node for quick access
	std::unordered_map<std::string, SkeletonNode*> nodeMap;

	~Animation() { delete rootNode; }
};

// Animation player to control playback
class AnimationPlayer {
public:
	AnimationPlayer() = default;
	~AnimationPlayer() = default;

	// Initialize with a model that has animations
	void initialize(Model* model);

	// Update animation state
	void update(float dt);

	// Set current animation by index
	bool setAnimation(int index);

	// Set current animation by name
	bool setAnimation(std::string const& name);

	// Play, pause, stop controls
	void play();
	void pause();
	void stop();

	// Set playback speed (1.0 = normal speed)
	void setSpeed(float speed) { playbackSpeed_ = speed; }

	// Set looping
	void setLooping(bool loop) { looping_ = loop; }

	// Get current animation name
	std::string getCurrentAnimationName() const;

	// Get number of animations
	size_t getAnimationCount() const;

	// Get animation name by index
	std::string getAnimationName(int index) const;

	// Get current animation duration in seconds
	float getCurrentDuration() const;

	// Get current animation progress (0.0 - 1.0)
	float getProgress() const;

	// Set animation progress (0.0 - 1.0)
	void setProgress(float progress);

	// Is animation playing
	bool isPlaying() const { return playing_; }

	// Update bone transforms in the skeleton
	void updateBoneTransforms(float animationTime, SkeletonNode* node, glm::mat4 const& parentTransform, Skeleton& skeleton);

private:
	Model* model_ = nullptr;
	Animation* currentAnimation_ = nullptr;
	int currentAnimationIndex_ = -1;

	bool playing_ = false;
	bool looping_ = true;
	float currentTime_ = 0.0f;
	float playbackSpeed_ = 1.0f;
};