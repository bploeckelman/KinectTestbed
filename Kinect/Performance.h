#pragma once
//#include <Windows.h>
//#define WIN32_LEAN_AND_MEAN

//#include <SFML/OpenGL.hpp>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <map>

#include <glm/glm.hpp>

#include "Types.h"


class Performance 
{
private:
	bool loaded;
	std::string name;

	AnimationFrames frames;
	unsigned int currentFrameIndex;

	glm::vec3 minPos;
	glm::vec3 maxPos;

public:
	Performance(const std::string& name);
	Performance(const std::string& name, const AnimationFrames& frames);

	bool load(const std::string& filename);
	void clear();

	void moveToNextFrame();
	void moveToPrevFrame();

	bool isLoaded() const;
	const std::string& getName() const;

	int getNumFrames() const;
	float getDuration() const;

	void setCurrentFrameIndex(unsigned int i);
	unsigned int getCurrentFrameIndex() const;

	Joint& getCurrentFrameJoint(unsigned int i);

	JointFrame& getCurrentFrame();
	JointFrame& getFrame(unsigned int i = 0);
	JointFrame& getFrameNearestTime(float seconds);

	AnimationFrames& getFrames();
	const AnimationFrames& getFrames() const;

}; // end class Performance


inline bool Performance::isLoaded()     const { return loaded; }
inline int  Performance::getNumFrames() const { return frames.size(); }
inline const std::string& Performance::getName() const { return name; }

inline void Performance::setCurrentFrameIndex(unsigned int index) {
	currentFrameIndex = (index < frames.size()) ? index : index % frames.size();
}
inline unsigned int Performance::getCurrentFrameIndex() const { return currentFrameIndex; }

inline float Performance::getDuration() const {
	if (!loaded) return 0.f;
	return (frames.back().timestamp - frames.front().timestamp);
}

inline void Performance::moveToNextFrame() {
	if (!loaded) return;
	const unsigned int nextFrameIndex = currentFrameIndex + 1;
	currentFrameIndex = (nextFrameIndex < frames.size()) ? nextFrameIndex : 0;
}

inline void Performance::moveToPrevFrame() {
	if (!loaded) return;
	int prevFrameIndex = currentFrameIndex - 1;
	currentFrameIndex = (prevFrameIndex >= 0) ? prevFrameIndex : frames.size() - 1;
}
