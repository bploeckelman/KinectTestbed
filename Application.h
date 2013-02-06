#pragma once
#include <SFML/Graphics/RenderWindow.hpp>

#include "UserInterface.h"
#include "Config.h"

#include <NuiApi.h>
#include <NuiSensor.h>


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

public:
    Application();
    ~Application();

    void run();

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
