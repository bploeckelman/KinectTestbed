#pragma once
#include <Windows.h>
#include <Ole2.h>
#define WIN32_LEAN_AND_MEAN

#include <NuiApi.h>
#include <NuiSensor.h>

#include <SFML/Graphics/RenderWindow.hpp>

#include "UserInterface.h"
#include "Config.h"


class Application
{
private:
    static const sf::VideoMode videoMode;

    // SFML/SFGUI vars
    sf::RenderWindow window;
    UserInterface gui;

    // OpenGL vars
    GLuint colorTextureId;
    GLuint depthTextureId;
    GLubyte *colorData;
    GLubyte *depthData;

    // Kinect vars
    HANDLE colorStream;
    HANDLE depthStream;

    int numSensors;
    INuiSensor *sensor;

    enum EKinectDataType { COLOR, DEPTH };

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

    int getNumSensors() const { return numSensors; }

private:
    // Main methods
    void mainLoop();
    void processEvents();
    void draw();

    // OpenGL methods
    void initOpenGL();
    void shutdownOpenGL();

    // Kinect methods
    bool initKinect();
    void getKinectData(GLubyte *data, const EKinectDataType &dataType);
    void drawKinectData();
     
};
