#include "fllgenerator.h"

#include <GLFW/glfw3.h>
#include <params.h>
#include <glm/gtc/matrix_transform.hpp>
using glm::vec3;
using glm::mat4;
using cv::Mat;

FLLGenerator::FLLGenerator()
{
	prog.CompileShader("./shader/fragmentlist.vs", GLSLShader::VERTEX);
	prog.CompileShader("./shader/fragmentlist.fs", GLSLShader::FRAGMENT);
	prog.Link();
}

void FLLGenerator::ClearBuffers(int width, int height)
{
	GLuint zero = 0;
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, buffers[COUNTER_BUFFER]);
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, clearBuf);
	glBindTexture(GL_TEXTURE_2D, headPtrTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED_INTEGER,
		GL_UNSIGNED_INT, 0);
}

void FLLGenerator::Configure(Model3D* model3D, LinkedList& linkedList)
{
	linkedList.aabb = model3D->aabb;
	vec3 center = linkedList.aabb.GetCenter();
	vec3 size = linkedList.aabb.GetSize();

	Params& params = Params::GetInstance();
	float margin = params.pixelWidth * 10;
	size += vec3(margin);
	float halfX = size.x / 2.0f;
	float halfY = size.y / 2.0f;
	float halfZ = size.z / 2.0f;

	linkedList.model = glm::translate(mat4(1.0f), -center);
	linkedList.view = glm::lookAt(
		vec3(0.0f, 0.0f, halfZ),
		vec3(0.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f)
	);
	linkedList.projection = glm::ortho(-halfX, halfX, -halfY, halfY,
		0.1f, size.z);

	linkedList.width = round(size.x / params.pixelWidth);
	linkedList.height = round(size.y / params.pixelWidth);
	if (linkedList.width > params.maxFBOSize ||
		linkedList.height > params.maxFBOSize) {
		std::cerr << "The model is too big\n";
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

void FLLGenerator::SetupFBO(int width, int height)
{
	glGenFramebuffers(1, &fboHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

	GLuint depthBuf;
	glGenRenderbuffers(1, &depthBuf);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, depthBuf);

	GLenum drawBuffers[] = { GL_NONE };
	glDrawBuffers(1, drawBuffers);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FLLGenerator::SetupShaderStorage(int width, int height)
{
	maxNodes = 20 * width * height;
	nodeSize = sizeof(GLfloat) + sizeof(GLuint);

	prog.Use();
	prog.SetUniform("MaxNodes", maxNodes);

	glGenBuffers(2, buffers);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, buffers[COUNTER_BUFFER]);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_DYNAMIC_DRAW);

	glGenTextures(1, &headPtrTex);
	glBindTexture(GL_TEXTURE_2D, headPtrTex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, width, height);
	glBindImageTexture(0, headPtrTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[LINKED_LIST_BUFFER]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, maxNodes * nodeSize, 0, GL_DYNAMIC_DRAW);

	std::vector<GLuint> headPtrClearBuf(width * height, 0xffffffff);
	glGenBuffers(1, &clearBuf);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, clearBuf);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, headPtrClearBuf.size() * sizeof(GLuint),
		&headPtrClearBuf[0], GL_STATIC_COPY);
}

void FLLGenerator::Generate(Model3D* model3D, LinkedList& linkedList)
{
	Configure(model3D, linkedList);
	SetupFBO(linkedList.width, linkedList.height);
	SetupShaderStorage(linkedList.width, linkedList.height);

	ClearBuffers(linkedList.width, linkedList.height);

	glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[LINKED_LIST_BUFFER]);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glViewport(0, 0, linkedList.width, linkedList.height);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT);

	prog.Use();
	mat4 mvp = linkedList.projection * linkedList.view * linkedList.model;
	prog.SetUniform("MVP", mvp);
	model3D->Render();

	linkedList.list.resize(maxNodes);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
		maxNodes * nodeSize, linkedList.list.data());
	linkedList.headPtr.resize(linkedList.width * linkedList.height);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT,
		linkedList.headPtr.data());
	glFlush();
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	int totalFragments = 0;
	for (int i = 0; i < linkedList.headPtr.size(); i++) {
		int n = linkedList.headPtr[i];
		while (n != 0xffffffff) {
			n = linkedList.list[n].next;
			totalFragments++;
		}
	}
}