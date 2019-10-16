#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <opencv2/opencv.hpp>
#include <model3d.h>

class AnchorMapGenerator
{
public:
	AnchorMapGenerator() {}
	~AnchorMapGenerator() {}

	void Run(Model3D*);

private:
	cv::Mat Subtract(cv::Mat, cv::Mat);
	cv::Mat Intersect(cv::Mat, cv::Mat);
	cv::Mat Union(cv::Mat, cv::Mat);
	cv::Mat Invert(cv::Mat);
	bool IsZeros(cv::Mat);

	cv::Mat GrowingSwallow(cv::Mat, cv::Mat, cv::Mat, float);

	cv::Mat GenAnchorMap(cv::Mat, float);
};
