#include "Performance.h"
//#include "Skeleton.h"
#include "Core/Constants.h"

#include <SFML/System/Clock.hpp>

#include <iostream>
#include <fstream>


Performance::Performance()
	: loaded(false)
	, currentFrameIndex(0)
	, frames()
{}

Performance::~Performance()
{}

bool Performance::loadFile( const std::string& filename )
{
	if (loaded) clearLoadedFrames();

	glm::vec3 mn(constants::max_float, constants::max_float, constants::max_float);
	glm::vec3 mx(constants::min_float, constants::min_float, constants::min_float);

	sf::Clock timer;
	std::ifstream loadStream(filename, std::ios::binary | std::ios::in);
	if (!loadStream.is_open()) {
		std::cerr << "Warning: failed to open '" << filename.c_str() << "' for reading" << std::endl;
		return false;
	}

	std::cout << "Opened file: '" << filename.c_str() << "'" << std::endl
			  << "Loading joints positions..." << std::endl;

	int numJointsRead = 0, totalJointsRead = 0, totalFramesRead = 0;
	JointFrame inputFrame;    
	Joint joint;
	while (loadStream.good()) {
		// Load the next joint from the file
		memset(&joint, 0, sizeof(Joint));
		loadStream.read((char *)&joint, sizeof(Joint));
		inputFrame[joint.type] = joint;
		++totalJointsRead;
		
		// Calculate minimum and maximum joint positions
		mn.x = std::min(mn.x, joint.position.x);
		mn.y = std::min(mn.y, joint.position.y);
		mn.z = std::min(mn.z, joint.position.z);
		mx.x = std::max(mx.x, joint.position.x);
		mx.y = std::max(mx.y, joint.position.y);
		mx.z = std::max(mx.z, joint.position.z);

		// Done reading joints for current frame, save it and continue with next frame 
		if (++numJointsRead == NUM_JOINT_TYPES) {
			frames.push_back(inputFrame);
			numJointsRead = 0; 
			++totalFramesRead;
		}
	}

	loadStream.close();
	loaded = true;

	std::cout << "Loaded " << totalJointsRead << " joints in "
			  << totalFramesRead << " frames in "
			  << timer.getElapsedTime().asSeconds() << " seconds." << std::endl
			  << "Done loading skeleton data from '" << filename.c_str() << "'." << std::endl;

	std::cout << "min,max = (" << mn.x << "," << mn.y << "," << mn.z << ")"
			  <<        " , (" << mx.x << "," << mx.y << "," << mx.z << ")"
			  << std::endl;

	// Normalize the z values for each joint in each frame
	for (auto& frame : frames) {
		for (auto joints : frame) {
			Joint& joint = joints.second;
			joint.position.z /= mx.z;
		}
	}

	//currentFrameIndex = 0;
	if (loaded) {
		//normalizeTimestamps(frames);
		const float initialTime = frames.front().at(SHOULDER_CENTER).timestamp;
		for (auto& frame : frames) {
			for (auto& joint : frame) {
				joint.second.timestamp -= initialTime;
			}
		}
		//visibleJointFrame = &jointFrames[frameIndex];
	} else {
		//visibleJointFrame = &currentJointFrame;
	}

	return loaded;
}

void Performance::clearLoadedFrames()
{
	frames.clear();
	currentFrameIndex = 0;
	loaded = false;
}

JointFrame& Performance::getCurrentFrame() {
	assert(!frames.empty() && "Frames must not be empty in order to get a frame.");
	return frames[currentFrameIndex];
}

const JointFrame& Performance::getFrame( unsigned int i /*= 0*/ ) const
{
	assert(!frames.empty() && "Frames must not be empty in order to get a frame.");
	return frames[i];
}

const JointFrame& Performance::getFrameNearestTime( float seconds ) const
{
	assert(!frames.empty() && "Frames must not be empty in order to get a frame.");
	assert(seconds >= 0.f && "'Seconds' must be zero or positive");

	float nearestDelta = constants::max_float;
	unsigned int nearestIndex = 0;
	for (unsigned int i = 0; i < frames.size(); ++i) {
		const float frameTimeStamp = frames[i].at(SHOULDER_CENTER).timestamp;
		const float currentDelta = fabs(frameTimeStamp - seconds);
		if (currentDelta < nearestDelta) {
			nearestDelta = currentDelta;
			nearestIndex = i;
		}
	}

	return frames[nearestIndex];
}

//AnimationFrames Performance::getFrames( unsigned int from, unsigned int to )
//{
//	assert(!frames.empty() && "Frames must not be empty in order to get a frame.");
//	assert(from < to && "'From' index must be less than 'to' index.");
//
//	// If the indices are too big, move to beginning or end
//	if (from >= frames.size()) from = 0;
//	if (to   >= frames.size()) to = frames.size() - 1;
//
//	AnimationFrames outFrames;
//	outFrames.reserve(to - from);
//	for (unsigned int i = from; i <= to; ++i) {
//		outFrames.push_back(frames[i]);
//	}
//	return outFrames;
//}

const AnimationFrames& Performance::getFrames() const
{
	return frames;
}

AnimationFrames& Performance::getFrames()
{
	return frames;
}
