#pragma once

#include <glm/glm.hpp>
#include <string>

#include "include_5568ke.hpp"

class Shader {
public:
	void resetShaderPath(std::string const& vertPath, std::string const& fragPath);
	void reload();
	void bind() const;
	void unbind() const;

	void sendMat4(char const* name, glm::mat4 const& mat) const;
	void sendVec3(char const* name, glm::vec3 const& vec) const;
	void sendFloat(char const* name, float value) const;
	void sendInt(char const* name, int value) const;
	void sendBool(char const* name, bool value) const { glUniform1i(glGetUniformLocation(program_, name), static_cast<int>(value)); }

private:
	unsigned int program_{};
	std::string vsPath_;
	std::string fsPath_;
};
