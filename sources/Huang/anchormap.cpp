#include "anchormap.h"
#include "binaryimages.h"

#include <params.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stopwatch.h>
using glm::vec3;
using glm::mat4;
using cv::Mat;
using std::chrono::nanoseconds;
using std::cout;
using std::endl;

void AnchorMapGenerator::Run(Model3D* model3D)
{
	vec3 size = model3D->aabb.GetSize();
	Params& params = Params::GetInstance();

	float val = size.z / params.sliceThickness;
	int quot = floor(val);
	float fract = val - quot;
	if (fract < 0.5)
		quot--;

	BinaryImageSampler sampler(model3D);
	int cols, rows;
	sampler.GetImageSize(cols, rows);
	Mat upperPart = Mat::zeros(rows, cols, CV_8UC1);
	Mat upperAnchorMap = Mat::zeros(rows, cols, CV_8UC1);
	StopWatch::GetInstance().Hit();
	for (int i = quot; i >= 0; i--) {
		float z = (i + 0.5) * params.sliceThickness;
		z -= size.z / 2.0;

		cv::Mat currentPart = sampler.Slice(z);
		if (i == quot) {
			upperPart = currentPart.clone();
			continue;
		}

		cv::Mat shadow = Subtract(upperPart, currentPart);
		cv::Mat pa = Subtract(upperAnchorMap, currentPart);
		cv::Mat supportRegion = GrowingSwallow(
			GrowingSwallow(shadow, currentPart, upperPart, params.selfSupportThres),
			pa, pa, params.effectiveRadius);
		cv::Mat anchorMap = GenAnchorMap(supportRegion, params.effectiveRadius);
		anchorMap = Union(anchorMap, pa);

		upperPart = currentPart.clone();
		upperAnchorMap = anchorMap.clone();
	}
	nanoseconds t = StopWatch::GetInstance().Hit();
	cout << "time for computing anchormaps : " << t.count() << endl;
}

cv::Mat AnchorMapGenerator::Subtract(cv::Mat a, cv::Mat b)
{
	Mat result;
	cv::bitwise_xor(a, b, result);
	cv::bitwise_and(a, result, result);

	return result;
}

cv::Mat AnchorMapGenerator::Intersect(cv::Mat a, cv::Mat b)
{
	Mat result;
	cv::bitwise_and(a, b, result);
	return result;
}

cv::Mat AnchorMapGenerator::Union(cv::Mat a, cv::Mat b)
{
	Mat result;
	cv::bitwise_or(a, b, result);
	return result;
}

cv::Mat AnchorMapGenerator::Invert(cv::Mat m)
{
	Mat result;
	cv::bitwise_not(m, result);
	return result;
}

bool AnchorMapGenerator::IsZeros(cv::Mat m)
{
	for (int i = 0; i < m.rows; i++) {
		for (int j = 0; j < m.cols; j++) {
			if (m.at<uchar>(i, j) != 0)
				return false;
		}
	}
	return true;
}

cv::Mat AnchorMapGenerator::GrowingSwallow(
	cv::Mat psi, cv::Mat p1, cv::Mat p2, float t)
{
	Params& params = Params::GetInstance();
	float radius = t / params.pixelWidth;

	Mat result = psi.clone();

	Mat a = Intersect(p1, p2);
	Mat b = a.clone();
	Mat c = b.clone();

	Mat dist;
	cv::distanceTransform(Invert(p1), dist, cv::DIST_L2, cv::DIST_MASK_3, CV_32F);
	cv::threshold(dist, dist, radius, 255, cv::THRESH_BINARY_INV);
	dist.convertTo(dist, CV_8UC1);

	while (1) {
		if (IsZeros(c))
			break;

		cv::dilate(a, b, cv::getStructuringElement(
			cv::MORPH_RECT, cv::Size(3, 3)));
		c = Intersect(Intersect(Subtract(b, a), dist), result);
		a = Union(c, a);
		result = Subtract(result, a);
	}

	return result;
}

cv::Mat AnchorMapGenerator::GenAnchorMap(cv::Mat supportRegion, float ta)
{
	Params& params = Params::GetInstance();
	int gridWidth = floor(1.414 * ta / params.pixelWidth);
	int rows = supportRegion.rows;
	int cols = supportRegion.cols;

	Mat anchorMap = Mat::zeros(rows, cols, CV_8UC1);
	for (int i = 0; i < rows; i += gridWidth) {
		for (int j = 0; j < cols; j += gridWidth) {
			if (supportRegion.at<uchar>(i, j) != 0)
				anchorMap.at<uchar>(i, j) = 255;
		}
	}

	supportRegion = GrowingSwallow(supportRegion, anchorMap, anchorMap, ta);

	for (int i = 0; i < rows; i += gridWidth) {
		bool inside = false;
		int enter, exit;
		for (int j = 0; j < cols; j++) {
			if (supportRegion.at<uchar>(i, j) != 0) {
				if (inside)
					continue;

				inside = true;
				enter = j;
			}
			else if (inside) {
				inside = false;
				exit = j - 1;
				int center = round((enter + exit) / 2.0);
				anchorMap.at<uchar>(i, center) = 255;
			}
		}
	}

	for (int j = 0; j < cols; j += gridWidth) {
		bool inside = false;
		int enter, exit;
		for (int i = 0; i < rows; i++) {
			if (supportRegion.at<uchar>(i, j) != 0) {
				if (inside)
					continue;

				inside = true;
				enter = i;
			}
			else if (inside) {
				inside = false;
				exit = i - 1;
				int center = round((enter + exit) / 2.0);
				anchorMap.at<uchar>(center, j) = 255;
			}
		}
	}

	supportRegion = GrowingSwallow(supportRegion, anchorMap, anchorMap, ta);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if (supportRegion.at<uchar>(i, j) != 0) {
				anchorMap.at<uchar>(i, j) = 255;
				supportRegion =
					GrowingSwallow(supportRegion, anchorMap, anchorMap, ta);
			}
		}
	}

	return anchorMap;
}