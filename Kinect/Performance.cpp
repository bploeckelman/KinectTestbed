#include "Performance.h"
#include "Core/Constants.h"

#include <SFML/System/Clock.hpp>

#include <iostream>
#include <fstream>

using namespace constants;


Performance::Performance( const std::string& name )
	: loaded(false)
	, currentFrameIndex(0)
	, frames()
	, name(name)
	, minPos(-1, -1, -1)
	, maxPos( 1,  1,  1)
{}

Performance::Performance( const std::string& name, const AnimationFrames& frames )
	: loaded(true)
	, currentFrameIndex(0)
	, frames(frames)
	, name(name)
	, minPos(-1, -1, -1)
	, maxPos( 1,  1,  1)
{}

// TODO : bool Performance::load( const AnimationFrames& frames ) {}

bool Performance::load( const std::string& filename )
{
	if (loaded) clear();

	sf::Clock timer;
	glm::vec3 mn(max_float, max_float, max_float);
	glm::vec3 mx(min_float, min_float, min_float);

	std::ifstream loadStream(filename, std::ios::binary | std::ios::in);
	if (!loadStream.is_open()) {
		std::cerr << "Warning: failed to open '" << filename.c_str() << "' for reading" << std::endl;
		return false;
	} else {
		std::cout << "Opened file: '" << filename.c_str() << "'" << std::endl
		          << "Loading joints positions..." << std::endl;
	}

	int numJointsRead = 0, totalJointsRead = 0, totalFramesRead = 0;
	JointFrame inputFrame;    
	Joint joint;
	while (loadStream.good()) {
		// Load the next joint from the file
		memset(&joint, 0, sizeof(Joint));
		loadStream.read((char *)&joint, sizeof(Joint));
		inputFrame.joints[joint.type] = joint;
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
			loadStream.read((char *)&inputFrame.timestamp, sizeof(float));
			std::cerr << "read timestamp[" << totalFramesRead << "] = " << inputFrame.timestamp << std::endl;
			frames.push_back(inputFrame);
			numJointsRead = 0; 
			++totalFramesRead;
		}
	}

	loadStream.close();
	loaded = true;

	std::cout << "Loaded " << totalJointsRead << " joints in "
			  << totalFramesRead << " frames in "
			  << timer.getElapsedTime().asSeconds() << " seconds." << std::endl;
			  //<< "Done loading skeleton data from '" << filename.c_str() << "'." << std::endl;

	minPos = mn;
	maxPos = mx;
	std::cout << "min,max = (" << mn.x << "," << mn.y << "," << mn.z << ")"
			  <<        " , (" << mx.x << "," << mx.y << "," << mx.z << ")"
			  << std::endl;

	// Normalize the z values for each joint in each frame
	//for (auto& frame : frames) {
	//	for (auto joints : frame.joints) {
	//		Joint& joint = joints.second;
	//		joint.position.z /= mx.z;
	//	}
	//}

	//currentFrameIndex = 0;
	if (loaded) {
		//normalizeTimestamps(frames);
		//const float initialTime = frames.front().timestamp;
		//for (auto& frame : frames) {
		//	frame.timestamp -= initialTime;
		//}
		//visibleJointFrame = &jointFrames[frameIndex];
	} else {
		//visibleJointFrame = &currentJointFrame;
	}

	return loaded;
}

void Performance::clear()
{
	frames.clear();
	currentFrameIndex = 0;
	loaded = false;
}

JointFrame& Performance::getCurrentFrame()
{
	return getFrame(currentFrameIndex);
}

JointFrame& Performance::getFrame( unsigned int i /*= 0*/ )
{
	assert(!frames.empty() && "Frames must not be empty in order to get a frame.");
	return frames[i];
}

JointFrame& Performance::getFrameNearestTime( float seconds )
{
	assert(!frames.empty() && "Frames must not be empty in order to get a frame.");
	assert(seconds >= 0.f && "'Seconds' must be zero or positive");

	float nearestDelta = constants::max_float;
	unsigned int nearestIndex = 0;
	for (unsigned int i = 0; i < frames.size(); ++i) {
		const float frameTimeStamp = frames[i].timestamp;
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

AnimationFrames& Performance::getFrames()
{
	return frames;
}

const AnimationFrames& Performance::getFrames() const
{
	return frames;
}

Joint& Performance::getCurrentFrameJoint( unsigned int i )
{
	return getCurrentFrame().joints[static_cast<EJointType>(i)];
}
