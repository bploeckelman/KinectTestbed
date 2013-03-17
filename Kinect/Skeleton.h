#pragma once
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

#include <glm/glm.hpp>

#include <vector>
#include <map>


class Skeleton
{
// A bunch of helper data enums and types -----------------
public:
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
		float     timestamp; // in seconds

		glm::vec3 position;
		glm::mat4 orientation;

		EJointType type;
		ETrackingState trackingState;
	} Joint;

	typedef std::map<EJointType, Joint> JointFrame;
	typedef std::vector<JointFrame>     JointFrames;
	
	// Rendering flags == [R_POS | R_JOINTS | R_ORIENT | R_BONES | R_INFER]
	// --------------------------------------------------------------------
	// R_POS    = whole skeleton position
	// R_JOINTS = skeleton joints
	// R_ORIENT = skeleton joint orientations
	// R_BONES  = connections between skeleton joints
	// R_INFER  = draw inferred joints/bones
	// R_PATH   = draw path of particular joints over time
	static const byte R_POS    = 0x01;
	static const byte R_JOINTS = 0x02;
	static const byte R_INFER  = 0x04;
	static const byte R_ORIENT = 0x08;
	static const byte R_BONES  = 0x10;
	static const byte R_PATH   = 0x20;
	typedef byte RenderingFlags;

private:
	JointFrame *visibleJointFrame;
	JointFrame  currentJointFrame;
	JointFrames jointFrames;

	bool loaded;
	unsigned int frameIndex;

	RenderingFlags renderingFlags;
	EFilteringLevel filteringLevel;

public:
	Skeleton();
	~Skeleton();

	void render() const;
	bool isLoaded() const { return loaded; }
	bool loadFile(const std::string& filename);
	void clearLoadedFrames();

	void nextFrame();
	void prevFrame();

	void setFrameIndex(const float fraction);
	unsigned int getFrameIndex() const { return frameIndex;         }
	unsigned int getNumFrames()  const { return jointFrames.size(); }
	JointFrame& getCurrentJointFrame() { return currentJointFrame;  }

	void togglePosition()                 { renderingFlags ^= R_POS;    }
	void toggleJoints()                   { renderingFlags ^= R_JOINTS; }
	void toggleOrientation()              { renderingFlags ^= R_ORIENT; }
	void toggleBones()                    { renderingFlags ^= R_BONES;  }
	void toggleInferred()                 { renderingFlags ^= R_INFER;  }
	void toggleJointPath()                { renderingFlags ^= R_PATH;   }

	void clearRenderFlags()               { renderingFlags  = 0;   }
	void setRenderFlags(RenderingFlags f) { renderingFlags  = f;   }
	RenderingFlags getRenderFlags() const { return renderingFlags; }

	void setFilterLevel(EFilteringLevel level) { filteringLevel = level; }
	EFilteringLevel getFilterLevel() const     { return filteringLevel;  }

private:
	void renderSkeletonPosition() const;
	void renderJoints() const;

	void renderBone(EJointType fromType, EJointType toType) const;
	void renderBones() const;

	void renderJointPath(EJointType type) const;
	void renderJointPaths() const;

	void renderOrientations() const;
};

