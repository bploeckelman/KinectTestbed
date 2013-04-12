#pragma once
//#include <Windows.h>
//#define WIN32_LEAN_AND_MEAN

//#include <SFML/OpenGL.hpp>

#include <glm/glm.hpp>

#include <vector>
#include <map>

#include "Types.h"


class Performance 
{

private:
	bool loaded;
	unsigned int currentFrameIndex;
	AnimationFrames frames;

public:
	Performance();
	~Performance();

	bool isLoaded() const;
	bool loadFile(const std::string& filename);
	void clearLoadedFrames();

	float getDuration() const;
	int  getNumFrames() const;

	void moveToFrame(unsigned int i);
	void moveToNextFrame();
	void moveToPrevFrame();

	JointFrame& getCurrentFrame();
	const JointFrame& getFrame(unsigned int i = 0) const;
	const JointFrame& getFrameNearestTime(float seconds) const;

	const AnimationFrames& getFrames() const;
	AnimationFrames& getFrames();
	//AnimationFrames getFrames(unsigned int from, unsigned int to);

}; // end class Performance


inline bool Performance::isLoaded()     const { return loaded; }
inline int  Performance::getNumFrames() const { return frames.size(); }

inline float Performance::getDuration() const {
	if (!loaded) return 0.f;
	const float lastTimestamp  = frames.back().at(SHOULDER_CENTER).timestamp;
	const float firstTimestamp = frames.front().at(SHOULDER_CENTER).timestamp;
	return (lastTimestamp - firstTimestamp);
}

inline void Performance::moveToFrame( unsigned int i ) {
	if (!loaded) return;

	const unsigned int index = (i < frames.size()) ? i : frames.size() - 1;
	currentFrameIndex = index;	
}

inline void Performance::moveToNextFrame() {
	if (!loaded) return;
	
	// TODO: loop?
	const unsigned int nextFrameIndex = currentFrameIndex + 1;
	if (nextFrameIndex < frames.size()) {
		currentFrameIndex = nextFrameIndex;
	} else {
		moveToFrame(0);
	}
}

inline void Performance::moveToPrevFrame() {
	if (!loaded) return;
	
	// TODO: loop?
	int prevFrameIndex = currentFrameIndex - 1;
	if (prevFrameIndex >= 0) {
		currentFrameIndex = prevFrameIndex;
	} else {
		moveToFrame(frames.size() - 1);
	}
}
