#pragma once

#include <string>

class Texture {
public:
	Texture() = default;
	Texture(std::string const& file, bool srgb = true);
	void bind(int unit = 0) const;

private:
	unsigned id_ = 0;
};
