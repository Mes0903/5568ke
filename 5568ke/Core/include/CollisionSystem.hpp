#pragma once

#include <functional>
#include <vector>

#include "BoundingBox.hpp"
#include "Collider.hpp"
#include "GameObject.hpp"

class CollisionSystem {
public:
	static CollisionSystem& getInstance()
	{
		static CollisionSystem instance;
		return instance;
	}

	using Callback = std::function<void(std::shared_ptr<GameObject>, std::shared_ptr<GameObject>)>;

	void add(std::shared_ptr<AABBCollider> c) { colliders_.push_back(c); }
	void remove(std::shared_ptr<AABBCollider> c);

	// Call once per frame AFTER all GameObject transforms have been updated
	void update()
	{
		std::size_t n = colliders_.size();
		for (std::size_t i = 0; i < n; ++i) {
			for (std::size_t j = i + 1; j < n; ++j) {
				auto a = colliders_[i];
				auto b = colliders_[j];
				if (BBoxUtil::isIntersectBBox(a->bounds(), b->bounds())) {
					onCollision_(a->owner(), b->owner());
				}
			}
		}
	}

private:
	CollisionSystem() = default;
	~CollisionSystem() = default;

	std::vector<std::shared_ptr<AABBCollider>> colliders_;
	void onCollision_(std::shared_ptr<GameObject>, std::shared_ptr<GameObject>);
};

inline void CollisionSystem::remove(std::shared_ptr<AABBCollider> c)
{
	colliders_.erase(std::remove(colliders_.begin(), colliders_.end(), c), colliders_.end());
}

inline void CollisionSystem::onCollision_(std::shared_ptr<GameObject> a, std::shared_ptr<GameObject> b)
{
	a->onCollisionEnter(b);
	b->onCollisionEnter(a);
}