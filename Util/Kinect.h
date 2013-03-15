#pragma once
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

#include <NuiApi.h>

#include <string>
#include <vector>

enum EKinectDataType { COLOR, DEPTH };

class Kinect
{
public:
	static const int COLOR_STREAM_WIDTH  = 1280;
	static const int COLOR_STREAM_HEIGHT = 960;
	static const int COLOR_STREAM_BYTES  = 4 * COLOR_STREAM_WIDTH * COLOR_STREAM_HEIGHT; // BGRA

	static const int DEPTH_STREAM_WIDTH  = 640;
	static const int DEPTH_STREAM_HEIGHT = 480;
	static const int DEPTH_STREAM_BYTES  = 4 * DEPTH_STREAM_WIDTH * DEPTH_STREAM_HEIGHT; // BGRA

private:
	bool initialized;

	std::string deviceId;
	std::vector<INuiSensor *> sensors;

	HANDLE colorStream;
	HANDLE depthStream;
	HANDLE nextSkeletonEvent;
	DWORD  skeletonTrackingFlags;

	// TODO: Skeleton skeleton;

public:
	Kinect();
	~Kinect();

	bool initialize();

	void getStreamData(byte *dest, const EKinectDataType &dataType, unsigned int sensorIndex = 0);

	bool isInitialized() const;
	int getNumSensors() const;
	INuiSensor *getSensor(unsigned int i) const;
	const std::string &getDeviceId() const;

};
