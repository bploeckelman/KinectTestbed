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
#include "UI/UserInterface.h"


struct joint {
    float timestamp; // in seconds
    glm::vec3 position;
    NUI_SKELETON_POSITION_INDEX index;
    NUI_SKELETON_BONE_ORIENTATION orientation;
    NUI_SKELETON_POSITION_TRACKING_STATE trackState;
};

typedef std::map<NUI_SKELETON_POSITION_INDEX, struct joint> JointPosFrame;
typedef std::vector<JointPosFrame> JointPositionFrames;

// Skeleton rendering flags
const byte POS    = 0x01;
const byte JOINTS = 0x02;
const byte INFER  = 0x04;
const byte ORIENT = 0x08;
const byte BONES  = 0x10;
const byte PATH   = 0x20;

enum ESkeletonFilterLevel { OFF, LOW, MEDIUM, HIGH };


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
    ESkeletonFilterLevel filterLevel;
    enum EKinectDataType { COLOR, DEPTH };

    // State
    bool loaded;
    bool saving;
    bool showColor;
    bool showDepth;
    bool showSkeleton;
    bool rightMouseDown;
    bool leftMouseDown;
    bool kinectInitialized;

    byte skeletonRenderFlags;

    float cameray;
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
    void toggleShowColor()     { showColor    = !showColor;    }
    void toggleShowDepth()     { showDepth    = !showDepth;    }
    void toggleShowSkeleton()  { showSkeleton = !showSkeleton; }

    // Rendering flags
    void setRenderFlags(const byte flags) { skeletonRenderFlags  = flags;  }
    void clearRenderFlags()               { skeletonRenderFlags  = 0;      }
    void togglePosition()                 { skeletonRenderFlags ^= POS;    }
    void toggleJoints()                   { skeletonRenderFlags ^= JOINTS; }
    void toggleOrientation()              { skeletonRenderFlags ^= ORIENT; }
    void toggleBones()                    { skeletonRenderFlags ^= BONES;  }
    void toggleInferred()                 { skeletonRenderFlags ^= INFER;  }
    void toggleJointPath()                { skeletonRenderFlags ^= PATH;   }
    void setFilterLevel(const ESkeletonFilterLevel& level) { filterLevel = level; }

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

    float getCameraRotationX();
    float getCameraRotationY();
    std::wstring showFileChooser();

    // OpenGL methods
    void initOpenGL();
    void shutdownOpenGL();

    // Kinect methods -------------------------------------
    bool initKinect();

    // Kinect image methods
    void updateKinectCameraTextures();
    void getKinectData(GLubyte *data, const EKinectDataType &dataType);
    void drawKinectCameraFrame() ;

    // Kinect skeleton methods
    void checkForSkeletonFrame();
    void skeletonFrameReady(NUI_SKELETON_FRAME *skeletonFrame);
    void drawSkeletonFrame();
    void drawJoints();
    void drawOrientations();

    void drawKinectSkeletonFrame();
    void drawBones();
    void drawSkeletonPosition();
    void drawBone(NUI_SKELETON_POSITION_INDEX jointFrom , NUI_SKELETON_POSITION_INDEX jointTo);
    void drawJointPath();

    void moveToNextFrame();
    void moveToPreviousFrame();
};
