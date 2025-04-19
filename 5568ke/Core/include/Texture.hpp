#pragma once

#include "include_5568ke.hpp"

#include <string>

enum class TextureType { Diffuse, Specular, Normal, Roughness };

class Texture {
public:
	GLuint id = 0;
	TextureType type;
	std::string path;

	void bind(unsigned slot) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, id);
	}
};
