#pragma once
#include <Windows.h>
#include <Ole2.h>
#define WIN32_LEAN_AND_MEAN

#include <NuiApi.h>
#include <NuiSensor.h>

#include <SFML/Graphics/RenderWindow.hpp>

#include <fstream>
#include <string>

#include "UI/UserInterface.h"
#include "Core/Config.h"


struct joint {
    float timestamp; // in seconds
    sf::Vector3f position;
    NUI_SKELETON_POSITION_INDEX index;
    NUI_SKELETON_BONE_ORIENTATION boneOrientation;
    NUI_SKELETON_POSITION_TRACKING_STATE trackState;
};

typedef std::map<NUI_SKELETON_POSITION_INDEX, struct joint> JointPosFrame;
typedef std::vector<JointPosFrame> JointPositionFrames;


class Application
{
private:
    static const sf::VideoMode videoMode;

    // SFML/SFGUI vars
    sf::Clock clock;
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
    HANDLE nextSkeletonEvent;
    INuiSensor *sensor;
    int numSensors;
    int jointFrameIndex;
    JointPosFrame *jointFrameVis;
    JointPosFrame currentJoints;
    JointPositionFrames jointPositionFrames;
    enum EKinectDataType { COLOR, DEPTH };

    // State
    bool loaded;
    bool saving;
    bool showColor;
    bool showDepth;
    bool showJoints;

    float cameraz;

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
    void toggleShowColor()  { showColor  = !showColor; }
    void toggleShowDepth()  { showDepth  = !showDepth; }
    void toggleShowJoints() { showJoints = !showJoints; }

    void loadFile();
    void setJointIndex(const float fraction);

    bool isSaving() const { return saving; }
    int getNumSensors() const { return numSensors; }
    const sf::Vector2i getMousePosition() const { return sf::Mouse::getPosition(window); }

private:
    // Main methods
    void mainLoop();
    void processEvents();
    void draw();

    std::wstring showFileChooser();

    // OpenGL methods
    void initOpenGL();
    void shutdownOpenGL();

    // Kinect methods -------------------------------------
    bool initKinect();

    // Kinect image methods
    void updateKinectCameraTextures();
    void getKinectData(GLubyte *data, const EKinectDataType &dataType);

    // Kinect skeleton methods
    void checkForSkeletonFrame();
    void skeletonFrameReady(NUI_SKELETON_FRAME *skeletonFrame);
    void drawSkeletonFrame();

    void drawTrackedSkeletonJoints(const NUI_SKELETON_DATA& skeleton);
    void drawSkeletonPosition(const Vector4& position);
    void drawBone(const NUI_SKELETON_DATA& skeleton
                , NUI_SKELETON_POSITION_INDEX jointFrom
                , NUI_SKELETON_POSITION_INDEX jointTo);

    void moveToNextFrame();
    void moveToPreviousFrame();
};
