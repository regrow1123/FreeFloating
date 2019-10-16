#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <glslprogram.h>
#include <model3d.h>
#include <opencv2/opencv.hpp>

enum BufferNames {
	COUNTER_BUFFER = 0,
	LINKED_LIST_BUFFER
};

struct ListNode {
	GLfloat depth;
	GLuint next;
};

struct LinkedList {
	std::vector<ListNode> list;
	std::vector<GLuint> headPtr;

	int width, height;
	glm::mat4 model, view, projection;

	AABB aabb;
};

class FLLGenerator
{
private:
	GLSLProgram prog;
	GLuint fboHandle;
	GLuint buffers[2], clearBuf, headPtrTex;
	GLuint maxNodes, nodeSize;

	glm::mat4 model, view, projection;

	void Configure(Model3D*, LinkedList&);
	void SetupFBO(int, int);
	void SetupShaderStorage(int, int);
	void ClearBuffers(int, int);

public:
	FLLGenerator();
	~FLLGenerator() {}

	void Generate(Model3D*, LinkedList&);
};
