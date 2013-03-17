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
	unsigned int numFramesSaved;

	sf::Clock clock;

	std::string deviceId;
	std::vector<INuiSensor *> sensors;

	HANDLE colorStream;
	HANDLE depthStream;
	HANDLE nextSkeletonEvent;
	DWORD  skeletonTrackingFlags;

	Skeleton skeleton;

	std::ofstream saveStream;

public:
	Kinect();
	~Kinect();

	bool initialize();
	void update();

	void toggleSave();
	void toggleSeatedMode();

	void getStreamData(byte *dest, const EStreamDataType& dataType, unsigned int sensorIndex = 0);

	Skeleton& getSkeleton()             { return skeleton; }
	const Skeleton& getSkeleton() const { return skeleton; }

	bool isInitialized() const { return initialized; }
	bool isSaving()      const { return saving; }

	int  getNumSensors() const { return sensors.size(); }
	const std::string& getDeviceId() const { return deviceId; }

	INuiSensor *getSensor(unsigned int i = 0) const;

private:
	void checkForSkeletonFrame();
	void skeletonFrameReady(NUI_SKELETON_FRAME& skeletonFrame);

	bool isSeatedModeEnabled() const { return 0 != (skeletonTrackingFlags & NUI_SKELETON_FRAME_FLAG_SEATED_SUPPORT_ENABLED); }

};
