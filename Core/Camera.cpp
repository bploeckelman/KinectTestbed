#include "Camera.h"
#include "Constants.h"
#include "Application.h"

#include <SFML/OpenGL.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace constants;

float getCameraRotationX();
float getCameraRotationY();

Camera::Camera()
	: position(initial_camera_pos)
	, projection(1.f)
	, modelview(1.f)
	, binormal(worldX)
	, normal(worldY)
	, tangent(worldZ)
	, lastBinormal(worldX)
	, lastNormal(worldY)
	, lastTangent(worldZ)
{}

void Camera::apply()
{
	modelview = glm::translate(glm::mat4(1), glm::vec3(position * -1.f));
	if (Application::request().getGUI().isHandControlEnabled()) {
		modelview[0][0] = binormal.x; modelview[0][1] = binormal.y; modelview[0][2] = binormal.z;
		modelview[1][0] = normal.x;   modelview[1][1] = normal.y;   modelview[1][2] = normal.z;
		modelview[2][0] = tangent.x;  modelview[2][1] = tangent.y;  modelview[2][2] = tangent.z;
		modelview = glm::rotate(modelview, 180.f, constants::worldY); // Face skeleton
	} else {
		modelview = glm::rotate(modelview, getCameraRotationX(), constants::worldX);
		modelview = glm::rotate(modelview, getCameraRotationY(), constants::worldY);
		modelview = glm::rotate(modelview, 180.f, constants::worldY);
	}
	glLoadMatrixf(glm::value_ptr(modelview));
}

void Camera::update()
{
	updateHandControls();

	apply();


	lastBinormal = binormal;
	lastNormal = normal;
	lastTangent = tangent;
}

void Camera::setViewport()
{
	glViewport(0, 0, window_width, window_height);
}

void Camera::setProjection()
{
	const float aspect = (float) window_width / (float) window_height;
	projection = glm::perspective(camera_fov, aspect, camera_z_near, camera_z_far);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(projection));
}

void Camera::setModelview()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(modelview));
}

void Camera::updateHandControls()
{
	/* TODO : this is a mess... sort it out
	bool useLast = false;
	glm::vec3 binormal = worldX;
	glm::vec3 normal   = worldY;
	glm::vec3 tangent  = worldZ;
	if (gui.isHandControlEnabled() && !skeleton.getCurrentJointFrame().joints.empty()) {
		const Joint& rightHand = skeleton.getCurrentRightHand();
		const Joint& leftHand  = skeleton.getCurrentLeftHand();
		//const Joint& head      = skeleton.getCurrentJointFrame().at(Skeleton::HEAD);
		if (rightHand.trackingState == TRACKED && leftHand.trackingState == TRACKED) {
			// Build a coordinate frame, but only if hands are reasonably far apart
			if (glm::distance(rightHand.position, leftHand.position) > 0.35f) {
				// Build a coordinate frame using left hand as origin and norm(right - left) as Y axis vector 
				//glm::vec3 tempBinormal;
				//normal       = glm::normalize(rightHand.position - leftHand.position);
				//tempBinormal = glm::normalize(glm::cross(constants::worldZ, normal));
				//tangent      = glm::normalize(glm::cross(normal, tempBinormal));
				//binormal     = glm::normalize(glm::cross(tangent, normal));

				// Build a coordinate frame using left hand as origin and norm(right - left) as X axis vector
				//glm::vec3 tempTangent;
				//binormal    = glm::normalize(rightHand.position - leftHand.position);
				//tempTangent = glm::normalize(glm::cross(binormal, constants::worldY));
				//normal      = glm::normalize(glm::cross(tempTangent, binormal));
				//tangent     = glm::normalize(glm::cross(binormal, normal));

				glm::vec3 tempTangent;
				binormal    = glm::normalize(rightHand.position - leftHand.position);
				//const float d = glm::dot(binormal, constants::worldX);
				//const float angle = acos(d) * 180.f / constants::pi;
				tempTangent = glm::normalize(glm::cross(binormal, constants::worldY));
				normal      = glm::normalize(glm::cross(tempTangent, binormal));
				tangent     = glm::normalize(glm::cross(binormal, normal));
			} else {
				binormal = camera.getLastBinormal();
				normal   = camera.getLastNormal();
				tangent  = camera.getLastTangent();
			}

			// Only enable hand distance zoom control if both hands are above the head
			//if (leftHand.position.y >= head.position.y && rightHand.position.y >= head.position.y) {
				const float dist = glm::distance(rightHand.position, leftHand.position) * constants::zoom_scale;
				camera.position.z = (dist < constants::min_zoom) ? constants::min_zoom 
						: (dist > constants::max_zoom) ? constants::max_zoom : dist;
			//}
		} else {
			binormal = camera.getLastBinormal();
			normal   = camera.getLastNormal();
			tangent  = camera.getLastTangent();
		}
	}
	*/
}

// TODO : this is ugly as hell, make it cleaner 
float getCameraRotationX()
{
	static const int middleY = window_height / 2;
	static const int scrollThreshold = 20;
	static const float timeThreshold = 0.05f;
	static const float rotBound      = 90;
	static const float scale         = 5.f;
	static const sf::Clock clock;

	static float rot = 0.f;
	static float lastTime = clock.getElapsedTime().asSeconds();

	float thisTime = clock.getElapsedTime().asSeconds();
	float delta    = thisTime - lastTime;
	if (delta > timeThreshold) {
		if (Application::request().isRightMouseDown()) {
			const int currY = Application::request().getMousePosition().y;//sf::Mouse::getPosition(window).y;
			const int delta = currY - middleY;
			if (delta < -scrollThreshold || delta > scrollThreshold) {
				rot += static_cast<int>(scale * ((float)delta / (float)middleY));
				if (rot > rotBound) rot = rotBound;
				if (rot < 0.f) rot = 0.f;
			}
		}
		lastTime = thisTime;
	}

	return rot;
}

// TODO : this is ugly as hell, make it cleaner 
float getCameraRotationY()
{
	static const int middleX = window_width / 2;
	static const int scrollThreshold = 20;
	static const float timeThreshold = 0.05f;
	static const float rotBound      = 360.f;
	static const float scale         = 10.f;
	static const sf::Clock clock;

	static float rot = 0.f;
	static float lastTime = clock.getElapsedTime().asSeconds();

	float thisTime = clock.getElapsedTime().asSeconds();
	float delta    = thisTime - lastTime;
	if (delta > timeThreshold) {
		if (Application::request().isRightMouseDown()) {
			const int currX = Application::request().getMousePosition().x;//sf::Mouse::getPosition(window).x;
			const int delta = currX - middleX;
			if (delta < -scrollThreshold || delta > scrollThreshold) {
				rot += static_cast<int>(scale * ((float)delta / (float)middleX));
				if (rot > rotBound) rot = 0;
			}
		}
		lastTime = thisTime;
	}

	return rot;
}
