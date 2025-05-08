#define GLM_ENABLE_EXPERIMENTAL

#include "Animation.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <iostream>
#include "Model.hpp"

// Bone methods for keyframe interpolation
int Bone::getPositionIndex(float animationTime) const
{
	if (positions.empty()) {
		return -1;
	}

	// Handle edge cases
	if (positions.size() == 1 || animationTime <= positions[0].timeStamp) {
		return 0;
	}

	if (animationTime >= positions.back().timeStamp) {
		return static_cast<int>(positions.size() - 1);
	}

	// Binary search for the correct position
	int left = 0;
	int right = static_cast<int>(positions.size() - 1);
	while (left <= right) {
		int mid = left + (right - left) / 2;
		if (positions[mid].timeStamp == animationTime) {
			return mid;
		}
		if (positions[mid].timeStamp < animationTime) {
			left = mid + 1;
		}
		else {
			right = mid - 1;
		}
	}

	// At this point, left > right and animationTime is between [right] and [left]
	return right;
}

int Bone::getRotationIndex(float animationTime) const
{
	if (rotations.empty()) {
		return -1;
	}

	if (rotations.size() == 1 || animationTime <= rotations[0].timeStamp) {
		return 0;
	}

	if (animationTime >= rotations.back().timeStamp) {
		return static_cast<int>(rotations.size() - 1);
	}

	int left = 0;
	int right = static_cast<int>(rotations.size() - 1);
	while (left <= right) {
		int mid = left + (right - left) / 2;
		if (rotations[mid].timeStamp == animationTime) {
			return mid;
		}
		if (rotations[mid].timeStamp < animationTime) {
			left = mid + 1;
		}
		else {
			right = mid - 1;
		}
	}

	return right;
}

int Bone::getScaleIndex(float animationTime) const
{
	if (scales.empty()) {
		return -1;
	}

	if (scales.size() == 1 || animationTime <= scales[0].timeStamp) {
		return 0;
	}

	if (animationTime >= scales.back().timeStamp) {
		return static_cast<int>(scales.size() - 1);
	}

	int left = 0;
	int right = static_cast<int>(scales.size() - 1);
	while (left <= right) {
		int mid = left + (right - left) / 2;
		if (scales[mid].timeStamp == animationTime) {
			return mid;
		}
		if (scales[mid].timeStamp < animationTime) {
			left = mid + 1;
		}
		else {
			right = mid - 1;
		}
	}

	return right;
}

glm::vec3 Bone::interpolatePosition(float animationTime) const
{
	if (positions.empty()) {
		return glm::vec3(0.0f);
	}

	if (positions.size() == 1) {
		return positions[0].position;
	}

	int p0Index = getPositionIndex(animationTime);
	int p1Index = p0Index + 1;

	// Handle edge cases
	if (p0Index == -1) {
		return glm::vec3(0.0f);
	}

	if (p0Index == static_cast<int>(positions.size() - 1)) {
		return positions[p0Index].position;
	}

	float scaleFactor = (animationTime - positions[p0Index].timeStamp) / (positions[p1Index].timeStamp - positions[p0Index].timeStamp);

	// Linear interpolation
	return glm::mix(positions[p0Index].position, positions[p1Index].position, scaleFactor);
}

glm::quat Bone::interpolateRotation(float animationTime) const
{
	if (rotations.empty()) {
		return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	}

	if (rotations.size() == 1) {
		return rotations[0].orientation;
	}

	int r0Index = getRotationIndex(animationTime);
	int r1Index = r0Index + 1;

	// Handle edge cases
	if (r0Index == -1) {
		return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	}

	if (r0Index == static_cast<int>(rotations.size() - 1)) {
		return rotations[r0Index].orientation;
	}

	float scaleFactor = (animationTime - rotations[r0Index].timeStamp) / (rotations[r1Index].timeStamp - rotations[r0Index].timeStamp);

	// Spherical linear interpolation for quaternions
	return glm::slerp(rotations[r0Index].orientation, rotations[r1Index].orientation, scaleFactor);
}

glm::vec3 Bone::interpolateScale(float animationTime) const
{
	if (scales.empty()) {
		return glm::vec3(1.0f);
	}

	if (scales.size() == 1) {
		return scales[0].scale;
	}

	int s0Index = getScaleIndex(animationTime);
	int s1Index = s0Index + 1;

	// Handle edge cases
	if (s0Index == -1) {
		return glm::vec3(1.0f);
	}

	if (s0Index == static_cast<int>(scales.size() - 1)) {
		return scales[s0Index].scale;
	}

	float scaleFactor = (animationTime - scales[s0Index].timeStamp) / (scales[s1Index].timeStamp - scales[s0Index].timeStamp);

	// Linear interpolation
	return glm::mix(scales[s0Index].scale, scales[s1Index].scale, scaleFactor);
}

glm::mat4 Bone::calculateLocalTransform(float animationTime)
{
	glm::vec3 position = interpolatePosition(animationTime);
	glm::quat rotation = interpolateRotation(animationTime);
	glm::vec3 scale = interpolateScale(animationTime);

	// Create transformation matrix from components
	glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
	glm::mat4 rotation_mat = glm::toMat4(rotation);
	glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), scale);

	// Combine transforms: T * R * S
	localTransform = translation * rotation_mat * scale_mat;
	return localTransform;
}

// AnimationPlayer implementation
void AnimationPlayer::initialize(Model* model)
{
	model_ = model;
	currentAnimation_ = nullptr;
	currentAnimationIndex_ = -1;
	currentTime_ = 0.0f;
	playing_ = false;

	// Set default animation if available
	if (model_ && !model_->animations.empty()) {
		setAnimation(0);
	}
}

void AnimationPlayer::update(float dt)
{
	if (!model_ || !currentAnimation_ || !playing_) {
		return;
	}

	// Update animation time
	currentTime_ += dt * playbackSpeed_ * currentAnimation_->ticksPerSecond;

	// Handle looping
	if (currentTime_ >= currentAnimation_->duration) {
		if (looping_) {
			currentTime_ = fmod(currentTime_, currentAnimation_->duration);
		}
		else {
			currentTime_ = currentAnimation_->duration;
			playing_ = false;
		}
	}

	// Update bone transformations
	if (currentAnimation_->rootNode) {
		updateBoneTransforms(currentTime_, currentAnimation_->rootNode, glm::mat4(1.0f), model_->skeleton);
	}
}

bool AnimationPlayer::setAnimation(int index)
{
	if (!model_ || index < 0 || index >= static_cast<int>(model_->animations.size())) {
		return false;
	}

	currentAnimation_ = &model_->animations[index];
	currentAnimationIndex_ = index;
	currentTime_ = 0.0f;
	return true;
}

bool AnimationPlayer::setAnimation(std::string const& name)
{
	if (!model_) {
		return false;
	}

	for (size_t i = 0; i < model_->animations.size(); i++) {
		if (model_->animations[i].name == name) {
			return setAnimation(static_cast<int>(i));
		}
	}

	return false;
}

void AnimationPlayer::play()
{
	if (currentAnimation_) {
		playing_ = true;

		// If we're at the end and not looping, restart
		if (currentTime_ >= currentAnimation_->duration && !looping_) {
			currentTime_ = 0.0f;
		}
	}
}

void AnimationPlayer::pause() { playing_ = false; }

void AnimationPlayer::stop()
{
	playing_ = false;
	currentTime_ = 0.0f;

	// Reset bone transformations to bind pose
	if (model_) {
		model_->skeleton.finalBoneMatrices.assign(Skeleton::MAX_BONES, glm::mat4(1.0f));
	}
}

std::string AnimationPlayer::getCurrentAnimationName() const
{
	if (currentAnimation_) {
		return currentAnimation_->name;
	}
	return "";
}

size_t AnimationPlayer::getAnimationCount() const
{
	if (model_) {
		return model_->animations.size();
	}
	return 0;
}

std::string AnimationPlayer::getAnimationName(int index) const
{
	if (model_ && index >= 0 && index < static_cast<int>(model_->animations.size())) {
		return model_->animations[index].name;
	}
	return "";
}

float AnimationPlayer::getCurrentDuration() const
{
	if (currentAnimation_) {
		return currentAnimation_->duration / currentAnimation_->ticksPerSecond;
	}
	return 0.0f;
}

float AnimationPlayer::getProgress() const
{
	if (currentAnimation_ && currentAnimation_->duration > 0.0f) {
		return currentTime_ / currentAnimation_->duration;
	}
	return 0.0f;
}

void AnimationPlayer::setProgress(float progress)
{
	if (currentAnimation_) {
		progress = std::clamp(progress, 0.0f, 1.0f);
		currentTime_ = progress * currentAnimation_->duration;

		// Update bone transformations immediately
		if (currentAnimation_->rootNode) {
			updateBoneTransforms(currentTime_, currentAnimation_->rootNode, glm::mat4(1.0f), model_->skeleton);
		}
	}
}

void AnimationPlayer::updateBoneTransforms(float animationTime, SkeletonNode* node, glm::mat4 const& parentTransform, Skeleton& skeleton)
{
	if (!node) {
		return;
	}

	// Get node transformation
	glm::mat4 nodeTransform = node->transformation;

	// If node corresponds to a bone, calculate animation transform
	if (node->boneIndex >= 0) {
		Bone& bone = skeleton.bones[node->boneIndex];
		nodeTransform = bone.calculateLocalTransform(animationTime);
	}

	// Calculate global transformation
	glm::mat4 globalTransform = parentTransform * nodeTransform;

	// If node corresponds to a bone, update final bone matrix
	if (node->boneIndex >= 0) {
		Bone& bone = skeleton.bones[node->boneIndex];
		skeleton.finalBoneMatrices[node->boneIndex] = globalTransform * bone.offsetMatrix;
	}

	// Process children
	for (auto child : node->children) {
		updateBoneTransforms(animationTime, child, globalTransform, skeleton);
	}
}