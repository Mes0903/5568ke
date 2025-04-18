#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8

#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>

#include "Texture.hpp"

Texture::Texture(std::string const& file, bool srgb) {
	int w, h, n;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(file.c_str(), &w, &h, &n, 0);
	if (!data) {
		std::cerr << "stb_image: " << file << " failed\n";
		return;
	}
	glGenTextures(1, &id_);
	glBindTexture(GL_TEXTURE_2D, id_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLenum fmt = n == 4 ? GL_RGBA : GL_RGB;
	GLenum ifmt = srgb ? (fmt == GL_RGBA ? GL_SRGB_ALPHA : GL_SRGB) : fmt;
	glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
}

void Texture::bind(int unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, id_);
}
