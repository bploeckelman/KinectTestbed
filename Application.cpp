#include <Windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <Ole2.h>

#define WIN32_LEAN_AND_MEAN
#define _WIN32_DCOM

#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>

#include <NuiApi.h>
#include <NuiSensor.h>
#include <NuiImageCamera.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <stdio.h>
#include <tchar.h>

#include "Application.h"
#include "Config.h"

const sf::VideoMode Application::videoMode = sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP);
const std::string Application::saveFileName("joints.txt");


// ----------------------------------------------------------------------------
Application::Application()
    : window(Application::videoMode, "Kinect Testbed")
    , clock()
    , gui()
    , colorTextureId(0)
    , depthTextureId(0)
    , colorData(new GLubyte[STREAM_WIDTH * STREAM_HEIGHT * 4])
    , depthData(new GLubyte[STREAM_WIDTH * STREAM_HEIGHT * 4])
    , showColor(true)
    , showDepth(true)
    , colorStream()
    , depthStream()
    , nextSkeletonEvent()
    , numSensors(-1)
    , sensor(nullptr)
    , saving(false)
    , saveStream()
    , loadStream()
{}

Application::~Application()
{
    delete[] colorData;
    delete[] depthData;

    if (saveStream.is_open())
        saveStream.close();
    if (loadStream.is_open())
        loadStream.close();
}

void Application::startup()
{
    clock.restart();
    if (!initKinect()) {
        std::cerr << "Unable to initialize Kinect." << std::endl;
        exit(1);
    } else {
        initJointMap();
    }
    std::cout << "Kinect initialized in " << clock.getElapsedTime().asSeconds() << " seconds." << std::endl;

    clock.restart();    
    initOpenGL();
    mainLoop();
    shutdownOpenGL();
}

void Application::shutdown()
{
    window.close();
}

void Application::toggleSave()
{
    if (saving) {
        std::cout << "Stopped saving joint data." << std::endl;
    } else {
        std::cout << "Started saving joint data." << std::endl;
    }

    saving = !saving; 

    if (saving) {
        if (!saveStream.is_open())
            saveStream.open(saveFileName, std::ios::app);
    } else {
        if (saveStream.is_open())
            saveStream.close();
    }
}

// ----------------------------------------------------------------------------
void Application::mainLoop()
{
    while (window.isOpen()) {
        processEvents();
        draw();
    }
}

void Application::processEvents()
{
    sf::Event event;
    while (window.pollEvent(event)) {
        gui.handleEvent(event);

        if ((event.type == sf::Event::KeyPressed && sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
         || (event.type == sf::Event::Closed)) {
            window.close();
        }
    }
}

void Application::draw()
{
    window.setActive();

    // Get kinect frame and update textures
    drawKinectData();

    // Draw color and depth images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (showColor) {
        glBindTexture(GL_TEXTURE_2D, colorTextureId);
        glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
            glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH / 2, 0, 0);
            glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH / 2, (float) WINDOW_HEIGHT, 0);
            glTexCoord2f(0, 1); glVertex3f(0, (float) WINDOW_HEIGHT, 0);
        glEnd();
    }

    if (showDepth) {
        glBindTexture(GL_TEXTURE_2D, depthTextureId);
        glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex3f(WINDOW_WIDTH / 2, 0, 0);
            glTexCoord2f(1, 0); glVertex3f((float) WINDOW_WIDTH, 0, 0);
            glTexCoord2f(1, 1); glVertex3f((float) WINDOW_WIDTH, (float) WINDOW_HEIGHT, 0);
            glTexCoord2f(0, 1); glVertex3f(WINDOW_WIDTH / 2, (float) WINDOW_HEIGHT, 0);
        glEnd();
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    // Work the skeleton
    updateSkeleton();
    drawJoints();

    gui.draw(window);

    window.display();
}


// ----------------------------------------------------------------------------
std::wstring Application::showFileChooser() {
    BROWSEINFO      bi;
    char            pszBuffer[MAX_PATH]; 
    LPWSTR          buffer = (LPWSTR)pszBuffer;
    LPITEMIDLIST    pidl; 
    LPMALLOC        lpMalloc;

    // Initialize COM
    if(CoInitializeEx(0,COINIT_APARTMENTTHREADED) != S_OK)
    {
        MessageBox(NULL,_T("Error opening browse window"),_T("ERROR"),MB_OK);
        CoUninitialize();
        return std::wstring();
    }

    // Get a pointer to the shell memory allocator
    if(SHGetMalloc(&lpMalloc) != S_OK)
    {
        MessageBox(NULL,_T("Error opening browse window"),_T("ERROR"),MB_OK);
        CoUninitialize();
        return std::wstring();
    }

    bi.hwndOwner        =   NULL; 
    bi.pidlRoot         =   NULL;
    bi.pszDisplayName   =   buffer;
    bi.lpszTitle        =   _T("Select a joint data file"); 
    bi.ulFlags          =   BIF_BROWSEINCLUDEFILES;
    bi.lpfn             =   NULL; 
    bi.lParam           =   0;

    if(pidl = SHBrowseForFolder(&bi))
    {
        // Copy the path directory to the buffer
        if(SHGetPathFromIDList(pidl, buffer))
        {
            // buf now holds the directory path
            std::wstringstream ss;
            ss << _T("You selected the directory: ") << std::endl << _T("\t") << buffer << std::endl;
            MessageBox(NULL, (LPWSTR)(ss.str().c_str()), _T("Selection"), MB_OK);
        }

        lpMalloc->Free(pidl);
    }
    lpMalloc->Release();
    CoUninitialize();

    return buffer;
}

void Application::initOpenGL(){
    glGenTextures(1, &colorTextureId);
    glBindTexture(GL_TEXTURE_2D, colorTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, STREAM_WIDTH, STREAM_HEIGHT, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid *) colorData);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &depthTextureId);
    glBindTexture(GL_TEXTURE_2D, depthTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, STREAM_WIDTH, STREAM_HEIGHT, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid *) depthData);
    glBindTexture(GL_TEXTURE_2D, 0);

    glClearColor(0,0,0,0);
    glClearDepth(1.f);
    glEnable(GL_TEXTURE_2D);
    glPointSize(20.f);
    glEnable(GL_POINT_SMOOTH);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 1, -1);
    //GLfloat zNear = 1.0f;
    //GLfloat zFar = -1.0f;
    //GLfloat aspect = float(WINDOW_WIDTH)/float(WINDOW_HEIGHT);
    //GLfloat fH = tan( 66.f / 360.0f * 3.14159f ) * zNear;
    //GLfloat fW = fH * aspect;
    //glFrustum( -fW, fW, -fH, fH, zNear, zFar );
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Application::shutdownOpenGL() {
    glDeleteTextures(1, &colorTextureId);
    glDeleteTextures(1, &depthTextureId);
}

// ----------------------------------------------------------------------------
bool Application::initKinect() {
    // TODO: check return values
    // TODO: handle multiple sensors
    NuiGetSensorCount(&numSensors);
    if (numSensors < 0 || numSensors < 1) {
        std::cerr << "Unable to find Kinect sensors." << std::endl;
        return false;
    }
    std::stringstream ss;
    ss << "Sensors = " << numSensors;
    gui.setInfo(ss.str());

    NuiCreateSensorByIndex(0, &sensor);
    if (sensor < 0) {
        std::cerr << "Unable to create Kinect sensor 0." << std::endl;
        return false;
    }

    // Initialize sensor
    HRESULT hr;
    hr = sensor->NuiInitialize(
          NUI_INITIALIZE_FLAG_USES_DEPTH
        | NUI_INITIALIZE_FLAG_USES_COLOR
        | NUI_INITIALIZE_FLAG_USES_SKELETON);
    if (SUCCEEDED(hr)) {
        // Open a color stream to receive color data
        sensor->NuiImageStreamOpen(
              NUI_IMAGE_TYPE_COLOR
            , NUI_IMAGE_RESOLUTION_640x480 
            , 0    // Image stream flags, eg. near mode...
            , 2    // Number of frames to buffer
            , NULL // Event handle
            , &colorStream);

        // Open a depth stream to receive depth data
        sensor->NuiImageStreamOpen(
              NUI_IMAGE_TYPE_DEPTH
            , NUI_IMAGE_RESOLUTION_640x480 
            , 0    // Image stream flags, eg. near mode...
            , 2    // Number of frames to buffer
            , NULL // Event handle
            , &depthStream);

        // Create an event that will be signaled when skeleton data is available
        nextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

        // Open a skeleton stream to receive skeleton data
        hr = sensor->NuiSkeletonTrackingEnable(nextSkeletonEvent, 0);
        if (!SUCCEEDED(hr)) {
            std::cerr << "Unable to enable KInect skeleton tracking." << std::endl;
        }

        // Get sensor device data
        BSTR bs = sensor->NuiDeviceConnectionId();
        const std::wstring wstr(bs, SysStringLen(bs));
        std::string deviceId;
        deviceId.assign(wstr.begin(), wstr.end());
        std::stringstream ss;
        ss << "Sensor [" << numSensors << "] : '" << deviceId << "'" << std::endl;
        gui.setInfo(ss.str());
        std::cout << "Kinect Device Id: " << deviceId << std::endl;
    } else {
        std::cerr << "Unable to initialize Kinect NUI." << std::endl;
    }

    return (sensor != nullptr);
}

void Application::drawKinectData()
{
    glBindTexture(GL_TEXTURE_2D, colorTextureId);
    getKinectData(colorData, COLOR);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, STREAM_WIDTH, STREAM_HEIGHT, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid *) colorData);

    glBindTexture(GL_TEXTURE_2D, depthTextureId);
    getKinectData(depthData, DEPTH);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, STREAM_WIDTH, STREAM_HEIGHT, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid *) depthData);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Application::getKinectData(GLubyte *dest, const EKinectDataType &dataType) 
{
    NUI_IMAGE_FRAME imageFrame;
    NUI_LOCKED_RECT lockedRect;
    INuiFrameTexture *texture = nullptr;

    if (dataType == COLOR) {
        if (sensor->NuiImageStreamGetNextFrame(colorStream, 0, &imageFrame) < 0) {
            //std::cerr << "Unable to get next stream frame from Kinect." << std::endl;
            return;
        }

        texture = imageFrame.pFrameTexture;
        texture->LockRect(0, &lockedRect, NULL, 0);

        // Copy image data to destination buffer
        if (lockedRect.Pitch != 0) {
            const BYTE *curr = (const BYTE *) lockedRect.pBits;
            const BYTE *dataEnd = curr + (STREAM_WIDTH * STREAM_HEIGHT) * 4;
            while (curr < dataEnd) *dest++ = *curr++;
        }

        // Kinect data is in BGRA format, so we can copy it
        // directly into a buffer and use it as an OpenGL texture
        texture->UnlockRect(0);
        sensor->NuiImageStreamReleaseFrame(colorStream, &imageFrame);
    }

    else if (dataType == DEPTH) {
        if (sensor->NuiImageStreamGetNextFrame(depthStream, 0, &imageFrame) < 0) {
            //std::cerr << "Unable to get next stream frame from Kinect." << std::endl;
            return;
        }

        texture = imageFrame.pFrameTexture;
        texture->LockRect(0, &lockedRect, NULL, 0);

        // Copy depth data to destination buffer
        if (lockedRect.Pitch != 0) {
            const USHORT *curr = (const USHORT *) lockedRect.pBits;
            const USHORT *dataEnd = curr + (STREAM_WIDTH * STREAM_HEIGHT);
            while (curr < dataEnd) {
                // Get depth in millimeters
                USHORT depth = NuiDepthPixelToDepth(*curr++);

                // Draw a grayscale image of the depth...
                for (int i = 0; i < 3; ++i)
                    *dest++ = (BYTE) depth % 256; // B,G,R = depth % 256
                *dest++ = 0xff;	                  // Alpha = 1
            }
        }

        // Kinect data is in BGRA format, so we can copy it
        // directly into a buffer and use it as an OpenGL texture
        texture->UnlockRect(0);
        sensor->NuiImageStreamReleaseFrame(depthStream, &imageFrame);
    }
}

void Application::updateSkeleton()
{
    // Wait for 0ms to quickly test if it is time to process a skeleton
    if (WAIT_OBJECT_0 == WaitForSingleObject(nextSkeletonEvent, 0)) {
        NUI_SKELETON_FRAME skeletonFrame = {0};
        // Get the skeleton frame that is ready
        if (SUCCEEDED(sensor->NuiSkeletonGetNextFrame(0, &skeletonFrame))) {
            // Process the skeleton frame
            skeletonFrameReady(&skeletonFrame);
        } 
    }
}

void Application::skeletonFrameReady(NUI_SKELETON_FRAME *skeletonFrame)
{
    for (int i = 0; i < NUI_SKELETON_COUNT; ++i) {
        const NUI_SKELETON_DATA& skeleton = skeletonFrame->SkeletonData[i];

        // TODO: handle more than 1 skeleton
        // Update the current positions for drawing
        if (skeleton.eTrackingState == NUI_SKELETON_TRACKING_STATE::NUI_SKELETON_TRACKED) {
            updateSkeletonJoints(skeleton);
        }

        //switch (skeleton.eTrackingState) {
        //    case NUI_SKELETON_TRACKED:
        //        drawTrackedSkeletonJoints(skeleton);
        //    break;
        //    case NUI_SKELETON_POSITION_ONLY:
        //        drawSkeletonPosition(skeleton.Position);
        //    break;
        //}
    }
}

void Application::drawTrackedSkeletonJoints(const NUI_SKELETON_DATA& skeleton)
{
    // Render head and shoulders
    drawBone(skeleton, NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER);
    drawBone(skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT);
    drawBone(skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT);

    // Render left arm
    drawBone(skeleton, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT);
    drawBone(skeleton, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT);
    drawBone(skeleton, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT);

    // Render right arm
    drawBone(skeleton, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT);
    drawBone(skeleton, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT);
    drawBone(skeleton, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT);

    // Render torso
    drawBone(skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE);
    drawBone(skeleton, NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER);
    drawBone(skeleton, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT);
    drawBone(skeleton, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT);

    // Render other bones...
    // TODO
}

void Application::drawSkeletonPosition(const Vector4& position)
{
    // TODO
}

void Application::drawJoints()
{
    static const float Z = 0.5f;

    // TODO: store tracking/inferral info for each joint and use it to update the color
    glBegin(GL_POINTS);
    glColor3f(0.f, 1.f, 0.f);
    // Draw each joint
    for (auto i = 0; i < _NUI_SKELETON_POSITION_INDEX::NUI_SKELETON_POSITION_COUNT; ++i) {
        const struct joint &joint = joints[static_cast<_NUI_SKELETON_POSITION_INDEX>(i)];
        glVertex3f(joint.position.x *  WINDOW_WIDTH  / 3 + 512.f
                 , joint.position.y * -WINDOW_HEIGHT / 2 + 256.f
                 , Z);//jointFromPosition.z);
    }
    glEnd();
    glColor3f(1.f, 1.f, 1.f);
}

void Application::drawBone(const NUI_SKELETON_DATA& skeleton
                         , NUI_SKELETON_POSITION_INDEX jointFrom
                         , NUI_SKELETON_POSITION_INDEX jointTo)
{
    NUI_SKELETON_POSITION_TRACKING_STATE jointFromState = skeleton.eSkeletonPositionTrackingState[jointFrom];
    NUI_SKELETON_POSITION_TRACKING_STATE jointToState   = skeleton.eSkeletonPositionTrackingState[jointTo];
    if (jointFromState == NUI_SKELETON_POSITION_NOT_TRACKED || jointToState == NUI_SKELETON_POSITION_NOT_TRACKED) {
        return; // nothing to draw, one joint not tracked
    }

    const sf::Vector3f& jointFromPosition(joints[jointFrom].position);//skeleton.SkeletonPositions[jointFrom];
    const sf::Vector3f& jointToPosition(joints[jointTo].position);//skeleton.SkeletonPositions[jointTo];
    static const float Z = 1.f;

    // Don't draw if both points are inferred

    // TODO: draw thinner lines if either side is inferred
    if (jointFromState == NUI_SKELETON_POSITION_INFERRED || jointToState == NUI_SKELETON_POSITION_INFERRED) {
        glColor3f(1.f, 0.f, 0.f);
        //glBegin(GL_POINTS);
        glBegin(GL_LINES);
            glVertex3f(jointFromPosition.x * WINDOW_WIDTH / 3 + 512.f, jointFromPosition.y * -WINDOW_HEIGHT / 2 + 256.f, Z);//jointFromPosition.z);
            glVertex3f(jointToPosition.x * WINDOW_WIDTH / 3 + 512.f, jointToPosition.y * -WINDOW_HEIGHT / 2 + 256.f, Z);//jointToPosition.z);
        glEnd();
        glColor3f(1.f, 1.f, 1.f);
    }

    // Assume all drawn bones are inferred unless BOTH joints are tracked
    if (jointFromState == NUI_SKELETON_POSITION_TRACKED && jointToState == NUI_SKELETON_POSITION_TRACKED) {
        // TODO: draw thick lines between joints
        glColor3f(0.f, 1.f, 0.f);
        //glBegin(GL_POINTS);
        glBegin(GL_LINES);
            glVertex3f(jointFromPosition.x * WINDOW_WIDTH / 3 + 512.f, jointFromPosition.y * -WINDOW_HEIGHT / 2 + 256.f, Z);//jointFromPosition.z);
            glVertex3f(jointToPosition.x * WINDOW_WIDTH / 3 + 512.f, jointToPosition.y * -WINDOW_HEIGHT / 2 + 256.f, Z);//jointToPosition.z);
        glEnd();
        glColor3f(1.f, 1.f, 1.f);
    }
}

void Application::initJointMap()
{
    joints.clear();
    // For each joint
    for (auto i = 0; i < _NUI_SKELETON_POSITION_INDEX::NUI_SKELETON_POSITION_COUNT; ++i) {
        // Add an entry 
        joints[static_cast<_NUI_SKELETON_POSITION_INDEX>(i)];
    }
}

void Application::updateSkeletonJoints(const NUI_SKELETON_DATA& skeleton)
{
    // For each joint
    for (auto i = 0; i < _NUI_SKELETON_POSITION_INDEX::NUI_SKELETON_POSITION_COUNT; ++i) {
        const _NUI_SKELETON_POSITION_INDEX &jointIndex = static_cast<_NUI_SKELETON_POSITION_INDEX>(i);
        const Vector4& jointPosition = skeleton.SkeletonPositions[jointIndex];

        // Update the entry 
        struct joint &joint = joints[jointIndex];
        joint.index      = jointIndex;
        joint.position.x = jointPosition.x;
        joint.position.y = jointPosition.y;
        joint.position.z = jointPosition.z;
        joint.timestamp  = clock.getElapsedTime().asSeconds();

        if (saving && saveStream.is_open()) {
            // TODO: save joint struct as binary
            saveStream  << "Joint[" << joint.index << "] "
                        << "@ " << joint.timestamp << ": "
                        << joint.position.x << ","
                        << joint.position.y << ","
                        << joint.position.z << std::endl;
        }
    }
}
