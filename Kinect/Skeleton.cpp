#include "Skeleton.h"
#include "Performance.h"
#include "Util/RenderUtils.h"
#include "Core/Application.h"

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
	, performance(nullptr)
	, loaded(false)
	, useMaterials(true)
	, alpha(1.f)
	, frameIndex(0)
	, quadric(gluNewQuadric())
	, renderingFlags(R_JOINTS | R_BONES)
	, filteringLevel(MEDIUM)
{
	visibleJointFrame = &liveJointFrame;
	performances.push_back(Performance("Live"));
	performance = &performances.back();
}


Skeleton::~Skeleton()
{
	gluDeleteQuadric(quadric);
}

void Skeleton::render() //const
{
	if (visibleJointFrame == nullptr) return;

	if (performance == nullptr) return;

	UserInterface& gui = Application::request().getGUI();
	const std::string& selected = gui.getPerformancesCombo()->GetSelectedText();

	if (selected == "Live") {
		// Draw the current live joint frame
		visibleJointFrame = &liveJointFrame;
		glPushMatrix();
		glTranslatef(0, 1, -1);
			if (renderingFlags & R_JOINTS) renderJoints();
			if (renderingFlags & R_ORIENT) renderOrientations();
			if (renderingFlags & R_BONES)  renderBones();
			if (renderingFlags & R_PATH)   renderJointPaths();
		glPopMatrix();
		glColor4f(1,1,1,1);
		return;
	}

	// Set the current performance
	bool found = false;
	for (auto& p : performances) {
		if (selected == p.getName()) {
			performance = &p;
			found = true;
			break;
		}
	}
	if (!found) {
		performance = nullptr;
	}

	// Draw the current performance joint frame
	if (performance != nullptr && performance->isLoaded()) {
		// Draw last several frames
		const unsigned int currentFrameIndex = performance->getCurrentFrameIndex();
		unsigned int numFrames = 20;
		const int deltaFrames = currentFrameIndex - numFrames;
		unsigned int lastFrameIndex;
		if (deltaFrames < 0) {
			lastFrameIndex = 0;
			numFrames = currentFrameIndex + 1;
		} else {
			lastFrameIndex = deltaFrames;
		}

		// Draw previous frames up to 'numFrames' blurred
		useMaterials = false;
		glDisable(GL_LIGHTING);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for (unsigned int i = currentFrameIndex, count = 1; i >= 0 && count <= numFrames; --i, ++count) {
			visibleJointFrame = &performance->getFrame(i);
			alpha = 1.f - ((currentFrameIndex - i) / (float) numFrames);
			glColor4f(alpha, alpha, 0, alpha);
			glPushMatrix();
			glTranslatef(0, 1, -1);
				if (renderingFlags & R_JOINTS) renderJoints();
				if (renderingFlags & R_ORIENT) { 
					renderOrientations();
					glColor4f(alpha, alpha, 0, alpha);
				}
				if (renderingFlags & R_BONES)  renderBones();
				if (renderingFlags & R_PATH)   { glDisable(GL_BLEND); renderJointPaths(); glEnable(GL_BLEND); }
			glPopMatrix();
		}
		glColor4f(1,1,1,1);
		glEnable(GL_LIGHTING);
		glDisable(GL_BLEND);
		useMaterials = true;

		// Draw current frame unblurred and lit
		visibleJointFrame = &performance->getCurrentFrame();
		glPushMatrix();
		glTranslatef(0, 1, -1);
			if (renderingFlags & R_JOINTS) renderJoints();
			if (renderingFlags & R_ORIENT) renderOrientations();
			if (renderingFlags & R_BONES)  renderBones();
			if (renderingFlags & R_PATH)   renderJointPaths();
		glPopMatrix();
		glColor4f(1,1,1,1);
	}
}


void Skeleton::nextFrame()
{
	performance->moveToNextFrame();
}

void Skeleton::prevFrame()
{
	performance->moveToPrevFrame();
}

void Skeleton::addPerformance( const Performance& newPerformance )
{
	if (!newPerformance.isLoaded()) {
		std::cerr << "Warning: attempted to add unloaded performance to skeleton; ignored" << std::endl;
		return;
	}

	performances.push_back(newPerformance);
	Application::request().getGUI().addPerformance(newPerformance.getName());
	Application::request().getGUI().getPerformancesCombo()->SelectItem(performances.size());
	performance = &performances.back();
}

// TODO : change to (const Performance& performance) + overloaded to index from performances vector
void Skeleton::applyPerformance( Performance& newPerformance )
{
	std::cout << "Warning: applying new performance not yet implemented." << std::endl;
	//if (jointFrames.size() != newFrames.size()) {
	//	std::cerr << "Warning: unable to apply performance - number of new frames doesn't match number of existing frames" << std::endl;
	//	return;
	//}
	// HACK : need to have a way to specify which other performance to apply this one to
	// NOTE : could have non-member function that takes 2 performance&'s and applies one to the other in place
	auto p = performances.begin();
	std::advance(p, 1); // 0 = live, 1 = base performance
	Performance& currentPerformance = *p;//*performance;
	AnimationFrames& currentFrames = currentPerformance.getFrames();
	AnimationFrames& newFrames = newPerformance.getFrames();
	Joint& initialCurrentHand = currentFrames[0].joints[HAND_LEFT];
	Joint& initialNewHand = newFrames[0].joints[HAND_LEFT];
	Joint& initialCurrentWrist = currentFrames[0].joints[WRIST_LEFT];
	Joint& initialNewWrist = newFrames[0].joints[WRIST_LEFT];
	Joint& initialCurrentElbow = currentFrames[0].joints[ELBOW_LEFT];
	Joint& initialNewElbow = newFrames[0].joints[ELBOW_LEFT];
	Joint& initialCurrentShoulder = currentFrames[0].joints[SHOULDER_LEFT];
	Joint& initialNewShoulder = newFrames[0].joints[SHOULDER_LEFT];

	const glm::vec3& y0pos(initialCurrentHand.position);
	const glm::vec3& x0pos(initialNewHand.position);
	std::cout << "Applying new performance..." << std::endl
		      << "Y(0)[hand_left] : pos(" << y0pos.x << "," << y0pos.y << "," << y0pos.z << ")" << std::endl
		      << "X(0)[hand_left] : pos(" << x0pos.x << "," << x0pos.y << "," << x0pos.z << ")" << std::endl;
	// TODO : Move to Performance
	// For each animation frame
	unsigned int i = 0;
	for (unsigned int i = 1; i < currentFrames.size(); ++i) {
		// Get the corresponding pair of frames
		JointFrame& currFrame = currentFrames[i];
		JointFrame& newFrame = newFrames[i];
		JointFrame& prevNewFrame = newFrames[i - 1];

		// Just update the hand for now
		Joint& currentHand = currFrame.joints[HAND_LEFT];
		Joint& newHand = newFrame.joints[HAND_LEFT];
		Joint& prevNewHand = prevNewFrame.joints[HAND_LEFT];

		Joint& currentWrist = currFrame.joints[WRIST_LEFT];
		Joint& newWrist = newFrame.joints[WRIST_LEFT];
		Joint& prevNewWrist = prevNewFrame.joints[WRIST_LEFT];

		Joint& currentElbow = currFrame.joints[ELBOW_LEFT];
		Joint& newElbow = newFrame.joints[ELBOW_LEFT];
		Joint& prevNewElbow = prevNewFrame.joints[ELBOW_LEFT];

		Joint& currentShoulder = currFrame.joints[SHOULDER_LEFT];
		Joint& newShoulder = newFrame.joints[SHOULDER_LEFT];
		Joint& prevNewShoulder = prevNewFrame.joints[SHOULDER_LEFT];

		// Absolute Mapping
		// Y'(t) = Y0 + C'(X(t) - X0)
		// Yr'(t) = C' * Xr(t) * inv(Xr0) * Yr0

		// Additive Mapping
		// Y'(t) = Y0 + C'(X(t) - X(t - dt))
		glm::vec3 t = newHand.position - prevNewHand.position;
		glm::vec4 xtSubxtdt(t.x, t.y, t.z, 1);
		glm::vec4 y0(y0pos.x, y0pos.y, y0pos.z, 1);
		glm::mat4 c(1);

		glm::vec4 pos = y0 + c * xtSubxtdt;
		currentHand.position = glm::vec3(pos.x, pos.y, pos.z);

		t = newWrist.position - prevNewWrist.position;
		xtSubxtdt = glm::vec4(t.x, t.y, t.z, 1);
		y0 = glm::vec4(initialCurrentWrist.position.x, initialCurrentWrist.position.y, initialCurrentWrist.position.z, 1);
		pos = y0 + c * xtSubxtdt;
		currentWrist.position = glm::vec3(pos.x, pos.y, pos.z);

		t = newElbow.position - prevNewElbow.position;
		xtSubxtdt = glm::vec4(t.x, t.y, t.z, 1);
		y0 = glm::vec4(initialCurrentElbow.position.x, initialCurrentElbow.position.y, initialCurrentElbow.position.z, 1);
		pos = y0 + c * xtSubxtdt;
		currentElbow.position = glm::vec3(pos.x, pos.y, pos.z);

		t = newShoulder.position - prevNewShoulder.position;
		xtSubxtdt = glm::vec4(t.x, t.y, t.z, 1);
		y0 = glm::vec4(initialCurrentShoulder.position.x, initialCurrentShoulder.position.y, initialCurrentShoulder.position.z, 1);
		pos = y0 + c * xtSubxtdt;
		currentShoulder.position = glm::vec3(pos.x, pos.y, pos.z);

		// Trajectory-Relative Mapping
		// Y'(t) = Y0 + K(t) * C'(X(t) - X0)
		// current_new_position = current_initial_position + rotation_hierarchy * inv_camera@(current_new_position - initial_new_position)
		// TODO : calculate K(t) and C'(X(t) - X0)
		//glm::mat4 k(1);
		//glm::mat4 c(1);
		//updatedPosition = y0 + (k * c * xtSubx0);
		//currentHand.position = glm::vec3(updatedPosition.x, updatedPosition.y, updatedPosition.z);
		//std::cout << "Updated pos(" << i << ") = (" << currentHand.position.x << "," << currentHand.position.y << "," << currentHand.position.z << std::endl;
	}
}

void Skeleton::setFrameIndex( const float fraction )
{
	assert(fraction >= 0.f && fraction <= 1.f);
	if (!performance->isLoaded() || performance->getFrames().empty())
		return;

	frameIndex = static_cast<int>(floor(fraction * performance->getNumFrames()));
	performance->setCurrentFrameIndex(frameIndex);
	performance->moveToFrame(frameIndex);
	visibleJointFrame = &performance->getFrames()[frameIndex];
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
	const GLfloat diffuseRed[]   = { 1.0f, 0.0f, 0.0f, alpha };
	const GLfloat diffuseGreen[] = { 0.0f, 1.0f, 0.0f, alpha };
	const GLfloat diffuseWhite[] = { 1.0f, 1.0f, 1.0f, alpha };
	const GLfloat diffuseRed2[]   = { 0.5f, 0.2f, 0.0f, alpha };
	const GLfloat diffuseGreen2[] = { 0.2f, 0.5f, 0.0f, alpha };
	const GLfloat diffuseWhite2[] = { 0.5f, 0.5f, 0.5f, alpha };

	gluQuadricOrientation(quadric, GLU_OUTSIDE);

	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		const Joint& joint = (*visibleJointFrame).joints[(EJointType) i];

		// Change rendering size/material based on tracking type 
		switch (joint.trackingState) {
			// Skip untracked joints
			case NOT_TRACKED: continue;
			case TRACKED:
				if (useMaterials) {
					radius = maxRadius;
					if (visibleJointFrame == &liveJointFrame) 
						glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGreen);
					else
						glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGreen2);
				} else {
					radius = maxRadius * 0.5f;
				}
			break;
			case INFERRED:
				if (renderingFlags & R_INFER) {
					if (useMaterials) {
						radius = minRadius;
						if (visibleJointFrame == &liveJointFrame) 
							glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed);
						else
							glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed2);
					} else {
						radius = minRadius * 0.5f;
					}
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
	glPointSize(1.f);
	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		const Joint& joint = (*visibleJointFrame).joints[(EJointType) i];

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
		Render::basis(scale, joint.position, glm::normalize(x), glm::normalize(y), glm::normalize(z), alpha);
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
	const GLfloat diffuseRed[]   = { 1.0f, 0.0f, 0.0f, alpha };
	const GLfloat diffuseGood[]  = { 1.0f, 0.85f, 0.73f, alpha };
	const GLfloat diffuseWhite[] = { 1.0f, 1.0f, 1.0f, alpha };
	const GLfloat diffuseRed2[]   = { 0.5f, 0.0f, 0.0f, alpha };
	const GLfloat diffuseGood2[]  = { 0.5f, 0.25f, 0.13f, alpha };
	const GLfloat diffuseWhite2[] = { 0.5f, 0.5f, 0.5f, alpha };

	JointFrame& frame = *visibleJointFrame;
	const Joint& fromJoint = frame.joints[fromType];
	const Joint& toJoint   = frame.joints[toType];

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
		if (useMaterials) {
			baseRadius = minRadius;
			topRadius  = minRadius;
			if (visibleJointFrame == &liveJointFrame)
				glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed);
			else
				glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseRed2);
		} else {
			baseRadius = minRadius * 0.5f;
			topRadius  = minRadius * 0.5f;
		}
	}
	// Draw thick green lines if both joints are tracked
	else if (fromState == TRACKED && toState == TRACKED) {
		if (useMaterials) {
			baseRadius = maxRadius;
			topRadius  = maxRadius;
			if (visibleJointFrame == &liveJointFrame)
				glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGood);
			else
				glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseGood2);
		} else {
			baseRadius = maxRadius * 0.5f;
			topRadius  = maxRadius * 0.5f;
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

	if (useMaterials) {
		glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseWhite);
	}
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

	const AnimationFrames& jointFrames = performance->getFrames();
	const unsigned int numFrames = 20;
	const unsigned int lastFrame = ((frameIndex - numFrames) < 0) ? 0 : (frameIndex - numFrames);

	glDisable(GL_LIGHTING);

	glColor4f(1,1,0, alpha );
	glPushMatrix();
	glBegin(GL_LINE_STRIP);
		for (auto i = lastFrame; i <= frameIndex; ++i) {
			glVertex3fv(glm::value_ptr(jointFrames[i].joints.at(type).position));
		}
	glEnd();
	glPopMatrix();
	glColor4f(1,1,1, alpha );

	glEnable(GL_LIGHTING);
}

void Skeleton::renderJointPaths() const
{
	for (auto i = 0; i < NUM_JOINT_TYPES; ++i) {
		renderJointPath((EJointType) i);
	}
}
