#pragma once
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

#include <SFML/OpenGL.hpp>

#include <glm/glm.hpp>

#include <vector>
#include <map>

#include "Performance.h"
#include "Types.h"



class Skeleton
{
private:
	JointFrame *visibleJointFrame;
	JointFrame liveJointFrame;
	Performance performance;

	bool loaded;
	bool useMaterials;
	unsigned int frameIndex;
	GLUquadric *quadric;

	RenderingFlags renderingFlags;
	EFilteringLevel filteringLevel;

public:
	Skeleton();
	~Skeleton();

	void render(); //const;
	bool isLoaded() const { return performance.isLoaded(); }
	bool loadFile(const std::string& filename) { return performance.loadFile(filename); }
	void clearLoadedFrames() { performance.clearLoadedFrames(); }

	void nextFrame();
	void prevFrame();

	void applyPerformance(AnimationFrames& newFrames);

	void setFrameIndex(const float fraction);
	unsigned int getFrameIndex() const { return performance.getCurrentFrameIndex(); }
	unsigned int getNumFrames()  const { return performance.getNumFrames(); }

	Performance& getPerformance() { return performance; }

	float getAnimationDuration() const {
		if (!loaded) return 0.f;
		const AnimationFrames& jointFrames = performance.getFrames();
		const float firstTime = jointFrames.front().at(SHOULDER_CENTER).timestamp;
		const float lastTime  = jointFrames.back().at(SHOULDER_CENTER).timestamp;
		return lastTime - firstTime;
	}

	JointFrame& getCurrentJointFrame() { return liveJointFrame;  }
	const Joint& getCurrentRightHand() { return liveJointFrame.at(HAND_RIGHT); }
	const Joint& getCurrentLeftHand()  { return liveJointFrame.at(HAND_LEFT);  }

	GLUquadric* getQuadric() { return quadric; }

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
	void renderJoints() const;

	void renderBone(EJointType fromType, EJointType toType) const;
	void renderBones() const;

	void renderJointPath(const EJointType type) const;
	void renderJointPaths() const;

	void renderOrientations() const;
};


inline void normalizeTimestamps( AnimationFrames& animationFrames ) {
	const float initialTime = animationFrames.front().at(SHOULDER_CENTER).timestamp;
	for (auto& frame : animationFrames) {
		for (auto& joint : frame) {
			joint.second.timestamp -= initialTime;
		}
	}
}
