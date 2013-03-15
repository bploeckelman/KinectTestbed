#include "Kinect.h"

#include <NuiApi.h>

#include <iostream>
#include <string>
#include <sstream>
#include <cassert>

std::string toStdString(const BSTR bstr);


Kinect::Kinect()
	: initialized(false)
	, deviceId("?")
	, sensors()
	, colorStream()
	, depthStream()
	, nextSkeletonEvent()
	, skeletonTrackingFlags(NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT)
{}

Kinect::~Kinect()
{
	// TODO: delete each sensor and the streams
}

bool Kinect::initialize()
{
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

		std::cout << "Initialized Kinect #" << i << " with Device Id "
				  << "[" << toStdString(sensor->NuiDeviceConnectionId()) << "]"
				  << std::endl;
	}

	return (initialized = (sensors.size() >= 1));
}

void Kinect::getStreamData( byte *dest, const EKinectDataType &dataType, unsigned int sensorIndex )
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

int Kinect::getNumSensors() const
{
	return sensors.size();
}

INuiSensor * Kinect::getSensor( unsigned int i ) const
{
	assert(i >= 0 && i < sensors.size());
	return (sensors.size() == 0) ? nullptr : sensors[i];
}

bool Kinect::isInitialized() const
{
	return initialized;
}

const std::string & Kinect::getDeviceId() const
{
	return deviceId;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


std::string toStdString( const BSTR bstr )
{
	const std::wstring wstr(bstr, SysStringLen(bstr));
	std::string deviceId;
	deviceId.assign(wstr.begin(), wstr.end());
	return deviceId;
}
