#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <glslprogram.h>
#include <aabb.h>

class Triangles3D
{
private:
	GLuint vao;
	GLuint verticesN;

	std::vector<GLuint> buffers;

public:
	AABB aabb;

private:
	void DeleteBuffers();

public:
	Triangles3D();
	~Triangles3D();

	void InitBuffers(std::vector<GLfloat>& points);

	void Render();
};

class Rasterizer
{
private:
	Triangles3D* target;

	GLSLProgram prog;
	GLuint fboHandle;

	int width, height;
	glm::mat4 model, view, projection;

	float resolution;

private:
	void Configure();
	void SetupFBO();

public:
	Rasterizer() {}
	Rasterizer(Triangles3D*, float);
	~Rasterizer() {}

	void Sample(std::vector<glm::vec3>&);
};
