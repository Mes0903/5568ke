#include <glad/glad.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Shader.hpp"

namespace {
unsigned compileStage(std::string const& src, GLenum type) {
	unsigned id = glCreateShader(type);
	char const* s = src.c_str();
	glShaderSource(id, 1, &s, nullptr);
	glCompileShader(id);
	int ok;
	glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetShaderInfoLog(id, 1024, nullptr, log);
		std::cerr << "Shader error:\n" << log << '\n';
	}
	return id;
}

std::string loadFile(std::string const& p) {
	std::ifstream f(p, std::ios::binary);
	if (!f) {
		std::cerr << "[Shader] cannot open " << p << '\n';
		return {};
	}
	std::stringstream ss;
	ss << f.rdbuf();
	return ss.str();
}
} // namespace

void Shader::resetShader(std::string const& v, std::string const& f) {
	vsPath_ = v;
	fsPath_ = f;
	reload();
}

void Shader::reload() {
	if (vsPath_.empty() || fsPath_.empty())
		return;

	if (program_)
		glDeleteProgram(program_);
	unsigned vs = compileStage(loadFile(vsPath_), GL_VERTEX_SHADER);
	unsigned fs = compileStage(loadFile(fsPath_), GL_FRAGMENT_SHADER);
	program_ = glCreateProgram();
	glAttachShader(program_, vs);
	glAttachShader(program_, fs);
	glLinkProgram(program_);
	int ok;
	glGetProgramiv(program_, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetProgramInfoLog(program_, 1024, nullptr, log);
		std::cerr << "Link error:\n" << log << '\n';
	}
	glDeleteShader(vs);
	glDeleteShader(fs);
}

void Shader::bind() const { glUseProgram(program_); }
void Shader::unbind() const { glUseProgram(0); }
void Shader::setMat4(char const* n, float const* m) const { glUniformMatrix4fv(glGetUniformLocation(program_, n), 1, GL_FALSE, m); }
void Shader::setVec3(char const* n, float const* v) const { glUniform3fv(glGetUniformLocation(program_, n), 1, v); }
