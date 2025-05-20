#include "Shader.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

namespace {
unsigned int compileStage(std::string const& src, GLenum type)
{
	unsigned int id = glCreateShader(type);
	char const* s = src.c_str();
	glShaderSource(id, 1, &s, nullptr);
	glCompileShader(id);
	int ok;
	glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetShaderInfoLog(id, 1024, nullptr, log);
		std::cout << "[Shader ERROR] Shader error:\n" << log << '\n';
	}
	return id;
}

std::string loadFile(std::string const& path)
{
	std::ifstream f(path);
	if (!f) {
		std::cout << "[Shader ERROR] cannot open " << path << '\n';
		return {};
	}
	std::stringstream ss;
	ss << f.rdbuf();
	return ss.str();
}
} // namespace

void Shader::resetShaderPath(std::string const& v, std::string const& f)
{
	vsPath_ = v;
	fsPath_ = f;
	reload();
}

void Shader::reload()
{
	if (vsPath_.empty() || fsPath_.empty())
		return;

	if (program_)
		glDeleteProgram(program_);

	unsigned int vs = compileStage(loadFile(vsPath_), GL_VERTEX_SHADER);
	unsigned int fs = compileStage(loadFile(fsPath_), GL_FRAGMENT_SHADER);

	program_ = glCreateProgram();
	glAttachShader(program_, vs);
	glAttachShader(program_, fs);
	glLinkProgram(program_);

	GLint success;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(vs, 512, NULL, infoLog);
		std::cout << "[Shader ERROR] Vertex shader compilation failed: " << infoLog << std::endl;
	}

	glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(fs, 512, NULL, infoLog);
		std::cout << "[Shader ERROR] Fragment shader compilation failed: " << infoLog << std::endl;
	}

	// Check program linking status
	glGetProgramiv(program_, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program_, 512, NULL, infoLog);
		std::cout << "[Shader ERROR] Shader program linking failed: " << infoLog << std::endl;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);
}

void Shader::bind() const { glUseProgram(program_); }

void Shader::unbind() const { glUseProgram(0); }

void Shader::sendMat4(char const* name, glm::mat4 const& mat) const
{
	int loc = glGetUniformLocation(program_, name);
	if (loc != -1)
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
	else
		std::cout << "[Shader] Warning: uniform '" << name << "' not found.\n";
}

void Shader::sendVec3(char const* name, glm::vec3 const& vec) const
{
	int loc = glGetUniformLocation(program_, name);
	if (loc != -1)
		glUniform3fv(loc, 1, glm::value_ptr(vec));
	else
		std::cout << "[Shader] Warning: uniform '" << name << "' not found.\n";
}

void Shader::sendFloat(char const* name, float value) const
{
	int loc = glGetUniformLocation(program_, name);
	if (loc != -1)
		glUniform1f(loc, value);
	else
		std::cout << "[Shader] Warning: uniform '" << name << "' not found.\n";
}

void Shader::sendInt(char const* name, int value) const
{
	int loc = glGetUniformLocation(program_, name);
	if (loc != -1)
		glUniform1i(loc, value);
	else
		std::cout << "[Shader] Warning: uniform '" << name << "' not found.\n";
}
