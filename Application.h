#pragma once
#include <Windows.h>
#include <Ole2.h>
#define WIN32_LEAN_AND_MEAN

#include <NuiApi.h>
#include <NuiSensor.h>

#include <SFML/Graphics/RenderWindow.hpp>

#include <fstream>
#include <string>

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
    bool showColor;
    bool showDepth;

    // Kinect vars
    HANDLE colorStream;
    HANDLE depthStream;
    HANDLE nextSkeletonEvent;
    bool saving;

    int numSensors;
    INuiSensor *sensor;

    enum EKinectDataType { COLOR, DEPTH };

    // File
    static const std::string saveFileName;
    std::ofstream saveStream;
    std::ifstream loadStream;

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

    void toggleSave();
    void toggleShowColor() { showColor = !showColor; }
    void toggleShowDepth() { showDepth = !showDepth; }

    std::wstring showFileChooser();

    bool isSaving() const { return saving; }
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

    void drawKinectData();
    void getKinectData(GLubyte *data, const EKinectDataType &dataType);

    void updateSkeleton();
    void skeletonFrameReady(NUI_SKELETON_FRAME *skeletonFrame);
    void drawTrackedSkeletonJoints(const NUI_SKELETON_DATA& skeleton);
    void drawSkeletonPosition(const Vector4& position);
    void drawBone(const NUI_SKELETON_DATA& skeleton
                , NUI_SKELETON_POSITION_INDEX jointFrom
                , NUI_SKELETON_POSITION_INDEX jointTo);

};
