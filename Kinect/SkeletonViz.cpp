#pragma once
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

#include <SFML/OpenGL.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <list>
#include <map>

#include "SkeletonViz.h"
#include "Performance.h"
#include "Types.h"
#include "Core/Constants.h"
#include "Util/RenderUtils.h"

using namespace constants;

bool setJointMaterial(const ETrackingState& trackingState, RenderParams& params);
void setBoneMaterial(const ETrackingState& toTrackingState, const ETrackingState& fromTrackingState, RenderParams& params);


SkeletonViz::SkeletonViz()
	: performance(nullptr)
	, params()
	, quadric(gluNewQuadric())
{}

SkeletonViz::~SkeletonViz()
{
	gluDeleteQuadric(quadric);
}

void SkeletonViz::update( float delta )
{
	// TODO : add any updating logic here..
	// Bring currentFrameIndex and stepping logic into this class?
	// Should Performance just be a data dump of joint frames with 
	// no other functionality beyond accessing those frames?
}

void SkeletonViz::render()
{
	if (!isLoaded()) return;

	if (params.flags & R_JOINTS) renderJoints();
	if (params.flags & R_ORIENT) renderOrientations();
	if (params.flags & R_BONES)  renderBones();
	if (params.flags & R_PATH)   renderJointPaths();
}


void SkeletonViz::renderJoints()
{
	gluQuadricOrientation(quadric, GLU_OUTSIDE);

	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		const Joint& joint = performance->getCurrentFrameJoint(i);

		// Skip some joints
		if (!setJointMaterial(joint.trackingState, params)) {
			continue;
		}

		glPushMatrix();
		glTranslatef(joint.position.x, joint.position.y, joint.position.z);
			gluSphere(quadric, params.jointRadius, params.jointSlices, params.jointStacks);
		glPopMatrix();
	}

	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
}

void SkeletonViz::renderOrientations()
{
	glPointSize(1.f);
	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		const Joint& joint = performance->getCurrentFrameJoint(i);

		switch (joint.trackingState) {
			case NOT_TRACKED: continue; // skip untracked joints
			case INFERRED:
				if (!(params.flags & R_INFER)) continue;
			break;
		}

		// TODO : overload Render::basis() to take a matrix in place of basis vectors
		const glm::mat4& m = joint.orientation;
		const glm::vec3 x(m[0][0], m[0][1], m[0][2]);
		const glm::vec3 y(m[1][0], m[1][1], m[1][2]);
		const glm::vec3 z(m[2][0], m[2][1], m[2][2]);
		Render::basis(params.orientationScale, joint.position, glm::normalize(x), glm::normalize(y), glm::normalize(z), 1.f);
	}
}

void SkeletonViz::renderBone( EJointType fromType, EJointType toType )
{
	JointFrame& frame = performance->getCurrentFrame();
	const Joint& fromJoint = frame.joints[fromType];
	const Joint& toJoint   = frame.joints[toType];

	if (fromJoint.trackingState == NOT_TRACKED || toJoint.trackingState == NOT_TRACKED) {
		return; // nothing to draw, one joint not tracked
	}
	if (fromJoint.trackingState == INFERRED && toJoint.trackingState == INFERRED) {
		return; // nothing to draw, both joints inferred
	}

	setBoneMaterial(toJoint.trackingState, fromJoint.trackingState, params);

	const glm::vec3& fromPosition = fromJoint.position;
	const glm::vec3& toPosition   = toJoint.position;
	const double boneLength = glm::distance(fromPosition, toPosition);

	// Build an orientation matrix to place a cylinder between toPosition and fromPosition
	const glm::vec3 z(0,0,1);
	const glm::vec3 dir(glm::normalize(fromPosition - toPosition));
	const glm::vec3 axis = glm::cross(z, dir);
	const float angle = glm::degrees(acos(glm::dot(z, dir)));
	const glm::mat4 mat(glm::rotate(glm::translate(glm::mat4(1.0), toPosition), angle, axis));
	const glm::mat4 mat2(glm::rotate(glm::translate(glm::mat4(1.0), fromPosition), angle, axis));

	gluQuadricNormals(quadric, GLU_SMOOTH);
	gluQuadricOrientation(quadric, GLU_OUTSIDE);

	// Draw one end cap
	glPushMatrix();
	glMultMatrixf(glm::value_ptr(mat2));
		gluDisk(quadric, 0.0, params.boneRadius, params.boneSlices, 1);
	glPopMatrix();

	// Draw the cylinder and the other end cap
	glPushMatrix();
	glMultMatrixf(glm::value_ptr(mat));
		gluCylinder(quadric, params.boneRadius, params.boneRadius, boneLength, params.boneSlices, params.boneStacks);
	gluQuadricOrientation(quadric, GLU_INSIDE);
		gluDisk(quadric, 0.0, params.boneRadius, params.boneSlices, 1);
	glPopMatrix();

	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
}

void SkeletonViz::renderBones()
{
	// Render head and shoulders
	renderBone(HEAD, SHOULDER_CENTER);
	renderBone(SHOULDER_CENTER, SHOULDER_LEFT);
	renderBone(SHOULDER_CENTER, SHOULDER_RIGHT);

	// Render left arm
	renderBone(SHOULDER_LEFT, ELBOW_LEFT);
	renderBone(ELBOW_LEFT, WRIST_LEFT);
	renderBone(WRIST_LEFT, HAND_LEFT);

	// Render right arm
	renderBone(SHOULDER_RIGHT, ELBOW_RIGHT);
	renderBone(ELBOW_RIGHT, WRIST_RIGHT);
	renderBone(WRIST_RIGHT, HAND_RIGHT);

	// Render torso
	renderBone(SHOULDER_CENTER, SPINE);
	renderBone(SPINE, HIP_CENTER);
	renderBone(HIP_CENTER, HIP_RIGHT);
	renderBone(HIP_CENTER, HIP_LEFT);

	// Render left leg
	renderBone(HIP_LEFT, KNEE_LEFT);
	renderBone(KNEE_LEFT, ANKLE_LEFT);
	renderBone(ANKLE_LEFT, FOOT_LEFT);

	// Render right leg
	renderBone(HIP_RIGHT, KNEE_RIGHT);
	renderBone(KNEE_RIGHT, ANKLE_RIGHT);
	renderBone(ANKLE_RIGHT, FOOT_RIGHT);
}

// TODO : fix this, its currently broken
void SkeletonViz::renderJointPath( const EJointType type )
{
	const AnimationFrames& jointFrames = performance->getFrames();
	const unsigned int currentFrameIndex = performance->getCurrentFrameIndex();
	const unsigned int numFrames = 20;
	const unsigned int lastFrame = ((currentFrameIndex - numFrames) < 0) ? 0 : (currentFrameIndex - numFrames);

	glDisable(GL_LIGHTING);

	// TODO: alpha
	glColor4f(1, 1, 0, 1);
	glPushMatrix();
	glBegin(GL_LINE_STRIP);
		for (auto i = lastFrame; i <= currentFrameIndex; ++i) {
			glVertex3fv(glm::value_ptr(jointFrames[i].joints.at(type).position));
		}
	glEnd();
	glPopMatrix();
	glColor4f(1, 1, 1, 1);

	glEnable(GL_LIGHTING);
}

void SkeletonViz::renderJointPaths()
{
	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		renderJointPath((EJointType) i);
	}
}

bool setJointMaterial(const ETrackingState& trackingState, RenderParams& params)
{
	switch (trackingState) {
		case NOT_TRACKED: return false; // skip untracked joints
		case TRACKED:
			params.jointRadius = params.jointMaxRadius;
			glMaterialfv(GL_FRONT, GL_DIFFUSE, params.materials.jointTracked);
		break;
		case INFERRED:
			if (params.flags & R_INFER) {
				params.jointRadius = params.jointMinRadius;
				glMaterialfv(GL_FRONT, GL_DIFFUSE, params.materials.jointInferred);
			} else {
				// Skip inferred joints if we shouldn't render them
				return false;
			}
		break;
	}
	return true;
}

void setBoneMaterial(const ETrackingState& toTrackingState, const ETrackingState& fromTrackingState, RenderParams& params)
{
	// Draw thin red lines if one joint is inferred
	if (fromTrackingState == INFERRED || toTrackingState == INFERRED) {
		params.boneRadius = params.boneMinRadius;
		glMaterialfv(GL_FRONT, GL_DIFFUSE, params.materials.boneInferred);
	}
	// Draw thick green lines if both joints are tracked
	else if (fromTrackingState == TRACKED && toTrackingState == TRACKED) {
		params.boneRadius = params.boneMaxRadius;
		glMaterialfv(GL_FRONT, GL_DIFFUSE, params.materials.boneTracked);
	}
}
