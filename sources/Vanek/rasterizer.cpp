#include "rasterizer.h"

#include <params.h>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
using glm::vec3;
using glm::mat4;
using cv::Mat;

Triangles3D::Triangles3D()
	: vao(0),
	verticesN(0)
{
}

Triangles3D::~Triangles3D()
{
	DeleteBuffers();
}

void Triangles3D::DeleteBuffers()
{
	if (!buffers.empty()) {
		glDeleteBuffers(buffers.size(), buffers.data());
		buffers.clear();
	}

	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
}

void Triangles3D::InitBuffers(std::vector<GLfloat>& points)
{
	verticesN = points.size();

	GLuint posBuf = 0;

	glGenBuffers(1, &posBuf);
	glBindBuffer(GL_ARRAY_BUFFER, posBuf);
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(GLfloat),
		points.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, posBuf);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	buffers.push_back(posBuf);
}

void Triangles3D::Render()
{
	if (vao == 0)
		return;

	glBindVertexArray(vao);
	glDrawArrays(GL_LINES, 0, verticesN);
	glBindVertexArray(0);
}

void Rasterizer::Configure()
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
	view = glm::lookAt(vec3(0.0f, 0.0f, halfZ),
		vec3(0.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f));
	projection = glm::ortho(-halfX, halfX, -halfY, halfY, 0.1f, size.z);

	width = round(size.x / resolution);
	height = round(size.y / resolution);
	if (width > params.maxFBOSize || height > params.maxFBOSize) {
		std::cerr << "The model is too big\n";
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

void Rasterizer::SetupFBO()
{
	glGenFramebuffers(1, &fboHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

	GLuint dsTex;
	glGenTextures(1, &dsTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, dsTex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH32F_STENCIL8, width, height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D, dsTex, 0);

	GLenum drawBuffers[] = { GL_NONE };
	glDrawBuffers(1, drawBuffers);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Rasterizer::Rasterizer(Triangles3D* triangles, float resolution_)
{
	prog.CompileShader("./shader/ldni.vs", GLSLShader::VERTEX);
	prog.CompileShader("./shader/ldni.fs", GLSLShader::FRAGMENT);
	prog.Link();

	target = triangles;
	resolution = resolution_;

	Configure();
	SetupFBO();
}

void Rasterizer::Sample(std::vector<glm::vec3>& points)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
	prog.Use();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glViewport(0, 0, width, height);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClearDepth(1.0);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glDepthFunc(GL_ALWAYS);
	glStencilFunc(GL_GREATER, 1, 0xff);
	glStencilOp(GL_INCR, GL_INCR, GL_INCR);

	prog.SetUniform("MVP", projection * view * model);
	target->Render();

	Mat depth(height, width, CV_32FC1);
	glPixelStorei(GL_PACK_ALIGNMENT, (depth.step & 3) ? 1 : 4);
	glPixelStorei(GL_PACK_ROW_LENGTH, depth.step / depth.elemSize());
	glReadPixels(0, 0, width, height,
		GL_DEPTH_COMPONENT, GL_FLOAT, depth.data);
	cv::flip(depth, depth, 0);

	Mat stencil(height, width, CV_8UC1);
	glPixelStorei(GL_PACK_ALIGNMENT, (stencil.step & 3) ? 1 : 4);
	glPixelStorei(GL_PACK_ROW_LENGTH, stencil.step / stencil.elemSize());
	glReadPixels(0, 0, width, height,
		GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil.data);
	cv::flip(stencil, stencil, 0);

	int maxDepthComplex = 0;
	for (int m = 0; m < stencil.rows; m++) {
		for (int n = 0; n < stencil.cols; n++) {
			if (stencil.at<uchar>(m, n) > maxDepthComplex)
				maxDepthComplex = stencil.at<uchar>(m, n);
		}
	}

	int totalFragments = 0;
	for (int i = 0; i < stencil.rows; i++) {
		for (int j = 0; j < stencil.cols; j++) {
			if (stencil.at<uchar>(i, j) >= 1) {
				float d = depth.at<float>(i, j);
				vec3 pos = glm::unProject(vec3(j, i, d),
					view * model, projection, glm::vec4(0, 0, width, height));
				points.push_back(pos);
				totalFragments++;
			}
		}
	}

	glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
	for (int i = 2; i <= maxDepthComplex; i++) {
		glStencilFunc(GL_GREATER, i, 0xff);

		glClear(GL_COLOR_BUFFER_BIT |
			GL_DEPTH_BUFFER_BIT |
			GL_STENCIL_BUFFER_BIT);

		target->Render();

		glPixelStorei(GL_PACK_ALIGNMENT, (stencil.step & 3) ? 1 : 4);
		glPixelStorei(GL_PACK_ROW_LENGTH, stencil.step / stencil.elemSize());
		glReadPixels(0, 0, width, height,
			GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil.data);
		cv::flip(stencil, stencil, 0);

		glPixelStorei(GL_PACK_ALIGNMENT, (depth.step & 3) ? 1 : 4);
		glPixelStorei(GL_PACK_ROW_LENGTH, depth.step / depth.elemSize());
		glReadPixels(0, 0, width, height,
			GL_DEPTH_COMPONENT, GL_FLOAT, depth.data);
		cv::flip(depth, depth, 0);

		for (int m = 0; m < stencil.rows; m++) {
			for (int n = 0; n < stencil.cols; n++) {
				if (stencil.at<uchar>(m, n) >= i) {
					float d = depth.at<float>(m, n);
					vec3 pos = glm::unProject(vec3(n, m, d),
						view * model, projection, glm::vec4(0, 0, width, height));
					points.push_back(pos);
					totalFragments++;
				}
			}
		}
	}

	glBindBuffer(GL_FRAMEBUFFER, 0);

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_ALWAYS, 0, 0xff);
	glDepthFunc(GL_LESS);
	glDisable(GL_STENCIL_TEST);
}