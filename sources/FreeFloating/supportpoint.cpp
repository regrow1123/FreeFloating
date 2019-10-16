#include "supportpoint.h"

#include <params.h>
#include <stopwatch.h>
using glm::vec3;
using glm::uvec3;
using std::chrono::nanoseconds;
using std::cout;
using std::endl;

void SupportPointFinder::Run(Model3D* model3D)
{
	zLDNIGenerator generator(model3D);

	StopWatch::GetInstance().Hit();
	ConstructGraph(generator);
	nanoseconds t = StopWatch::GetInstance().Hit();
	cout << "time for constructing graph : " << t.count() << endl;
	FindSupportPoints();
	t = StopWatch::GetInstance().Hit();
	cout << "time for finding support points : " << t.count() << endl;
}

void SupportPointFinder::ConstructGraph(zLDNIGenerator& generator)
{
	generator.GetImageSize(cols, rows);
	intersections.resize(rows);
	for (int i = 0; i < rows; i++)
		intersections[i].resize(cols);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			std::vector<vec3> list;
			generator.GetSortedList(list, i, j);
			if (list.empty())
				continue;

			intersections[i][j].resize(list.size());
			for (int k = 0; k < list.size(); k++) {
				intersections[i][j][k].first = list[k];
				Vertex v = boost::add_vertex(VertexProp(list[k], true), g);
				intersections[i][j][k].second = v;
			}
		}
	}
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			for (int k = 0; k < intersections[i][j].size(); k += 2) {
				vec3 pos = intersections[i][j][k].first;
				Vertex d = intersections[i][j][k].second;

				MakeEdgeIfConnected(i - 1, j - 1, pos, d);
				MakeEdgeIfConnected(i - 1, j, pos, d);
				MakeEdgeIfConnected(i - 1, j + 1, pos, d);
				MakeEdgeIfConnected(i, j - 1, pos, d);
				MakeEdgeIfConnected(i, j + 1, pos, d);
				MakeEdgeIfConnected(i + 1, j - 1, pos, d);
				MakeEdgeIfConnected(i + 1, j, pos, d);
				MakeEdgeIfConnected(i + 1, j + 1, pos, d);

				Vertex up = intersections[i][j][k + 1].second;
				boost::add_edge(d, up, EdgeProp(0), g);
			}
		}
	}
}

void SupportPointFinder::MakeEdgeIfConnected(int row, int col,
	glm::vec3 pos, Vertex d)
{
	if (row > rows - 1 || row < 0 || col > cols - 1 || col < 0)
		return;

	Params& params = Params::GetInstance();
	int machineZ =
		ceil((pos[2] - params.sliceThickness * 0.5) / params.sliceThickness);
	for (int i = 0; i < intersections[row][col].size(); i += 2) {
		float entryZ = intersections[row][col][i].first[2];
		int machineEntryZ = ceil((entryZ - params.sliceThickness * 0.5) /
			params.sliceThickness);

		if (machineEntryZ > machineZ)
			return;

		float exitZ = intersections[row][col][i + 1].first[2];
		int machineExitZ =
			ceil((exitZ - params.sliceThickness * 0.5) / params.sliceThickness);

		if (machineExitZ >= machineZ)
		{
			Vertex s = intersections[row][col][i].second;
			vec3 dir = glm::normalize(pos - intersections[row][col][i].first);
			float angle = glm::acos(glm::dot(vec3(0, 0, 1), dir));
			float weight = 0;
			if (angle > params.overhangAngle)
				weight = std::min(1.0f, 1.0f / (3.141592f / 2.0f - params.overhangAngle) *
				(angle - params.overhangAngle));
			boost::add_edge(s, d, EdgeProp(weight), g);
			return;
		}
	}
}

void SupportPointFinder::FindSupportPoints()
{
	Params& params = Params::GetInstance();
	float coverage = params.effectiveRadius / params.pixelWidth;

	std::vector<std::pair<Vertex, float>> vertices;

	Graph::vertex_iterator vi, v_end;
	for (boost::tie(vi, v_end) = boost::vertices(g); vi != v_end; vi++)
		vertices.push_back(std::make_pair(*vi, g[*vi].pos[2]));

	std::sort(vertices.begin(), vertices.end(),
		[](std::pair<Vertex, float> a, std::pair<Vertex, float> b) {
			return a.second < b.second;
		});

	for (int i = 0; i < vertices.size(); i++) {
		Vertex v = vertices[i].first;
		if (!g[v].floatable)
			continue;

		supportPoints.push_back(g[v].pos);

		DijkstraVisitor<boost::property_map<Graph, float VertexProp::*>::type,
			boost::property_map<Graph, bool VertexProp::*>::type>
			dijkstraVisitor(coverage,
				boost::get(&VertexProp::distance, g),
				boost::get(&VertexProp::floatable, g));
		try
		{
			boost::dijkstra_shortest_paths(g, v,
				boost::weight_map(boost::get(&EdgeProp::penalty, g))
				.distance_map(boost::get(&VertexProp::distance, g))
				.visitor(dijkstraVisitor));
		}
		catch (ExceedCoverage&)
		{
		}
	}
}