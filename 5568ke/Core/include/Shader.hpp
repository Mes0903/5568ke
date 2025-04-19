#pragma once

#include "include_5568ke.hpp"

#include <glm/glm.hpp>
#include <string>

class Shader {
public:
	void resetShader(std::string const& vertPath, std::string const& fragPath);
	void reload();
	void bind() const;
	void unbind() const;

	void setMat4(char const* name, glm::mat4 const& mat) const;
	void setVec3(char const* name, glm::vec3 const& vec) const;
	void setFloat(char const* name, float value) const;
	void setInt(char const* name, int value) const;

private:
	unsigned program_ = 0;
	std::string vsPath_, fsPath_;
};