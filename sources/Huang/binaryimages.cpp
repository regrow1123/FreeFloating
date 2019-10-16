#include "binaryimages.h"

#include <params.h>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <stopwatch.h>

using glm::mat4;
using glm::vec3;
using cv::Mat;
using std::chrono::nanoseconds;
using std::cout;
using std::endl;

BinaryImageSampler::BinaryImageSampler(Model3D* model3D)
{
	prog.CompileShader("./shader/ldni.vs", GLSLShader::VERTEX);
	prog.CompileShader("./shader/ldni.fs", GLSLShader::FRAGMENT);
	prog.Link();

	target = model3D;
	Configure();
	SetupFBO();
	StopWatch::GetInstance().Hit();
	Sample();
	Sort();
	nanoseconds t = StopWatch::GetInstance().Hit();
	cout << "time for computing LDNI : " << t.count() << endl;
}

void BinaryImageSampler::GetImageSize(int& w, int& h)
{
	w = width;
	h = height;
}

cv::Mat BinaryImageSampler::Slice(float h)
{
	vec3 projected = glm::project(vec3(0, 0, h), view, projection,
		glm::vec4(0, 0, width, height));

	int cols = width;
	int rows = height;

	cv::Mat slice(rows, cols, CV_8UC1);
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			int cnt = 0;
			for (int k = 0; k < ldni.size(); k++) {
				float d = ldni[k].at<float>(i, j);
				if (d == 0.0f)
					break;
				else if (d < projected.z)
					cnt++;
			}

			if (cnt % 2 == 0)
				slice.at<uchar>(i, j) = 0;
			else
				slice.at<uchar>(i, j) = 255;
		}
	}

	return slice;
}

void BinaryImageSampler::Configure()
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

	width = round(size.x / params.pixelWidth);
	height = round(size.y / params.pixelWidth);
	if (width > params.maxFBOSize || height > params.maxFBOSize) {
		std::cerr << "The model is too big\n";
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

void BinaryImageSampler::SetupFBO()
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

void BinaryImageSampler::Sample()
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
	Mat layer(height, width, CV_32FC1);
	for (int i = 0; i < stencil.rows; i++) {
		for (int j = 0; j < stencil.cols; j++) {
			if (stencil.at<uchar>(i, j) >= 1) {
				layer.at<float>(i, j) = depth.at<float>(i, j);
				totalFragments++;
			}
			else
				layer.at<float>(i, j) = 0.0f;
		}
	}
	ldni.push_back(layer.clone());

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
					layer.at<float>(m, n) = depth.at<float>(m, n);
					totalFragments++;
				}
				else
					layer.at<float>(m, n) = 0.0f;
			}
		}
		ldni.push_back(layer.clone());
	}

	glBindBuffer(GL_FRAMEBUFFER, 0);

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_ALWAYS, 0, 0xff);
	glDepthFunc(GL_LESS);
	glDisable(GL_STENCIL_TEST);
}

void BinaryImageSampler::Sort()
{
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			std::vector<float> intersections;
			for (int k = 0; k < ldni.size(); k++) {
				if (ldni[k].at<float>(i, j) == 0.0f)
					break;

				intersections.push_back(ldni[k].at<float>(i, j));
			}

			if (intersections.empty())
				continue;

			std::sort(intersections.begin(), intersections.end());

			for (int k = 0; k < intersections.size(); k++)
				ldni[k].at<float>(i, j) = intersections[k];
		}
	}
}