#pragma once
#include "Performance.h"
#include "Types.h"


class SkeletonViz
{
private:
	Performance *performance;
	RenderParams params;
	GLUquadric *quadric;

public:
	SkeletonViz();
	~SkeletonViz();

	void update(float delta);
	void render();

	void attachPerformance(Performance& p);
	void removePerformance();

	const bool isLoaded() const;

	void toggleJoints();
	void toggleOrients();
	void toggleBones();
	void toggleInferred();
	void togglePaths();

	RenderParams& getRenderParams();

private:
	void renderJoints();

	void renderBone(EJointType fromType, EJointType toType);
	void renderBones();

	void renderJointPath(const EJointType type);
	void renderJointPaths();

	void renderOrientations();

};


inline void SkeletonViz::attachPerformance(Performance& p) { performance = &p; }
inline void SkeletonViz::removePerformance() { performance = nullptr; }

inline const bool SkeletonViz::isLoaded() const { return (performance != nullptr); }

inline void SkeletonViz::toggleJoints()   { params.flags ^= R_JOINTS; }
inline void SkeletonViz::toggleOrients()  { params.flags ^= R_ORIENT; }
inline void SkeletonViz::toggleBones()    { params.flags ^= R_BONES;  }
inline void SkeletonViz::toggleInferred() { params.flags ^= R_INFER;  }
inline void SkeletonViz::togglePaths()    { params.flags ^= R_PATH;   }

inline RenderParams& SkeletonViz::getRenderParams() { return params; }
