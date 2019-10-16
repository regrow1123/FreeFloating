#pragma once

#include <glad/glad.h>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

namespace GLSLShader {
	enum GLSLShaderType {
		VERTEX = GL_VERTEX_SHADER,
		FRAGMENT = GL_FRAGMENT_SHADER
	};
}

class GLSLProgram
{
private:
	GLuint handle;

	std::map<std::string, int> uniformLocations;

public:
	GLSLProgram() : handle(0) {}
	~GLSLProgram() {
		if (handle == 0)
			return;

		GLint numShaders = 0;
		glGetProgramiv(handle, GL_ATTACHED_SHADERS, &numShaders);

		GLuint* shaderNames = new GLuint[numShaders];
		glGetAttachedShaders(handle, numShaders, 0, shaderNames);

		for (int i = 0; i < numShaders; i++)
			glDeleteShader(shaderNames[i]);

		glDeleteProgram(handle);
		delete[] shaderNames;
	}

	void CompileShader(std::string path, GLSLShader::GLSLShaderType type) {
		if (handle == 0) {
			handle = glCreateProgram();
			if (handle == 0)
				exit(EXIT_FAILURE);
		}

		std::ifstream inFile(path, std::ios::in);
		if (!inFile) {
			std::cout << "Unable to open : " << path << std::endl;
			exit(EXIT_FAILURE);
		}

		std::string whole, line;
		while (!inFile.eof()) {
			std::getline(inFile, line);
			whole = whole + line + '\n';
		}

		GLuint shaderHandle = glCreateShader(type);

		const char* c_code = whole.c_str();
		glShaderSource(shaderHandle, 1, &c_code, 0);
		glCompileShader(shaderHandle);
		glAttachShader(handle, shaderHandle);
	}
	void Link() {
		glLinkProgram(handle);
	}
	void Use() {
		glUseProgram(handle);
	}

	void SetUniform(std::string name, float val) {
		GLint loc = GetUniformLocation(name);
		glUniform1f(loc, val);
	}
	void SetUniform(std::string name, GLuint val) {
		GLint loc = GetUniformLocation(name);
		glUniform1ui(loc, val);
	}
	void SetUniform(std::string name, const glm::mat3& m) {
		GLint loc = GetUniformLocation(name);
		glUniformMatrix3fv(loc, 1, GL_FALSE, &m[0][0]);
	}
	void SetUniform(std::string name, const glm::mat4& m) {
		GLint loc = GetUniformLocation(name);
		glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
	}

private:
	GLSLProgram(const GLSLProgram&) {}
	GLSLProgram& operator=(const GLSLProgram&) {
		return *this;
	}

	GLint GetUniformLocation(std::string name) {
		auto pos = uniformLocations.find(name);
		if (pos == uniformLocations.end())
			uniformLocations[name] = glGetUniformLocation(handle, name.c_str());

		return uniformLocations[name];
	}
};
