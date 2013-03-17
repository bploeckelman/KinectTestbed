#include "Kinect.h"
#include "Core/Constants.h"

#include <NuiApi.h>

#include <SFML/System/Clock.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <cassert>
#include <tchar.h>

std::string toStdString(const BSTR bstr);
Skeleton::EJointType toJointType(unsigned int i);
NUI_SKELETON_POSITION_INDEX toPositionIndex(unsigned int i);
glm::mat4 toMat4(const Matrix4& m);

// TODO : allow user to change path and filename for output
const std::string Kinect::saveFileName("../../Res/Out/joint_frames.bin");


Kinect::Kinect()
	: initialized(false)
	, saving(false)
	, numFramesSaved()
	, clock()
	, deviceId("?")
	, sensors()
	, colorStream()
	, depthStream()
	, nextSkeletonEvent()
	, skeletonTrackingFlags(NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT)
	, skeleton()
	, saveStream()
{}

Kinect::~Kinect()
{
	// TODO: delete each sensor and the streams
	if (saveStream.is_open()) saveStream.close();
}

bool Kinect::initialize()
{
	clock.restart();

	int numSensors = -1;
	HRESULT hr = NuiGetSensorCount(&numSensors);
	if (!SUCCEEDED(hr) || numSensors < 1) {
		std::cerr << "Failed to find Kinect sensors." << std::endl;
		return false;
	}

	// Create and initialize each sensor
	for (int i = 0; i < numSensors; ++i) {
		INuiSensor *sensor = nullptr;		
		hr = NuiCreateSensorByIndex(i, &sensor);
		if (!SUCCEEDED(hr) || sensor == nullptr) {
			std::cerr << "Failed to create Kinect sensor #" << i << std::endl;
			return false;
		}
		sensors.push_back(sensor);

		// Initialize sensor
		hr = sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH
								 | NUI_INITIALIZE_FLAG_USES_COLOR
								 | NUI_INITIALIZE_FLAG_USES_SKELETON);
		if (!SUCCEEDED(hr)) {
			std::cerr << "Failed to initialize Kinect sensor #" << i << std::endl;
			return false;
		}
		
		// Open color stream to receive color data
		hr = sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR
			, NUI_IMAGE_RESOLUTION_1280x960
			, 0    // Image stream flags, eg. near mode...
			, 2    // Number of frames to buffer
			, NULL // Event handle
			, &colorStream);
		if (!SUCCEEDED(hr)) {
			std::cerr << "Failed to open color stream for Kinect sensor #" << i << std::endl;
			return false;
		}

		// Open depth stream to receive depth data
		// NOTE: if type: depth and player index, resolution 320x240
		hr = sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH
			, NUI_IMAGE_RESOLUTION_640x480
			, 0    // Image stream flags, eg. near mode...
			, 2    // Number of frames to buffer
			, NULL // Event handle
			, &depthStream);
		if (!SUCCEEDED(hr)) {
			std::cerr << "Failed to open depth stream for Kinect sensor #" << i << std::endl;
			return false;
		}

		// Create an event that will be signaled when skeleton data is available
		nextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

		// Open skeleton stream to receive skeleton data
		hr = sensor->NuiSkeletonTrackingEnable(nextSkeletonEvent, skeletonTrackingFlags);
		if (!SUCCEEDED(hr)) {
			std::cerr << "Failed to enable skeleton tracking for Kinect sensor #" << i << std::endl;
			return false;
		}

		deviceId = toStdString(sensor->NuiDeviceConnectionId());
		std::cout << "Initialized Kinect #" << i << " with Device Id [" << deviceId.c_str() << "]" << std::endl;
	}

	initialized = (sensors.size() >= 1);
	if (!initialized) {
		std::cerr << "Unable to initialize Kinect." << std::endl;
	} else {
		std::cout << "Initialized " << sensors.size() << " Kinect device(s) "
				  << "in " << clock.getElapsedTime().asSeconds() << " seconds."
				  << std::endl;
	}

	return initialized;
}

void Kinect::update()
{
	checkForSkeletonFrame();
}

void Kinect::toggleSave()
{
	std::cout << (saving ? "Stopped" : "Started") << " saving joint data." << std::endl;

	saving = !saving; 
	if (saving) {
		if (!saveStream.is_open())
			saveStream.open(saveFileName, std::ios::binary | std::ios::app);
	} else {
		std::cout << "Joint frames saved: " << numFramesSaved << std::endl;
		if (saveStream.is_open())
			saveStream.close();
	}
}

void Kinect::getStreamData( byte *dest, const EStreamDataType& dataType, unsigned int sensorIndex )
{
	INuiSensor *sensor = getSensor(sensorIndex);
	if (sensor == nullptr) {
		std::cerr << "Failed to get Kinect sensor #" << sensorIndex << std::endl;
		return;
	}

	HANDLE streamHandle;
	     if (dataType == COLOR) streamHandle = colorStream;
	else if (dataType == DEPTH) streamHandle = depthStream;

	// Get next frame
	// TODO : use event notifier like skeleton
	NUI_IMAGE_FRAME imageFrame;
	HRESULT hr = sensor->NuiImageStreamGetNextFrame(streamHandle, 0, &imageFrame);
	if (!SUCCEEDED(hr)) {
		//std::cerr << "Failed to get next image frame from Kinect sensor #" << sensorIndex << std::endl;
		return;
	}

	// Copy frame data to destination buffer
	NUI_LOCKED_RECT lockedRect;
	imageFrame.pFrameTexture->LockRect(0, &lockedRect, NULL, 0);	
	if (lockedRect.Pitch != 0) {
		if (dataType == COLOR) {
			const byte *curr = (const byte *) lockedRect.pBits;
			const byte *last = curr + COLOR_STREAM_BYTES;

			// Store to dest as BGRA
			while (curr < last) {
				*dest++ = *curr++;
			}
		}
		else if (dataType == DEPTH) {
			const USHORT *curr = (const USHORT *) lockedRect.pBits;
			const USHORT *last = curr + (DEPTH_STREAM_WIDTH * DEPTH_STREAM_HEIGHT);

			while (curr < last) {
				// Depth distance stored in top 13 bits, get normalized value
				USHORT depth = NuiDepthPixelToDepth(*curr++);
				USHORT dist  = (depth >> 3) / 2^13;

				// Store to dest as BGRA
				byte thresh = 128;
				byte value  = (byte) (dist * 255);
				*dest++ = (value > thresh) ? 0 : value; 
				*dest++ = (value);
				*dest++ = (value > thresh) ? 0 : value;
				*dest++ = 0xff; 
			}
		}
	}
	imageFrame.pFrameTexture->UnlockRect(0);

	// Release frame
	hr = sensor->NuiImageStreamReleaseFrame(streamHandle, &imageFrame);
	if (!SUCCEEDED(hr)) {
		std::cerr << "Failed to release image frame from Kinect sensor #" << sensorIndex << std::endl;
		return;
	}
}

INuiSensor * Kinect::getSensor( unsigned int i ) const
{
	assert(sensors.size() > 0 && i < sensors.size());
	return sensors[i];
}

void Kinect::checkForSkeletonFrame()
{
	// Wait for 0ms to quickly test if it is time to process a skeleton frame
	if (WAIT_OBJECT_0 == WaitForSingleObject(nextSkeletonEvent, 0)) {
		// Get and process the skeleton frame that is ready
		NUI_SKELETON_FRAME skeletonFrame = {0};
		HRESULT hr = getSensor()->NuiSkeletonGetNextFrame(0, &skeletonFrame);
		if (!SUCCEEDED(hr)) {
			std::cerr << "Failed to get ready skeleton frame from Kinect sensor #0" << std::endl;
			return;
		}
		skeletonFrameReady(skeletonFrame);
	}
}

void Kinect::skeletonFrameReady( NUI_SKELETON_FRAME& skeletonFrame )
{
	// Reset clock for timestamps on first run
	static bool firstRun = true;
	if (firstRun) {
		firstRun = false;
		clock.restart();
	}
	const float timestamp = clock.getElapsedTime().asSeconds();

	// Get data for the first tracked skeleton
	const NUI_SKELETON_DATA *skeletonData = nullptr;
	for (auto i = 0; i < NUI_SKELETON_COUNT; ++i) {
		if (skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED) {
			skeletonData = &skeletonFrame.SkeletonData[i];
			break;
		}
	}
	if (skeletonData == nullptr) {
		//std::cerr << "Failed to find tracked skeleton from Kinect sensor #0" << std::endl;
		return;
	}

	// Set filtering level
	switch (skeleton.getFilterLevel()) {
		case Skeleton::OFF:    break;
		case Skeleton::LOW:    NuiTransformSmooth(&skeletonFrame, constants::joint_smooth_params_low);  break;
		case Skeleton::MEDIUM: NuiTransformSmooth(&skeletonFrame, constants::joint_smooth_params_med);  break;
		case Skeleton::HIGH:   NuiTransformSmooth(&skeletonFrame, constants::joint_smooth_params_high); break;
	}

	// Get bone orientations for this skeleton's joints
	NUI_SKELETON_BONE_ORIENTATION boneOrientations[NUI_SKELETON_POSITION_COUNT];
	HRESULT hr = NuiSkeletonCalculateBoneOrientations(skeletonData, boneOrientations);
	if (!SUCCEEDED(hr)) {
		std::cerr << "Failed to calculate bone orientations Kinect sensor #0" << std::endl;
	}

	// For each joint type...
	for (auto i = 0; i < NUI_SKELETON_POSITION_COUNT; ++i) {
		// Get joint data in Kinect API form
		const NUI_SKELETON_POSITION_INDEX   positionIndex   = toPositionIndex(i);
		const NUI_SKELETON_BONE_ORIENTATION boneOrientation = boneOrientations[positionIndex];
		const NUI_SKELETON_POSITION_TRACKING_STATE positionTrackingState = skeletonData->eSkeletonPositionTrackingState[positionIndex];
		const Vector4& position = skeletonData->SkeletonPositions[positionIndex];
		const Matrix4& matrix4 = boneOrientation.absoluteRotation.rotationMatrix;

		// Update the joint frame entry for this joint type
		Skeleton::Joint& joint = skeleton.getCurrentJointFrame()[toJointType(i)];
		joint.timestamp     = timestamp;
		joint.position      = glm::vec3(position.x, position.y, position.z);
		joint.orientation   = toMat4(matrix4);;
		joint.type          = toJointType(i);
		joint.trackingState = static_cast<Skeleton::ETrackingState>(positionTrackingState);

		// Save the joint frame entry if appropriate
		if (saving && saveStream.is_open()) {
			saveStream.write((char *)&joint, sizeof(Skeleton::Joint));
		}
	}

	if (saving && saveStream.is_open()) {
		++numFramesSaved;
	}
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


std::string toStdString( const BSTR bstr )
{
	const std::wstring wstr(bstr, SysStringLen(bstr));
	std::string str; str.assign(wstr.begin(), wstr.end());
	return str;
}

Skeleton::EJointType toJointType( unsigned int i )
{
	assert(i < Skeleton::NUM_JOINT_TYPES);
	return static_cast<Skeleton::EJointType>(i);
}

NUI_SKELETON_POSITION_INDEX toPositionIndex( unsigned int i )
{
	assert(i < NUI_SKELETON_POSITION_COUNT);
	return static_cast<NUI_SKELETON_POSITION_INDEX>(i);
}

glm::mat4 toMat4( const Matrix4& m )
{
	return glm::mat4(
		  m.M11, m.M12, m.M13, m.M14
		, m.M21, m.M22, m.M23, m.M24
		, m.M31, m.M32, m.M33, m.M34
		, m.M41, m.M42, m.M43, m.M44);
}
