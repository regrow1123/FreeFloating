#pragma once

#include <model3d.h>
#include <glm/glm.hpp>

class OverhangDetector
{
public:
	OverhangDetector() {}
	~OverhangDetector() {}

	void Run(Model3D*);

private:
	void DetectPointOverhangs();
	void DetectEdgeOverhangs();
	void DetectFaceOverhangs();

	bool IsOverhang(OpenMeshData::Normal);

private:
	Model3D* target;

	std::vector<glm::vec3> pointOverhang;
	std::vector<glm::vec3> edgeOverhang;
	std::vector<glm::vec3> faceOverhang;
};
