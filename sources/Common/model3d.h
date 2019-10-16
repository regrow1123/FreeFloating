#pragma once

#include <glad/glad.h>
#include <vector>
#include <memory>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/TriConnectivity.hh>

#include "aabb.h"

class OpenMeshData : public
	OpenMesh::TriMesh_ArrayKernelT<OpenMesh::DefaultTraits>
{
public:
	OpenMeshData() {
		request_face_normals();
		request_vertex_normals();
		request_halfedge_normals();
	}

	bool Read(const std::string& path) {
		OpenMesh::IO::Options ropt;
		if (!OpenMesh::IO::read_mesh(*this, path, ropt)) {
			std::cerr << "Error loading mesh from " <<
				path << std::endl;
			return false;
		}

		if (!ropt.check(OpenMesh::IO::Options::VertexNormal))
			update_normals();
	}
	bool Write(const std::string& path) {
		OpenMesh::IO::Options wopt;
		if (!OpenMesh::IO::write_mesh(*this, path, wopt)) {
			std::cerr << "Unable to write mesh to " <<
				path << std::endl;
			return false;
		}

		return true;
	}
};

class GLMeshData
{
public:
	GLMeshData() {}
	GLMeshData(OpenMeshData& arg) {
		for (OpenMeshData::VIter vit = arg.vertices_begin();
			vit != arg.vertices_end();
			vit++) {
			OpenMeshData::Point p = arg.point(*vit);
			OpenMeshData::Normal n = arg.normal(*vit);

			for (int i = 0; i < 3; i++) {
				points.push_back(p[i]);
				normals.push_back(n[i]);
			}
		}

		for (OpenMeshData::FIter fit = arg.faces_begin();
			fit != arg.faces_end();
			fit++) {
			for (OpenMeshData::FVIter fvit = arg.fv_begin(*fit);
				fvit != arg.fv_end(*fit);
				fvit++)
				indices.push_back(fvit->idx());
		}
	}
	~GLMeshData() {}

	void Clear() {
		points.clear();
		normals.clear();
		indices.clear();
	}

	std::vector<GLfloat> points, normals;
	std::vector<GLuint> indices;
};

class TriMesh
{
protected:
	GLuint nElements;
	GLuint vao;

	std::vector<GLuint> buffers;

	virtual void InitBuffers(
		std::vector<GLuint>& indices,
		std::vector<GLfloat>& points,
		std::vector<GLfloat>& normals
	) {
		nElements = indices.size();

		GLuint indexBuf = 0, posBuf = 0, normBuf = 0;

		glGenBuffers(1, &indexBuf);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
			indices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &posBuf);
		glBindBuffer(GL_ARRAY_BUFFER, posBuf);
		glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(GLfloat),
			points.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &normBuf);
		glBindBuffer(GL_ARRAY_BUFFER, normBuf);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
			normals.data(), GL_STATIC_DRAW);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);

		glBindBuffer(GL_ARRAY_BUFFER, posBuf);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, normBuf);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);

		buffers.push_back(indexBuf);
		buffers.push_back(posBuf);
		buffers.push_back(normBuf);
	}

	void DeleteBuffers() {
		if (!buffers.empty()) {
			glDeleteBuffers(buffers.size(), buffers.data());
			buffers.clear();
		}

		if (vao != 0) {
			glDeleteVertexArrays(1, &vao);
			vao = 0;
		}
	}

public:
	TriMesh() : vao(0), nElements(0) {}
	virtual ~TriMesh() {
		DeleteBuffers();
	}

	virtual void Render() const {
		if (vao == 0)
			return;

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, nElements, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
};

class Model3D : public TriMesh
{
private:

public:
	AABB aabb;
	OpenMeshData openMeshData;

	static std::unique_ptr<Model3D> Load(const std::string& path) {
		std::unique_ptr<Model3D> model3D(new Model3D);

		model3D->openMeshData.Read(path);

		GLMeshData glMeshData(model3D->openMeshData);

		model3D->InitBuffers(glMeshData.indices,
			glMeshData.points,
			glMeshData.normals);

		for (int i = 0; i < glMeshData.points.size(); i += 3) {
			glm::vec3 pt(glMeshData.points[i],
				glMeshData.points[i + 1],
				glMeshData.points[i + 2]);
			model3D->aabb.Add(pt);
		}

		return model3D;
	}
};
