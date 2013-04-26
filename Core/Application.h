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

	Kinect kinect;
	UserInterface gui;

	// TODO : move to OpenGLEnvironment class?
	GLuint colorTextureId;
	GLuint depthTextureId;
	GLubyte *colorData;
	GLubyte *depthData;

	// TODO : add Performances and Skeletons

	// TODO : encapsulate in Input class?
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

	// TODO : move these elsewhere?
	void loadFile();
	void closeFile();
	void moveToNextFrame();
	void moveToPreviousFrame();
	void setJointFrameIndex(const float fraction);
	bool isSaving() const;
	bool isLoaded() const;
	int getNumSensors() const;

	const sf::Vector2i getMousePosition() const;

	Kinect& getKinect();
	const Kinect& getKinect() const;

	UserInterface& getGUI();
	const UserInterface& getGUI() const;

private:
	void mainLoop();
	void processEvents();
	void draw();

	// TODO : fix these horrible methods
	float getCameraRotationX();
	float getCameraRotationY();

	// TODO : make non-member function
	std::wstring showFileChooser();

	// TODO : move to opengl environment class?
	void initOpenGL();
	void shutdownOpenGL();

	// TODO : move these to Kinect class?
	void updateKinectImageStreams();
	void drawKinectImageStreams() ;
};


// Interface functions
inline bool Application::isSaving()     const { return kinect.isSaving(); }
inline bool Application::isLoaded()     const { return kinect.getSkeleton().isLoaded(); }
inline int Application::getNumSensors() const { return kinect.getNumSensors(); }

inline const sf::Vector2i Application::getMousePosition() const {
	return sf::Mouse::getPosition(window);
}

inline Kinect& Application::getKinect()             { return kinect; }
inline const Kinect& Application::getKinect() const { return kinect; }

// TODO : still needed?
inline UserInterface& Application::getGUI()             { return gui; }
inline const UserInterface& Application::getGUI() const { return gui; }
