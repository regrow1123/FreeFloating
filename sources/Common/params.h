#pragma once

#include <memory>
#include <mutex>

class Params
{
public:
	static Params& GetInstance();

	float sliceThickness;
	int dpi;
	float pixelWidth;
	int maxFBOSize;

	float selfSupportThres;
	float effectiveRadius;
	float overhangAngle;

	float samplingResolution;

private:
	static std::unique_ptr<Params> instance;
	static std::once_flag flag;
};
