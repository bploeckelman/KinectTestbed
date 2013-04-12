#include "Skeleton.h"
#include "Performance.h"
#include "Util/RenderUtils.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SFML/OpenGL.hpp>
#include <SFML/System/Vector3.hpp>

#include <iostream>
#include <fstream>


Skeleton::Skeleton()
	: visibleJointFrame(nullptr)
	, liveJointFrame()
	, loaded(false)
	, useMaterials(true)
	, frameIndex(0)
	, quadric(gluNewQuadric())
	, renderingFlags(R_JOINTS | R_BONES)
	, filteringLevel(MEDIUM)
{
	visibleJointFrame = &liveJointFrame;
}


Skeleton::~Skeleton()
{
	gluDeleteQuadric(quadric);
}

void Skeleton::render() //const
{
	if (visibleJointFrame == nullptr) return;
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	visibleJointFrame = &liveJointFrame;
	glPushMatrix();
	glTranslatef(0, 1, -1);
		if (renderingFlags & R_JOINTS) renderJoints();
		if (renderingFlags & R_ORIENT) renderOrientations();
		if (renderingFlags & R_BONES)  renderBones();
		if (renderingFlags & R_PATH)   renderJointPaths();
	glPopMatrix();
	glColor4f(1,1,1,1);

	if (performance.isLoaded()) {
		visibleJointFrame = &performance.getCurrentFrame();
		glPushMatrix();
		glTranslatef(0, 1, -1);
			if (renderingFlags & R_JOINTS) renderJoints();
			if (renderingFlags & R_ORIENT) renderOrientations();
			if (renderingFlags & R_BONES)  renderBones();
			if (renderingFlags & R_PATH)   renderJointPaths();
		glPopMatrix();
		glColor4f(1,1,1,1);

		// Draw last three frames
		const unsigned int currentFrameIndex = performance.getCurrentFrameIndex();
		const unsigned int numFrames = 5;
		const unsigned int lastFrameIndex = currentFrameIndex - numFrames;
		if (currentFrameIndex > numFrames) {
			useMaterials = false;
			for (unsigned int i = currentFrameIndex; i >= lastFrameIndex; --i) {
				visibleJointFrame = &performance.getFrame(i);
				const float alpha = 1.f - ((currentFrameIndex - i) / (float) numFrames);
				glColor4f(1,1,1, alpha);
				glPushMatrix();
				glTranslatef(0, 1, -1);
					if (renderingFlags & R_JOINTS) renderJoints();
					if (renderingFlags & R_ORIENT) renderOrientations();
					if (renderingFlags & R_BONES)  renderBones();
					if (renderingFlags & R_PATH)   renderJointPaths();
				glPopMatrix();
			}
			useMaterials = true;
		}
		glDisable(GL_BLEND);
		glColor4f(1,1,1,1);
	}

}


void Skeleton::nextFrame()
{
	performance.moveToNextFrame();
}

void Skeleton::prevFrame()
{
	performance.moveToPrevFrame();
}

void Skeleton::applyPerformance( AnimationFrames& newFrames )
{
	std::cout << "Warning: applying new performance not yet implemented." << std::endl;
	//if (jointFrames.size() != newFrames.size()) {
	//	std::cerr << "Warning: unable to apply performance - number of new frames doesn't match number of existing frames" << std::endl;
	//	return;
	//}
	AnimationFrames& jointFrames = performance.getFrames();

	Joint& initialCurrentHand = jointFrames.front()[HAND_LEFT];
	Joint& initialNewHand = newFrames.front()[HAND_LEFT];

	// TODO : Move to Performance
	// For each animation frame
	//unsigned int i = 0;
	//for (auto& currentFrame : jointFrames) {
	//	// Get the corresponding performance frame
	//	JointFrame& newFrame = newFrames[i++];

	//	// Just update the hand for now
	//	Joint& currentHand = currentFrame[HAND_LEFT];
	//	Joint& newHand = newFrame[HAND_LEFT];

	//	// New position = initial position
	//	// Y'(t) = Y0 + K(t) * C'(X(t) - X0)
	//	// TODO : calculate K(t) and C'(X(t) - X0)
	//	//std::cout << "Initial pos(" << i << ") = (" << currentHand.position.x << "," << currentHand.position.y << "," << currentHand.position.z << "   ";
	//	currentHand.position = glm::vec3(
	//		initialCurrentHand.position.x + (newHand.position.x - initialNewHand.position.x),
	//		initialCurrentHand.position.y + (newHand.position.x - initialNewHand.position.y),
	//		initialCurrentHand.position.z + (newHand.position.x - initialNewHand.position.z));
	//	//std::cout << "Updated pos(" << i << ") = (" << currentHand.position.x << "," << currentHand.position.y << "," << currentHand.position.z << std::endl;
	//}
}

void Skeleton::setFrameIndex( const float fraction )
{
	if (!performance.isLoaded()) return;
	assert(fraction >= 0.f && fraction <= 1.f);

	frameIndex = static_cast<int>(floor(fraction * performance.getNumFrames()));
	performance.setCurrentFrameIndex(frameIndex);
	performance.moveToFrame(frameIndex);
	visibleJointFrame = &performance.getFrames()[frameIndex];
}

void Skeleton::renderJoints() const
{
	// Sphere parameters for joint primitive
	static const double minRadius = 0.01;
	static const double maxRadius = 0.03;
	static double radius = maxRadius;
	static const int slices = 10;
	static const int stacks = 10;

	// TODO : move these out to the constants namespace
	static const GLfloat diffuseRed[]   = { 1.0f, 0.0f, 0.0f, 1.0f };
	static const GLfloat diffuseGreen[] = { 0.0f, 1.0f, 0.0f, 1.0f };
	static const GLfloat diffuseWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat diffuseRed2[]   = { 0.5f, 0.2f, 0.0f, 1.0f };
	static const GLfloat diffuseGreen2[] = { 0.2f, 0.5f, 0.0f, 1.0f };
	static const GLfloat diffuseWhite2[] = { 0.5f, 0.5f, 0.5f, 1.0f };

	gluQuadricOrientation(quadric, GLU_OUTSIDE);
	JointFrame& joints = *visibleJointFrame;

	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		const Joint& joint = joints[(EJointType) i];

		// Change rendering size/material based on tracking type 
		switch (joint.trackingState) {
			// Skip untracked joints
			case NOT_TRACKED: continue;
			case TRACKED:
				radius = maxRadius;
				if (!useMaterials) break;
				if (visibleJointFrame == &liveJointFrame) 
					glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGreen);
				else
					glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGreen2);
			break;
			case INFERRED:
				if (renderingFlags & R_INFER) {
					radius = minRadius;
					if (!useMaterials) break;
					if (visibleJointFrame == &liveJointFrame) 
						glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed);
					else
						glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed2);
				} else {
					continue;
				}
			break;
		}

		glPushMatrix();
		glTranslatef(joint.position.x, joint.position.y, joint.position.z);
			gluSphere(quadric, radius, slices, stacks);
		glPopMatrix();
	}

	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseWhite);
}

void Skeleton::renderOrientations() const
{
	JointFrame& joints = *visibleJointFrame;

	glPointSize(1.f);
	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		const Joint& joint = joints[(EJointType) i];

		float scale = 0.075f;
		switch (joint.trackingState) { // skip untracked joints
			case NOT_TRACKED: continue;
			case INFERRED:
				if (renderingFlags & R_INFER) scale = 0.05f;
				else                          continue; 
			break;
		}

		const glm::mat4& m = joint.orientation;
		const glm::vec3 x(m[0][0], m[0][1], m[0][2]);
		const glm::vec3 y(m[1][0], m[1][1], m[1][2]);
		const glm::vec3 z(m[2][0], m[2][1], m[2][2]);
		Render::basis(scale, joint.position, glm::normalize(x), glm::normalize(y), glm::normalize(z));
	}
}

void Skeleton::renderBone( EJointType fromType, EJointType toType ) const
{
	// Cylinder parameters for bone primitive
	static const double minRadius = 0.02;
	static const double maxRadius = 0.04;
	static double baseRadius = maxRadius;
	static double topRadius  = minRadius;
	static const int slices  = 16;
	static const int stacks  = 8;

	// Material params
	static const GLfloat diffuseRed[]   = { 1.0f, 0.0f, 0.0f, 1.0f };
	static const GLfloat diffuseGood[]  = { 1.0f, 0.85f, 0.73f, 1.0f };
	static const GLfloat diffuseWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat diffuseRed2[]   = { 0.5f, 0.0f, 0.0f, 1.0f };
	static const GLfloat diffuseGood2[]  = { 0.5f, 0.25f, 0.13f, 1.0f };
	static const GLfloat diffuseWhite2[] = { 0.5f, 0.5f, 0.5f, 1.0f };

	JointFrame& joints = *visibleJointFrame;
	const Joint& fromJoint = joints[fromType];
	const Joint& toJoint   = joints[toType];

	const ETrackingState fromState = fromJoint.trackingState;
	const ETrackingState toState   = toJoint.trackingState;
	if (fromState == NOT_TRACKED || toState == NOT_TRACKED) {
		return; // nothing to draw, one joint not tracked
	}
	if (fromState == INFERRED && toState == INFERRED) {
		return; // nothing to draw, both joints inferred
	}

	// Draw thin red lines if one joint is inferred
	if (fromState == INFERRED || toState == INFERRED) {
		baseRadius = minRadius;
		topRadius  = minRadius;
		if (useMaterials) {
			if (visibleJointFrame == &liveJointFrame)
				glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed);
			else
				glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed2);
		}
	}
	// Draw thick green lines if both joints are tracked
	else if (fromState == TRACKED && toState == TRACKED) {
		baseRadius = maxRadius;
		topRadius  = maxRadius;
		if (useMaterials) {
			if (visibleJointFrame == &liveJointFrame)
				glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGood);
			else
				glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGood2);
		}
	}

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
		gluDisk(quadric, 0.0, topRadius, slices, 1);
	glPopMatrix();
	// Draw the cylinder and the other end cap
	glPushMatrix();
	glMultMatrixf(glm::value_ptr(mat));
		gluCylinder(quadric, baseRadius, topRadius, boneLength, slices, stacks);
	gluQuadricOrientation(quadric, GLU_INSIDE);
		gluDisk(quadric, 0.0, topRadius, slices, 1);
	glPopMatrix();

	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseWhite);
}

void Skeleton::renderBones() const
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

// TODO - update this for greater flexibility, number of historical frames to draw, fade out, etc...
void Skeleton::renderJointPath( const EJointType type ) const
{
	if (!loaded) return;

	const AnimationFrames& jointFrames = performance.getFrames();
	const unsigned int numFrames = 20;
	const unsigned int lastFrame = ((frameIndex - numFrames) < 0) ? 0 : (frameIndex - numFrames);

	glDisable(GL_LIGHTING);

	glColor3f(1,1,0);
	glPushMatrix();
	glBegin(GL_LINE_STRIP);
		for (auto i = lastFrame; i <= frameIndex; ++i) {
			glVertex3fv(glm::value_ptr(jointFrames[i].at(type).position));
		}
	glEnd();
	glPopMatrix();
	glColor3f(1,1,1);

	glEnable(GL_LIGHTING);
}

void Skeleton::renderJointPaths() const
{
	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		renderJointPath((EJointType) i);
	}
}
