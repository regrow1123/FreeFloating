#include "overhang.h"
#include "rasterizer.h"

#include <params.h>
#include <stopwatch.h>
using glm::vec3;
using std::chrono::nanoseconds;
using std::cout;
using std::endl;

void OverhangDetector::Run(Model3D* model3D)
{
	target = model3D;

	StopWatch::GetInstance().Hit();
	DetectPointOverhangs();
	nanoseconds t = StopWatch::GetInstance().Hit();
	cout << "time for detecting point overhangs : " << t.count() << endl;
	DetectEdgeOverhangs();
	t = StopWatch::GetInstance().Hit();
	cout << "time for detecting edge overhangs : " << t.count() << endl;
	DetectFaceOverhangs();
	t = StopWatch::GetInstance().Hit();
	cout << "time for detecting face overhangs : " << t.count() << endl;
}

void OverhangDetector::DetectPointOverhangs()
{
	OpenMeshData& mesh = target->openMeshData;
	for (OpenMeshData::VIter vit = mesh.vertices_begin();
		vit != mesh.vertices_end();
		vit++) {
		OpenMeshData::Point p = mesh.point(*vit);
		bool flag = false;
		for (OpenMeshData::VVIter vvit = mesh.vv_begin(*vit);
			vvit != mesh.vv_end(*vit);
			vvit++) {
			OpenMeshData::Point neighbor = mesh.point(*vvit);
			if (p[2] >= neighbor[2])
				flag = true;
		}
		if (!flag)
			pointOverhang.push_back(vec3(p[0], p[1], p[2]));
	}
}

void OverhangDetector::DetectEdgeOverhangs()
{
	OpenMeshData& mesh = target->openMeshData;

	Params& params = Params::GetInstance();
	for (OpenMeshData::HIter hit = mesh.halfedges_begin();
		hit != mesh.halfedges_end();
		hit++) {
		OpenMeshData::Normal n = mesh.normal(*hit);
		if (IsOverhang(n)) {
			OpenMeshData::FHandle fh1 = mesh.face_handle(*hit);
			OpenMeshData::FHandle fh2 = mesh.opposite_face_handle(*hit);
			if (!fh2.is_valid())
				continue;

			OpenMeshData::Normal fn1 = mesh.normal(fh1);
			OpenMeshData::Normal fn2 = mesh.normal(fh2);

			if (IsOverhang(fn1) || IsOverhang(fn2))
				continue;

			OpenMeshData::VHandle fvh = mesh.from_vertex_handle(*hit);
			OpenMeshData::VHandle tvh = mesh.to_vertex_handle(*hit);
			OpenMeshData::Point fp = mesh.point(fvh);
			OpenMeshData::Point tp = mesh.point(tvh);
			float len = OpenMesh::norm(tp - fp);
			float x = 0;
			glm::vec3 s(fp[0], fp[1], fp[2]);
			glm::vec3 d(tp[0], tp[1], tp[2]);
			glm::vec3 dir = (d - s) / len;
			while (x < len) {
				glm::vec3 p = s + dir * x;
				edgeOverhang.push_back(p);
				x += params.samplingResolution;
			}
		}
	}
}

void OverhangDetector::DetectFaceOverhangs()
{
	OpenMeshData& mesh = target->openMeshData;

	std::vector<GLfloat> points;
	Params& params = Params::GetInstance();

	for (OpenMeshData::FIter fit = mesh.faces_begin();
		fit != mesh.faces_end();
		fit++) {
		OpenMeshData::Normal n = mesh.normal(*fit);
		float angle = std::acos(
			OpenMesh::dot(n, OpenMeshData::Normal(0, 0, -1)));
		if (angle < params.overhangAngle) {
			for (OpenMeshData::FVIter fvit = mesh.fv_begin(*fit);
				fvit != mesh.fv_end(*fit);
				fvit++) {
				OpenMeshData::Point p = mesh.point(*fvit);
				points.push_back(p[0]);
				points.push_back(p[1]);
				points.push_back(p[2]);
			}
		}
	}

	Triangles3D triangles;
	triangles.InitBuffers(points);
	for (int i = 0; i < points.size(); i += 3) {
		vec3 pt(points[i], points[i + 1], points[i + 2]);
		triangles.aabb.Add(pt);
	}

	Rasterizer rasterizer(&triangles, params.samplingResolution);
	rasterizer.Sample(faceOverhang);
}

bool OverhangDetector::IsOverhang(OpenMeshData::Normal n)
{
	Params& params = Params::GetInstance();
	float angle =
		std::acos(OpenMesh::dot(n, OpenMeshData::Normal(0, 0, -1)));
	if (angle < params.overhangAngle)
		return true;
	else
		return false;
}