#pragma once
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

#include <SFML/OpenGL.hpp>

#include <glm/glm.hpp>

#include <iostream>
#include <list>
#include <map>

#include "Performance.h"
#include "Types.h"



class Skeleton
{
private:
	JointFrame *visibleJointFrame;
	JointFrame liveJointFrame;
	Performance *performance;
	std::list<Performance> performances;

	bool loaded;
	bool useMaterials;
	float alpha;
	unsigned int frameIndex;
	GLUquadric *quadric;

	RenderingFlags renderingFlags;
	EFilteringLevel filteringLevel;

public:
	Skeleton();
	~Skeleton();

	void render(); //const;
	bool isLoaded() const { return performance != nullptr && performance->isLoaded(); }
	bool loadFile(const std::string& filename) {
		performances.push_back(Performance(filename));
		return performances.back().loadFile(filename);
	}
	void clearLoadedFrames() { performance->clearLoadedFrames(); }

	void nextFrame();
	void prevFrame();

	void addPerformance( const Performance& newPerformance );
	void applyPerformance(Performance& newPerformance);

	void setFrameIndex(const float fraction);
	unsigned int getFrameIndex() const { return performance->getCurrentFrameIndex(); }
	unsigned int getNumFrames()  const { return performance->getNumFrames(); }

	void setPerformance( const std::string& name ) {
		bool success = false;
		for (auto p : performances) {
			if (name == p.getName()) {
				performance = &p;
				success = true;
				break;
			}
		}
		if (!success) {
			std::cout << "Warning: no such performance '" << name << "'" << std::endl;
		}
	}
	Performance& getPerformance() { return *performance; }
	std::list<Performance>& getPerformances() { return performances; }

	float getAnimationDuration() const {
		if (!loaded) return 0.f;
		const AnimationFrames& jointFrames = performance->getFrames();
		const float firstTime = jointFrames.front().timestamp;
		const float lastTime  = jointFrames.back().timestamp;
		return lastTime - firstTime;
	}

	JointFrame& getCurrentJointFrame() { return liveJointFrame;  }
	const Joint& getCurrentJoint(const EJointType& jointType) { return liveJointFrame.joints.at(jointType); }
	const Joint& getCurrentRightHand() { return liveJointFrame.joints.at(HAND_RIGHT); }
	const Joint& getCurrentLeftHand()  { return liveJointFrame.joints.at(HAND_LEFT);  }

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
	const float initialTime = animationFrames.front().timestamp;
	for (auto& frame : animationFrames) {
		frame.timestamp -= initialTime;
	}
}
