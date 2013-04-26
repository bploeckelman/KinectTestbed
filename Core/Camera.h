#pragma once

#include <glm/glm.hpp>


class Camera {
private:
	glm::mat4 projection;
	glm::mat4 modelview;

	glm::vec3 binormal;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 lastBinormal;
	glm::vec3 lastNormal;
	glm::vec3 lastTangent;

public:
	glm::vec3 position;

	Camera();
	
	void update();

	void setViewport();
	void setProjection();
	void setModelview();

private:
	void apply();
	void updateHandControls();

};
