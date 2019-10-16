#pragma once

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>
#include <model3d.h>
#include <glm/glm.hpp>

#include "zldni.h"

class SupportPointFinder
{
public:
	SupportPointFinder() {}
	~SupportPointFinder() {}

	void Run(Model3D*);

private:
	struct VertexProp
	{
		VertexProp() {}
		VertexProp(glm::vec3 pos_, bool floatable_)
			: pos(pos_), floatable(floatable_) {}

		glm::vec3 pos;
		bool floatable;
		float distance;
	};
	struct EdgeProp
	{
		EdgeProp() {}
		EdgeProp(float penalty_) : penalty(penalty_) {}

		float penalty;
	};
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
		VertexProp, EdgeProp> Graph;
	typedef Graph::vertex_descriptor Vertex;
	typedef Graph::edge_descriptor Edge;

	struct ExceedCoverage {};
	template<typename DistancePropertyMap, typename FloatablePropertyMap>
	class DijkstraVisitor : boost::default_dijkstra_visitor
	{
	public:
		DijkstraVisitor(float coverage_,
			DistancePropertyMap dm_,
			FloatablePropertyMap fm_)
			: coverage(coverage_), dm(dm_), fm(fm_) {}

		void initialize_vertex(const Vertex& s, const Graph& g) const {}
		void discover_vertex(const Vertex& s, const Graph& g) const
		{
			float d = boost::get(dm, s);
			if (d > coverage)
				throw ExceedCoverage();
		}
		void examine_vertex(const Vertex& s, const Graph& g) const {}
		void examine_edge(const Edge& e, const Graph& g) const {}
		void edge_relaxed(const Edge& e, const Graph& g) const {}
		void edge_not_relaxed(const Edge& e, const Graph& g) const {}
		void finish_vertex(const Vertex& s, const Graph& g) const
		{
			boost::put(fm, s, false);
		}
	private:
		float coverage;
		DistancePropertyMap dm;
		FloatablePropertyMap fm;
	};

	int rows, cols;
	std::vector<std::vector<
		std::vector<std::pair<glm::vec3, Vertex>>>> intersections;
	Graph g;
	std::vector<glm::vec3> supportPoints;

private:
	void ConstructGraph(zLDNIGenerator&);
	void MakeEdgeIfConnected(int, int, glm::vec3, Vertex);
	void FindSupportPoints();
};
