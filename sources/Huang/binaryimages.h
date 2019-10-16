#pragma once

#include <glslprogram.h>
#include <model3d.h>
#include <opencv2/opencv.hpp>

class BinaryImageSampler
{
private:
	Model3D* target;

	GLSLProgram prog;
	GLuint fboHandle;

	int width, height;
	glm::mat4 model, view, projection;

	std::vector<cv::Mat> ldni;

public:
	BinaryImageSampler(Model3D*);
	~BinaryImageSampler() {}

	void GetImageSize(int&, int&);

	cv::Mat Slice(float h);

private:
	void Configure();
	void SetupFBO();
	void Sample();
	void Sort();
};
