#include "Skeleton.h"
#include "Util/RenderUtils.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SFML/OpenGL.hpp>


Skeleton::Skeleton()
	: visibleJointFrame(nullptr)
	, currentJointFrame()
	, jointFrames()
	, renderingFlags(R_POS | R_JOINTS | R_ORIENT | R_BONES)
	, filteringLevel(MEDIUM)
{
	visibleJointFrame = &currentJointFrame;
}


Skeleton::~Skeleton()
{
	// TODO
}

void Skeleton::render() const
{
	if (visibleJointFrame == nullptr) return;

	glPushMatrix();
	glTranslatef(0, 1, -2); 
		if (renderingFlags & R_POS)    renderSkeletonPosition();
		if (renderingFlags & R_JOINTS) renderJoints();
		if (renderingFlags & R_ORIENT) renderOrientations();
		if (renderingFlags & R_BONES)  renderBones();
		if (renderingFlags & R_PATH)   renderJointPaths();
	glPopMatrix();
	glColor3f(1,1,1);
}

void Skeleton::renderSkeletonPosition() const
{
	JointFrame& joints = *visibleJointFrame;
	const Joint& hipCenter = joints[HIP_CENTER];

	glPointSize(20.f);
	glColor3f(1,0,1);
	glBegin(GL_POINTS);
		glVertex3fv(glm::value_ptr(hipCenter.position));
	glEnd();
	glColor3f(1,1,1);
	glPointSize(1.f);
}

void Skeleton::renderJoints() const
{
	JointFrame& joints = *visibleJointFrame;

	glPointSize(8.f);
	glBegin(GL_POINTS);
	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		const Joint& joint = joints[(EJointType) i];
		switch (joint.trackingState) { // skip untracked joints
			case NOT_TRACKED: continue;
			case TRACKED:     glColor3f(1.f, 1.f, 1.f); break;
			case INFERRED:
				if (renderingFlags & R_INFER) glColor3f(1.f, 0.5f, 0.f);
				else                          continue;
			break;
		}
		glVertex3fv(glm::value_ptr(joint.position));
	}
	glEnd();
	glPointSize(1.f);
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
	JointFrame& joints = *visibleJointFrame;
	const Joint& fromJoint = joints[fromType];
	const Joint& toJoint   = joints[toType];

	const ETrackingState fromState = fromJoint.trackingState;
	const ETrackingState toState   = toJoint.trackingState;
	if (fromState == NOT_TRACKED || toState == NOT_TRACKED) {
		return; // nothing to draw, one joint not tracked
	}
	if (fromState == INFERRED && toState == INFERRED) {
		return; // nothing to draw, points inferred
	}

	const glm::vec3& fromPosition = fromJoint.position;
	const glm::vec3& toPosition   = toJoint.position;

	// Draw thin red lines if one joint is inferred
	if (fromState == INFERRED || toState == INFERRED) {
		glLineWidth(1.f);
		glColor3f(1.f, 0.f, 0.f);
		return;
	}
	// Draw thick green lines if both joints are tracked
	else if (fromState == TRACKED && toState == TRACKED) {
		glLineWidth(8.f);
		glColor3f(0.f, 1.f, 0.f);
	}

	glBegin(GL_LINES);
		glVertex3fv(glm::value_ptr(fromPosition));
		glVertex3fv(glm::value_ptr(toPosition));
	glEnd();

	glColor3f(1.f, 1.f, 1.f);
	glLineWidth(1.f);
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

void Skeleton::renderJointPath( EJointType type ) const
{
	// TODO - update this for greater flexibilty
	/*
	if (!loaded) return;

	glPushMatrix();

	// TODO: draw sequence of frames for a particular joint or subset of joints
	// TODO: draw all frames for a particular joint or subset of joints    

	glColor3f(1.f, 1.f, 0.f);
	//glBegin(GL_LINE_STRIP);
	//    for (int i = 0; i <= jointFrameIndex; ++i) {
	//        auto& hip_center = jointPositionFrames[i][NUI_SKELETON_POSITION_HIP_CENTER];
	//        glVertex3fv(glm::value_ptr(hip_center.position));
	//    }
	//glEnd();
	glBegin(GL_LINE_STRIP);
		for (int i = 0; i <= jointFrameIndex; ++i) {
			auto& left_hand  = jointPositionFrames[i][NUI_SKELETON_POSITION_HAND_LEFT];
			glVertex3fv(glm::value_ptr(left_hand.position));
		}
	glEnd();
	glBegin(GL_LINE_STRIP);
		for (int i = 0; i < jointFrameIndex; ++i) {
			auto& right_hand = jointPositionFrames[i][NUI_SKELETON_POSITION_HAND_RIGHT];
			glVertex3fv(glm::value_ptr(right_hand.position));
		}
	glEnd();

	glPopMatrix();
	*/
}

void Skeleton::renderJointPaths() const
{
	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		renderJointPath((EJointType) i);
	}
}

void Skeleton::nextFrame()
{
	// TODO - get this working
	/*
	if (!loaded) return;
	
	const int nextFrame = jointFrameIndex + 1;
	const int numFrames = jointPositionFrames.size();
	if (nextFrame >= 0 && nextFrame < numFrames) {
		jointFrameIndex = nextFrame;
		jointFrameVis   = &jointPositionFrames[jointFrameIndex];
		gui.setProgress((float) jointFrameIndex / (float) (numFrames - 1));
		gui.setIndex(jointFrameIndex);
	}
	*/
}

void Skeleton::prevFrame()
{
	// TODO - get this working
	/*
	if (!loaded) return;

	const int prevFrame = jointFrameIndex - 1;
	const int numFrames = jointPositionFrames.size();
	if (prevFrame >= 0 && prevFrame < numFrames) {
		jointFrameIndex = prevFrame;
		jointFrameVis   = &jointPositionFrames[jointFrameIndex];
		gui.setProgress((float) jointFrameIndex / (float) (numFrames - 1));
		gui.setIndex(jointFrameIndex);
	}
	*/
}
