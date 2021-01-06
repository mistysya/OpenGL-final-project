#ifndef SHADER_H
#define SHADER_H

//#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
	unsigned int ID;
	Shader(const char* vertexPath, const char* fragmentPath, const char* tesselControlPath=NULL, const char* tesselEvauatePath=NULL)
	{
		std::string vertexCode;
		std::string fragmentCode;
		std::string tesselControlCode;
		std::string tesselEvauateCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		std::ifstream tcShaderFile;
		std::ifstream teShaderFile;
		// ensure throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		tcShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		teShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			vShaderFile.close();
			fShaderFile.close();
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
			// Load tessellation shader GLSL code
			if (tesselControlPath != NULL && tesselEvauatePath != NULL) {
				tcShaderFile.open(tesselControlPath);
				teShaderFile.open(tesselEvauatePath);
				std::stringstream tcShaderStream, teShaderStream;
				tcShaderStream << tcShaderFile.rdbuf();
				teShaderStream << teShaderFile.rdbuf();
				tcShaderFile.close();
				teShaderFile.close();
				tesselControlCode = tcShaderStream.str();
				tesselEvauateCode = teShaderStream.str();
			}
		}
		catch (std::ifstream::failure e)
		{
			std::cerr << e.code().message() << std::endl;
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		const char* vShaderCode = vertexCode.c_str(); // convert to C type
		const char * fShaderCode = fragmentCode.c_str();
		// compile
		unsigned int vertex, fragment, tesselControl, tesselEvauate;
		// vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX");
		// fragment Shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");
		// If there is tessellation GLSL code
		if (tesselControlPath != NULL && tesselEvauatePath != NULL) {
			// convert GLSL code to C type
			const char* tcShaderCode = tesselControlCode.c_str();
			const char* teShaderCode = tesselEvauateCode.c_str();
			// tessellation control Shader
			tesselControl = glCreateShader(GL_TESS_CONTROL_SHADER);
			glShaderSource(tesselControl, 1, &tcShaderCode, NULL);
			glCompileShader(tesselControl);
			checkCompileErrors(tesselControl, "TESSELLATION_CONTROL");
			// tessellation evaluation Shader
			tesselEvauate = glCreateShader(GL_TESS_EVALUATION_SHADER);
			glShaderSource(tesselEvauate, 1, &teShaderCode, NULL);
			glCompileShader(tesselEvauate);
			checkCompileErrors(tesselEvauate, "TESSELLATION_EVALUATION");
		}
		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		// If there is tessellation GLSL code
		if (tesselControlPath != NULL && tesselEvauatePath != NULL) {
			glAttachShader(ID, tesselControl);
			glAttachShader(ID, tesselEvauate);
		}
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");

		glDeleteShader(vertex);
		glDeleteShader(fragment);
		// If there is tessellation GLSL code
		if (tesselControlPath != NULL && tesselEvauatePath != NULL) {
			glDeleteShader(tesselControl);
			glDeleteShader(tesselEvauate);
		}
	}

	void use()
	{
		glUseProgram(ID);
	}

	void setBool(const std::string &name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	void setInt(const std::string &name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	void setFloat(const std::string &name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	void setVec2(const std::string &name, const glm::vec2 &value) const
	{
		glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const std::string &name, const glm::vec3 &value) const
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setMat4(const std::string &name, const glm::mat4 &mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

private:
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success); // COMPILE
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog); // get log
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success); // LINK
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog); // get log
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};
#endif