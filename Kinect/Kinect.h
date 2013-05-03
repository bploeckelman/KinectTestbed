#pragma once
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

#include <NuiApi.h>

#include <SFML/System/Clock.hpp>

#include "Skeleton.h"

#include <fstream>
#include <string>
#include <vector>

enum EStreamDataType { COLOR, DEPTH };

class Kinect
{
public:
	static const int COLOR_STREAM_WIDTH  = 1280;
	static const int COLOR_STREAM_HEIGHT = 960;
	static const int COLOR_STREAM_BYTES  = 4 * COLOR_STREAM_WIDTH * COLOR_STREAM_HEIGHT; // BGRA

	static const int DEPTH_STREAM_WIDTH  = 640;
	static const int DEPTH_STREAM_HEIGHT = 480;
	static const int DEPTH_STREAM_BYTES  = 4 * DEPTH_STREAM_WIDTH * DEPTH_STREAM_HEIGHT; // BGRA

	static const std::string saveFileName;

private:
	bool initialized;
	bool saving;
	bool isLayering;
	float performanceTimer;
	unsigned int numFramesSaved;

	sf::Clock clock;

	std::string deviceId;
	std::vector<INuiSensor *> sensors;

	HANDLE colorStream;
	HANDLE depthStream;
	HANDLE nextSkeletonEvent;
	DWORD  skeletonTrackingFlags;

	Skeleton skeleton;
	AnimationFrames layerFrames;

	std::ofstream saveStream;

public:
	Kinect();
	~Kinect();

	bool initialize();
	void update();

	void startLayering();
	void toggleSave();
	void toggleSeatedMode();

	int  getNumSensors() const;
	bool isSaving()      const;
	bool isInitialized() const;
	const std::string& getDeviceId() const;

	Skeleton& getSkeleton();
	const Skeleton& getSkeleton() const;

	INuiSensor *getSensor(unsigned int i = 0) const;
	void getStreamData(byte *dest, const EStreamDataType& dataType, unsigned int sensorIndex = 0);

private:
	bool isSeatedModeEnabled() const;

	void checkForSkeletonFrame();
	void skeletonFrameReady(NUI_SKELETON_FRAME& skeletonFrame);

};

inline int Kinect::getNumSensors()        const { return sensors.size(); }
inline bool Kinect::isSaving()            const { return saving; }
inline bool Kinect::isInitialized()       const { return initialized; }
inline bool Kinect::isSeatedModeEnabled() const { return 0 != (skeletonTrackingFlags & NUI_SKELETON_FRAME_FLAG_SEATED_SUPPORT_ENABLED); }

inline const std::string& Kinect::getDeviceId() const { return deviceId; }
inline Skeleton& Kinect::getSkeleton()                { return skeleton; }
inline const Skeleton& Kinect::getSkeleton()    const { return skeleton; }
