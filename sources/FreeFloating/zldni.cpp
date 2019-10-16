#include "zldni.h"

#include <GLFW/glfw3.h>
#include <params.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stopwatch.h>
using glm::vec3;
using glm::mat4;
using cv::Mat;
using std::chrono::nanoseconds;
using std::cout;
using std::endl;

zLDNIGenerator::zLDNIGenerator(Model3D* model3D)
{
	prog.CompileShader("./shader/fragmentlist.vs", GLSLShader::VERTEX);
	prog.CompileShader("./shader/fragmentlist.fs", GLSLShader::FRAGMENT);
	prog.Link();

	target = model3D;

	Configure();
	SetupFBO();
	SetupShaderStorage();
	StopWatch::GetInstance().Hit();
	ClearBuffers();
	Run();
	nanoseconds t = StopWatch::GetInstance().Hit();
	cout << "time for computing fragment list : " << t.count() << endl;
}

void zLDNIGenerator::GetImageSize(int& w, int& h)
{
	w = width;
	h = height;
}

void zLDNIGenerator::GetSortedList(std::vector<glm::vec3>& nodes,
	int row, int col)
{
	GLuint n = headPtr[width * row + col];
	while (n != 0xffffffff) {
		ListNode& node = list[n];
		float d = node.depth;
		vec3 pos = glm::unProject(vec3(col, row, d), view * model, projection,
			glm::vec4(0, 0, width, height));
		nodes.push_back(pos);
		n = node.next;
	}

	if (nodes.empty())
		return;

	std::sort(nodes.begin(), nodes.end(), [](glm::vec3 lhs, glm::vec3 rhs) {
		return lhs[2] < rhs[2];
		});
}

void zLDNIGenerator::Configure()
{
	vec3 center = target->aabb.GetCenter();
	vec3 size = target->aabb.GetSize();

	Params& params = Params::GetInstance();
	float margin = params.pixelWidth * 10;
	size += vec3(margin);
	float halfX = size.x / 2.0f;
	float halfY = size.y / 2.0f;
	float halfZ = size.z / 2.0f;

	model = glm::translate(mat4(1.0f), -center);
	view = glm::lookAt(
		vec3(0.0f, 0.0f, halfZ),
		vec3(0.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f)
	);
	projection = glm::ortho(-halfX, halfX, -halfY, halfY,
		0.1f, size.z);

	width = round(size.x / params.pixelWidth);
	height = round(size.y / params.pixelWidth);
	if (width > params.maxFBOSize || height > params.maxFBOSize) {
		std::cerr << "The model is too big\n";
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

void zLDNIGenerator::SetupFBO()
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

void zLDNIGenerator::SetupShaderStorage()
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

void zLDNIGenerator::ClearBuffers()
{
	GLuint zero = 0;
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, buffers[COUNTER_BUFFER]);
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, clearBuf);
	glBindTexture(GL_TEXTURE_2D, headPtrTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED_INTEGER,
		GL_UNSIGNED_INT, 0);
}

void zLDNIGenerator::Run()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[LINKED_LIST_BUFFER]);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glViewport(0, 0, width, height);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT);

	prog.Use();
	prog.SetUniform("MVP", projection * view * model);
	target->Render();

	list.resize(maxNodes);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
		maxNodes * nodeSize, list.data());
	headPtr.resize(width * height);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT,
		headPtr.data());
	glFlush();
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}