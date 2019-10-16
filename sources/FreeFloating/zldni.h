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

class zLDNIGenerator
{
private:
	GLSLProgram prog;
	GLuint fboHandle;
	GLuint buffers[2], clearBuf, headPtrTex;
	GLuint maxNodes, nodeSize;

	glm::mat4 model, view, projection;
	int width, height;

	Model3D* target;

	struct ListNode {
		GLfloat depth;
		GLuint next;
	};
	std::vector<ListNode> list;
	std::vector<GLuint> headPtr;

	void Configure();
	void SetupFBO();
	void SetupShaderStorage();
	void ClearBuffers();
	void Run();

public:
	zLDNIGenerator(Model3D*);
	~zLDNIGenerator() {}

	void GetImageSize(int&, int&);
	void GetSortedList(std::vector<glm::vec3>& nodes, int row, int col);
};
