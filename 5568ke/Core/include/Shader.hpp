#pragma once

#include <string>

class Shader {
public:
	void resetShader(std::string const& v, std::string const& f);
	void reload();
	void bind() const;
	void unbind() const;
	void setMat4(char const* name, float const* m) const;
	void setVec3(char const* name, float const* v) const;

private:
	unsigned program_ = 0;
	std::string vsPath_, fsPath_;
};
