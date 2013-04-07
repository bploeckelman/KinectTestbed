#pragma once
#include <Windows.h>
#include <Ole2.h>
#define WIN32_LEAN_AND_MEAN

#include <NuiApi.h>
#include <NuiSensor.h>

#include <SFML/Graphics/RenderWindow.hpp>

#include <glm/glm.hpp>

#include <fstream>
#include <string>

#include "Core/Config.h"
#include "Kinect/Kinect.h"
#include "UI/UserInterface.h"


class Application
{
private:
	static const sf::VideoMode videoMode;

	sf::Clock clock;
	sf::RenderWindow window;
	UserInterface gui;

	// TODO : move to OpenGLEnvironment class?
	GLuint colorTextureId;
	GLuint depthTextureId;
	GLubyte *colorData;
	GLubyte *depthData;

	Kinect kinect;

	bool showColor;
	bool showDepth;
	bool showSkeleton;
	bool handControl;

	bool autoPlay;
	float lastFrameTime;

	bool rightMouseDown;
	bool leftMouseDown;
	bool shiftDown;

	// TODO : extract camera control out to its own class 
	float camerax;
	float cameray;
	float cameraz;

	glm::vec3 lastBinormal;
	glm::vec3 lastNormal;
	glm::vec3 lastTangent;

	glm::mat4 projection;
	glm::mat4 modelview;

private:
	// Singleton
	Application();
	Application(const Application& application);
	Application& operator=(const Application& application);

public:
	// Singleton
	static Application& request() { static Application application; return application; }

	~Application();

	void startup();
	void shutdown();

	void loadFile();
	void closeFile();
	void moveToNextFrame();
	void moveToPreviousFrame();
	void setJointFrameIndex(const float fraction);

	void toggleAutoPlay()      { autoPlay     = !autoPlay;     }
	void toggleShowColor()     { showColor    = !showColor;    }
	void toggleShowDepth()     { showDepth    = !showDepth;    }
	void toggleShowSkeleton()  { showSkeleton = !showSkeleton; }
	void toggleHandControl()   { handControl  = !handControl;  }

	bool isSaving()     const { return kinect.isSaving(); }
	bool isLoaded()     const { return kinect.getSkeleton().isLoaded(); }
	bool isAutoPlay()   const { return autoPlay; }
	int getNumSensors() const { return kinect.getNumSensors(); }
	const sf::Vector2i getMousePosition() const { return sf::Mouse::getPosition(window); }

	Kinect& getKinect()             { return kinect; }
	const Kinect& getKinect() const { return kinect; }

private:
	void mainLoop();
	void processEvents();
	void draw();

	float getCameraRotationX();
	float getCameraRotationY();
	std::wstring showFileChooser();

	void initOpenGL();
	void shutdownOpenGL();

	// Kinect methods -------------------------------------
	// TODO : move these to Kinect class?
	void updateKinectImageStreams();
	void drawKinectImageStreams() ;
};
