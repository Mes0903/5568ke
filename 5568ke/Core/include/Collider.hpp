#pragma once

#include <cstdint>
#include <memory>

#include "BoundingBox.hpp"
#include "GameObject.hpp"

class AABBCollider {
public:
	explicit AABBCollider(std::shared_ptr<GameObject> owner) : owner_(owner) {}

	std::shared_ptr<GameObject> owner() const noexcept { return owner_; }
	BoundingBox const& bounds() const noexcept { return owner_->worldBBox; }

private:
	std::shared_ptr<GameObject> owner_;
};
