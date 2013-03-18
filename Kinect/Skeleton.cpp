#include "Skeleton.h"
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
	, currentJointFrame()
	, jointFrames()
	, loaded(false)
	, frameIndex(0)
	, quadric(gluNewQuadric())
	, renderingFlags(R_JOINTS | R_ORIENT | R_BONES)
	, filteringLevel(MEDIUM)
{
	visibleJointFrame = &currentJointFrame;
}


Skeleton::~Skeleton()
{
	gluDeleteQuadric(quadric);
}

void Skeleton::render() const
{
	if (visibleJointFrame == nullptr) return;

	glPushMatrix();
	glTranslatef(0, 1, -1);
		if (renderingFlags & R_JOINTS) renderJoints();
		if (renderingFlags & R_ORIENT) renderOrientations();
		if (renderingFlags & R_BONES)  renderBones();
		if (renderingFlags & R_PATH)   renderJointPaths();
	glPopMatrix();
	glColor3f(1,1,1);
}

bool Skeleton::loadFile( const std::string& filename )
{
	if (loaded) {
		jointFrames.clear();
		frameIndex = 0;
		loaded = false;
	}

	sf::Vector3f mn( 1e30f,  1e30f,  1e30f);
	sf::Vector3f mx(-1e30f, -1e30f, -1e30f);

	std::ifstream loadStream;
	loadStream.open(filename, std::ios::binary | std::ios::in);
	if (loadStream.is_open()) {
		std::cout << "Opened file: " << filename.c_str() << std::endl
				  << "Loading joints positions..." << std::endl;

		Skeleton::JointFrame inputJointFrame;    
		Skeleton::Joint joint;
		int numJointsRead = 0, totalJointsRead = 0, totalFramesRead = 0;
		while (loadStream.good()) {
			memset(&joint, 0, sizeof(Skeleton::Joint));
			loadStream.read((char *)&joint, sizeof(Skeleton::Joint));
			inputJointFrame[joint.type] = joint;
			++totalJointsRead;
			
			mn.x = std::min(mn.x, joint.position.x);
			mn.y = std::min(mn.y, joint.position.y);
			mn.z = std::min(mn.z, joint.position.z);

			mx.x = std::max(mx.x, joint.position.x);
			mx.y = std::max(mx.y, joint.position.y);
			mx.z = std::max(mx.z, joint.position.z);

			// Done reading joints for current frame, save it and continue with next frame 
			if (++numJointsRead == NUM_JOINT_TYPES) {
				jointFrames.push_back(inputJointFrame);
				numJointsRead = 0; 
				++totalFramesRead;
			}
		}

		loadStream.close();
		loaded = true;
		std::cout << "Loaded " << totalJointsRead << " joints in " << totalFramesRead << " frames." << std::endl
				  << "Done loading skeleton data from '" << filename.c_str() << "'." << std::endl;
	}

	std::cout << "min,max = (" << mn.x << "," << mn.y << "," << mn.z << ")"
			  <<        " , (" << mx.x << "," << mx.y << "," << mx.z << ")"
			  << std::endl;

	// Normalize the z values for each joint in each frame
	for (auto frame : jointFrames) {
		for (auto joints : frame) {
			Skeleton::Joint& joint = joints.second;
			joint.position.z /= mx.z;
		}
	}

	frameIndex = 0;
	if (loaded) {
		visibleJointFrame = &jointFrames[frameIndex];
	} else {
		visibleJointFrame = &currentJointFrame;
	}

	return loaded;
}

void Skeleton::clearLoadedFrames()
{
	if (!loaded) return;
	frameIndex = 0;
	jointFrames.clear();
	visibleJointFrame = &currentJointFrame;
}

void Skeleton::nextFrame()
{
	if (!loaded) return;
	
	const int nextFrame = frameIndex + 1;
	const int numFrames = jointFrames.size();
	if (nextFrame >= 0 && nextFrame < numFrames) {
		frameIndex = nextFrame;
		visibleJointFrame = &jointFrames[frameIndex];
	}
}

void Skeleton::prevFrame()
{
	if (!loaded) return;

	const int prevFrame = frameIndex - 1;
	const int numFrames = jointFrames.size();
	if (prevFrame >= 0 && prevFrame < numFrames) {
		frameIndex = prevFrame;
		visibleJointFrame = &jointFrames[frameIndex];
	}
}

void Skeleton::setFrameIndex( const float fraction )
{
	if (!loaded) return;
	assert(fraction >= 0.f && fraction <= 1.f);

	frameIndex = static_cast<int>(floor(fraction * jointFrames.size()));
	visibleJointFrame = &jointFrames[frameIndex];
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

	JointFrame& joints = *visibleJointFrame;

	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		const Joint& joint = joints[(EJointType) i];

		// Change rendering size/material based on tracking type 
		switch (joint.trackingState) {
			// Skip untracked joints
			case NOT_TRACKED: continue;
			case TRACKED:
				radius = maxRadius;
				glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGreen);
			break;
			case INFERRED:
				if (renderingFlags & R_INFER) {
					radius = minRadius;
					glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed);
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
		glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed);
	}
	// Draw thick green lines if both joints are tracked
	else if (fromState == TRACKED && toState == TRACKED) {
		baseRadius = maxRadius;
		topRadius  = maxRadius;
		glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGood);
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
