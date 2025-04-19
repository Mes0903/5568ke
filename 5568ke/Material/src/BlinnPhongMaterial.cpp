#include "include_5568ke.hpp"

#include "BlinnPhongMaterial.hpp"

void BlinnPhongMaterial::bind(Shader& shader) const
{
	// Your shader doesn't have material.albedo or material.shininess uniforms
	// It directly uses the texture colors instead

	// Bind diffuse/base texture to texture unit 0
	if (diffuseMap) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap->id);
		shader.setInt("tex0", 0);
	}

	// Bind overlay texture to texture unit 1
	if (overlayMap) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, overlayMap->id);
		shader.setInt("tex1", 1);
	}

	// If no textures are available, we could potentially add a fallback
	// by modifying the shader to use a uniform color, but that would require
	// shader changes. For now, we'll just make sure at least one texture is bound.

	if (!diffuseMap && !overlayMap) {
		// Create a small white texture as fallback
		static GLuint defaultTexture = 0;
		if (defaultTexture == 0) {
			// Create a default white texture
			unsigned char whitePixel[4] = {255, 255, 255, 255};
			glGenTextures(1, &defaultTexture);
			glBindTexture(GL_TEXTURE_2D, defaultTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, defaultTexture);
		shader.setInt("tex0", 0);
	}
}