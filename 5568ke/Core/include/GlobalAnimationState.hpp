#pragma once

#include <string>

class GlobalAnimationState {
public:
	static GlobalAnimationState& getInstance()
	{
		static GlobalAnimationState instance;
		return instance;
	}

	// Animation state
	bool isAnimating{};
	std::string entityName;
	int clipIndex{};
	float currentTime{};
	float camSpeed{3.0f};

	// Character movement state
	bool characterMoveMode{false};
	bool wasMoving{false};
	float followDistance{3.0f};
	float followHeight{1.0f};

	// Simple physics state
	float verticalVelocity{0.0f};
	bool onGround{true};
	float gravity{9.8f};
	float jumpSpeed{5.0f};

	// Animation control methods
	void play(int clip, float initialTime = 0.0f)
	{
		clipIndex = clip;
		currentTime = initialTime;
		isAnimating = true;
	}

	void stop()
	{
		isAnimating = false;
		currentTime = 0.0f;
	}

	void pause() { isAnimating = false; }
	void resume() { isAnimating = true; }
	void setAnimateSpeed(float newSpeed) { animateSpeed_ = newSpeed > 0.1f ? newSpeed : 0.1f; }
	float getAnimateSpeed() const { return animateSpeed_; }

private:
	GlobalAnimationState() = default;
	float animateSpeed_{1.0f};
};
