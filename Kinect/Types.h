#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <map>


enum EJointType {
	HIP_CENTER      = 0,
	SPINE           = (HIP_CENTER      + 1),
	SHOULDER_CENTER = (SPINE           + 1),
	HEAD            = (SHOULDER_CENTER + 1),
	SHOULDER_LEFT   = (HEAD            + 1),
	ELBOW_LEFT      = (SHOULDER_LEFT   + 1),
	WRIST_LEFT      = (ELBOW_LEFT      + 1),
	HAND_LEFT       = (WRIST_LEFT      + 1),
	SHOULDER_RIGHT  = (HAND_LEFT       + 1),
	ELBOW_RIGHT     = (SHOULDER_RIGHT  + 1),
	WRIST_RIGHT     = (ELBOW_RIGHT     + 1),
	HAND_RIGHT      = (WRIST_RIGHT     + 1),
	HIP_LEFT        = (HAND_RIGHT      + 1),
	KNEE_LEFT       = (HIP_LEFT        + 1),
	ANKLE_LEFT      = (KNEE_LEFT       + 1),
	FOOT_LEFT       = (ANKLE_LEFT      + 1),
	HIP_RIGHT       = (FOOT_LEFT       + 1),
	KNEE_RIGHT      = (HIP_RIGHT       + 1),
	ANKLE_RIGHT     = (KNEE_RIGHT      + 1),
	FOOT_RIGHT      = (ANKLE_RIGHT     + 1),
	NUM_JOINT_TYPES = (FOOT_RIGHT      + 1)
};

enum ETrackingState {
	NOT_TRACKED = 0,
	INFERRED    = (NOT_TRACKED + 1),
	TRACKED     = (INFERRED    + 1)
};

enum EFilteringLevel {
	OFF    = 0,
	LOW    = (OFF    + 1),
	MEDIUM = (LOW    + 1),
	HIGH   = (MEDIUM + 1)
};

typedef struct tag_joint {
	glm::vec3 position;
	glm::mat4 orientation;

	EJointType type;
	ETrackingState trackingState;
} Joint;

typedef struct tag_joint_frame {
	float     timestamp; // in seconds
	std::map<EJointType, Joint> joints;
} JointFrame;
typedef std::vector<JointFrame>     AnimationFrames;

// Rendering flags == [R_JOINTS | R_ORIENT | R_BONES | R_INFER]
// ------------------------------------------------------------
// R_JOINTS = skeleton joints
// R_ORIENT = skeleton joint orientations
// R_BONES  = connections between skeleton joints
// R_INFER  = draw inferred joints/bones
// R_PATH   = draw path of particular joints over time
static const unsigned char R_JOINTS = 0x01;
static const unsigned char R_INFER  = 0x02;
static const unsigned char R_ORIENT = 0x04;
static const unsigned char R_BONES  = 0x08;
static const unsigned char R_PATH   = 0x10;
typedef unsigned char RenderingFlags;