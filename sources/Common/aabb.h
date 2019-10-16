#pragma once

#include <glm/glm.hpp>
#include <limits>

class AABB
{
public:
	AABB() {
		Reset();
	}
	~AABB() {}

	void Reset() {
		minPoint = glm::vec3(std::numeric_limits<float>::max());
		maxPoint = glm::vec3(std::numeric_limits<float>::min());
	}

	void Add(const glm::vec3& pt) {
		for (int i = 0; i < 3; i++) {
			minPoint[i] = std::fmin(minPoint[i], pt[i]);
			maxPoint[i] = std::fmax(maxPoint[i], pt[i]);
		}
	}
	void Add(const AABB& other) {
		glm::vec3 otherMin = other.GetMin();
		glm::vec3 otherMax = other.GetMax();

		for (int i = 0; i < 3; i++) {
			minPoint[i] = std::fmin(minPoint[i], otherMin[i]);
			maxPoint[i] = std::fmax(maxPoint[i], otherMax[i]);
		}
	}

	glm::vec3 GetMax() const {
		return maxPoint;
	}
	glm::vec3 GetMin() const {
		return minPoint;
	}
	glm::vec3 GetSize() const {
		return maxPoint - minPoint;
	}
	glm::vec3 GetCenter() const {
		return (maxPoint + minPoint) / 2.0f;
	}

private:
	glm::vec3 minPoint, maxPoint;
};
