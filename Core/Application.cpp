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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
#include "Constants.h"
#include "Util/RenderUtils.h"
#include "Util/ImageManager.h"

const sf::VideoMode Application::videoMode = sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP);
const std::string Application::saveFileName("../../Res/Out/joint_frames.bin");


// ----------------------------------------------------------------------------
Application::Application()
    : window(Application::videoMode, "Kinect Testbed")
    , clock()
    , gui()
    , colorTextureId(0)
    , depthTextureId(0)
    , colorData(new GLubyte[STREAM_WIDTH * STREAM_HEIGHT * 4])
    , depthData(new GLubyte[STREAM_WIDTH * STREAM_HEIGHT * 4])
    , colorStream()
    , depthStream()
    , nextSkeletonEvent()
    , numSensors(-1)
    , sensor(nullptr)
    , jointFrameIndex(0)
    , jointFrameVis(&currentJoints)
    , currentJoints()
    , jointPositionFrames()
    , loaded(false)
    , saving(false)
    , showColor(true)
    , showDepth(true)
    , showSkeleton(true)
    , rightMouseDown(false)
    , skeletonRenderFlags(POS | JOINTS | ORIENT | BONES)
    , filterLevel(OFF)
    , saveStream()
    , loadStream()
    , cameray(constants::initial_camera_y)
    , cameraz(constants::initial_camera_z)
{}

Application::~Application()
{
    delete[] colorData;
    delete[] depthData;

    if (saveStream.is_open()) saveStream.close();
    if (loadStream.is_open()) loadStream.close();
}

void Application::startup()
{
    ImageManager::get().addResourceDir("../../Res/");

    clock.restart();
    if (!initKinect())
        std::cerr << "Unable to initialize Kinect." << std::endl;
    else
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
            saveStream.open(saveFileName, std::ios::binary | std::ios::app);
    } else {
        if (saveStream.is_open())
            saveStream.close();
    }
}

void Application::setJointIndex(const float fraction)
{
    if (!loaded) return;
    assert(fraction >= 0.f && fraction <= 1.f);

    jointFrameIndex = static_cast<int>(floor(fraction * jointPositionFrames.size()));
    jointFrameVis   = &jointPositionFrames[jointFrameIndex];
    gui.setIndex(jointFrameIndex);
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

        if (event.type == sf::Event::KeyPressed) {
            if      (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) moveToNextFrame();
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  moveToPreviousFrame();
        }

        if (event.type == sf::Event::MouseButtonPressed) {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) rightMouseDown = true;
        }
        if (event.type == sf::Event::MouseButtonReleased) {
            if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) rightMouseDown = false;
        }

        if (event.type == sf::Event::MouseWheelMoved) {
            cameraz -= event.mouseWheel.delta;
        }
    }
}

void Application::draw()
{
    window.setActive();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
        glTranslatef(0, -cameray, -cameraz);
        glRotatef(getCameraRotation(), 0.f, 1.f, 0.f);

        Render::ground();
        Render::basis();

        drawKinectSkeletonFrame();
        drawKinectCameraFrame();
    glPopMatrix();

    gui.draw(window);

    window.display();
}

float Application::getCameraRotation()
{
    static const int middleX = WINDOW_WIDTH / 2;
    static float rot = 0.f;
    static float lastTime = clock.getElapsedTime().asSeconds();
    float thisTime = clock.getElapsedTime().asSeconds();
    float delta    = thisTime - lastTime;
    if (delta > 0.05f) {
        if (rightMouseDown) {
            const int currX = sf::Mouse::getPosition(window).x;
            const int delta = currX - middleX;
            const int thresh = 30;
            if (delta < -thresh || delta > thresh) {
                rot += static_cast<int>(5.f * ((float)delta / (float)middleX));
                if (rot > 360) rot = 0;
            }
        }
        lastTime = thisTime;
    }
    return rot;
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
            //MessageBox(NULL, (LPWSTR)(ss.str().c_str()), _T("Selection"), MB_OK);
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
    glPointSize(10.f);
    glEnable(GL_POINT_SMOOTH);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 1, -1);
    GLfloat zNear  = 1.0f;
    GLfloat zFar   = 100.0f;
    GLfloat aspect = float(WINDOW_WIDTH)/float(WINDOW_HEIGHT);
    GLfloat fH = tan( 66.f / 360.0f * 3.14159f ) * zNear;
    GLfloat fW = fH * aspect;
    glFrustum( -fW, fW, -fH, fH, zNear, zFar );
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

void Application::updateKinectCameraTextures()
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

void Application::drawKinectCameraFrame()
{
    // Get kinect frame and update textures
    if (showColor || showDepth) {
        updateKinectCameraTextures();

        // Draw color and depth images
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(3.75f,2.25f,-5.f); // fix in upper right corner of window
        glDisable(GL_CULL_FACE);
        if (showColor) {
            glBindTexture(GL_TEXTURE_2D, colorTextureId);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex3f(0.f, -1.f, 0.f);
            glTexCoord2f(1, 1); glVertex3f(2.f, -1.f, 0.f);
            glTexCoord2f(1, 0); glVertex3f(2.f,  1.f, 0.f);
            glTexCoord2f(0, 0); glVertex3f(0.f,  1.f, 0.f);
            glEnd();
        }
        if (showDepth) {
            glBindTexture(GL_TEXTURE_2D, depthTextureId);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex3f(-2.f, -1.f, 0.f);
            glTexCoord2f(1, 1); glVertex3f( 0.f, -1.f, 0.f);
            glTexCoord2f(1, 0); glVertex3f( 0.f,  1.f, 0.f);
            glTexCoord2f(0, 0); glVertex3f(-2.f,  1.f, 0.f);
            glEnd();
        }
        glEnable(GL_CULL_FACE);
        glBindTexture(GL_TEXTURE_2D, 0);
        glPopMatrix();
    }
}

void Application::checkForSkeletonFrame()
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
    // Get data for the first tracked skeleton
    const NUI_SKELETON_DATA *skeleton = nullptr;
    for (auto i = 0; i < NUI_SKELETON_COUNT; ++i) {
        if (skeletonFrame->SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED) {
            skeleton = &skeletonFrame->SkeletonData[i];
            break;
        }
    }
    if (skeleton == nullptr) {
        //std::cerr << "Warning: unable to find a tracked skeleton." << std::endl;
        return;
    }

    // Filter skeleton frame? 
    switch (filterLevel) {
        case OFF:    break;
        case LOW:    NuiTransformSmooth(skeletonFrame, constants::joint_smooth_params_low);  break;
        case MEDIUM: NuiTransformSmooth(skeletonFrame, constants::joint_smooth_params_med);  break;
        case HIGH:   NuiTransformSmooth(skeletonFrame, constants::joint_smooth_params_high); break;
    }

    // Stash current time for timestamps
    const float timestamp = clock.getElapsedTime().asSeconds();

    // Get bone orientations for this skeleton's joints
    NUI_SKELETON_BONE_ORIENTATION boneOrientations[NUI_SKELETON_POSITION_COUNT];
    NuiSkeletonCalculateBoneOrientations(skeleton, boneOrientations);

    // For each joint
    for (auto i = 0; i < NUI_SKELETON_POSITION_COUNT; ++i) {
        const NUI_SKELETON_POSITION_INDEX&          jointIndex    = static_cast<NUI_SKELETON_POSITION_INDEX>(i);
        const NUI_SKELETON_BONE_ORIENTATION&        orientation   = boneOrientations[jointIndex];
        const NUI_SKELETON_POSITION_TRACKING_STATE& trackingState = skeleton->eSkeletonPositionTrackingState[jointIndex];

        const Vector4& jointPosition = skeleton->SkeletonPositions[jointIndex];

        // Update the entry 
        struct joint &joint = currentJoints[jointIndex];
        joint.timestamp     = timestamp;
        joint.index         = jointIndex;
        joint.position.x    = jointPosition.x;
        joint.position.y    = jointPosition.y;
        joint.position.z    = jointPosition.z;
        joint.trackState    = trackingState;
        joint.orientation   = orientation;

        // Save the frame if appropriate
        if (saving && saveStream.is_open()) {
            saveStream.write((char *)&joint, sizeof(struct joint));
        }
    }
}

void Application::drawKinectSkeletonFrame()
{
    if (showSkeleton) {
        checkForSkeletonFrame();
        drawSkeletonFrame();
    }
}

void Application::drawSkeletonFrame()
{
    assert(jointFrameVis);

    glPushMatrix();
    glTranslatef(0,1,-1.5); // Move closer to origin, default z is dist from kinect [0.8m, 4m]

    // Rendering flags == [POS | JOINTS | ORIENT | BONES | INFER]
    // ----------------------------------------------------------
    // POS    = whole skeleton position
    // JOINTS = skeleton joints
    // ORIENT = skeleton joint orientations
    // BONES  = connections between skeleton joints
    // INFER  = draw inferred joints/bones

    if (skeletonRenderFlags & POS) {
        // TODO: - draw skeleton position (hip center)
		drawSkeletonPosition();
    }
    if (skeletonRenderFlags & JOINTS) {
        // Draw each joint
        glPointSize(8.f);
        glBegin(GL_POINTS);
        for (auto i = 0; i < NUI_SKELETON_POSITION_COUNT; ++i) {
            const struct joint &joint = (*jointFrameVis)[static_cast<NUI_SKELETON_POSITION_INDEX>(i)];
            switch (joint.trackState) { // skip untracked joints
                case NUI_SKELETON_POSITION_NOT_TRACKED: continue;
                case NUI_SKELETON_POSITION_TRACKED:
                    glColor3f(1.f, 1.f, 1.f);
                break;
                case NUI_SKELETON_POSITION_INFERRED:
                    if (skeletonRenderFlags & INFER) glColor3f(1.f, 0.5f, 0.f);
                    else                             continue;
                break;
            }
            glVertex3fv(glm::value_ptr(joint.position));
        }
        glEnd();
    }
    if (skeletonRenderFlags & ORIENT) {
        drawOrientations();
    }
    if (skeletonRenderFlags & BONES) {
        drawBones();
    }

    glColor3f(1.f, 1.f, 1.f);
    glPopMatrix();
}

void Application::drawSkeletonPosition()
{
	glPushMatrix();

	glColor3f(1.f, 0.f, 1.f);

	glPointSize(20.f);
	glBegin(GL_POINTS);
		const struct joint &joint = (*jointFrameVis)[NUI_SKELETON_POSITION_HIP_CENTER];
		glVertex3fv(glm::value_ptr(joint.position));
	glEnd();
	glPointSize(1.f);

	glPopMatrix();
}

void Application::drawOrientations()
{
    // Draw each joint's orientation
    glPointSize(1.f);
    for (auto i = 0; i < NUI_SKELETON_POSITION_COUNT; ++i) {
        const struct joint &joint = (*jointFrameVis)[static_cast<NUI_SKELETON_POSITION_INDEX>(i)];
        float scale = 0.075f;
        switch (joint.trackState) { // skip untracked joints
        case NUI_SKELETON_POSITION_NOT_TRACKED: continue;
        case NUI_SKELETON_POSITION_INFERRED:
            if (skeletonRenderFlags & INFER) scale = 0.05f;
            else                             continue; 
            break;
        }
        const Matrix4&  m = joint.orientation.absoluteRotation.rotationMatrix;
        const glm::vec3 x(m.M11, m.M12, m.M13);
        const glm::vec3 y(m.M21, m.M22, m.M23);
        const glm::vec3 z(m.M31, m.M32, m.M33);
        Render::basis(scale, joint.position, glm::normalize(x), glm::normalize(y), glm::normalize(z));
    }
}

void Application::drawBones()
{
    // Render head and shoulders
    drawBone(NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER);
    drawBone(NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT);
    drawBone(NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT);

    // Render left arm
    drawBone(NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT);
    drawBone(NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT);
    drawBone(NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT);

    // Render right arm
    drawBone(NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT);
    drawBone(NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT);
    drawBone(NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT);

    // Render torso
    drawBone(NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE);
    drawBone(NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER);
    drawBone(NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT);
    drawBone(NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT);

    // Render left leg
    drawBone(NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT);
    drawBone(NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT);
    drawBone(NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT);

    // Render right leg
    drawBone(NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT);
    drawBone(NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT);
    drawBone(NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT);
}

void Application::drawBone( NUI_SKELETON_POSITION_INDEX jointFrom , NUI_SKELETON_POSITION_INDEX jointTo )
{
    NUI_SKELETON_POSITION_TRACKING_STATE jointFromState = (*jointFrameVis)[jointFrom].trackState;
    NUI_SKELETON_POSITION_TRACKING_STATE jointToState   = (*jointFrameVis)[jointTo].trackState;
    if (jointFromState == NUI_SKELETON_POSITION_NOT_TRACKED || jointToState == NUI_SKELETON_POSITION_NOT_TRACKED)
        return; // nothing to draw, one joint not tracked

    const glm::vec3& fromPosition((*jointFrameVis)[jointFrom].position);
    const glm::vec3& toPosition((*jointFrameVis)[jointTo].position);

    // Don't draw if both points are inferred
    if (jointFromState == NUI_SKELETON_POSITION_INFERRED && jointToState == NUI_SKELETON_POSITION_INFERRED) {
        return;
    }

    if (jointFromState == NUI_SKELETON_POSITION_INFERRED || jointToState == NUI_SKELETON_POSITION_INFERRED) {
        glLineWidth(1.f);
        // Draw thin red lines if either side is inferred
        glColor3f(1.f, 0.f, 0.f);
        glBegin(GL_LINES);
            glVertex3fv(glm::value_ptr(fromPosition));
            glVertex3fv(glm::value_ptr(toPosition));
        glEnd();
        glColor3f(1.f, 1.f, 1.f);
    }

    // Assume all drawn bones are inferred unless BOTH joints are tracked
    if (jointFromState == NUI_SKELETON_POSITION_TRACKED && jointToState == NUI_SKELETON_POSITION_TRACKED) {
        // TODO: draw thick lines between joints
        glLineWidth(8.f);
        glColor3f(0.f, 1.f, 0.f);
        glBegin(GL_LINES);
            glVertex3fv(glm::value_ptr(fromPosition));
            glVertex3fv(glm::value_ptr(toPosition));
        glEnd();
        glColor3f(1.f, 1.f, 1.f);
    }

    glLineWidth(1.f);
}

void Application::loadFile()
{
    if (loaded) {
        jointPositionFrames.clear();
    }

    // Get the file name from a file chooser dialog and try to load it
    std::wstring filename(showFileChooser());
    std::wcout << "Selected file: " << filename << std::endl;

    sf::Vector3f mn( 1e30f,  1e30f,  1e30f);
    sf::Vector3f mx(-1e30f, -1e30f, -1e30f);

    // Open the file
    if (loadStream.is_open()) loadStream.close();
    loadStream.open(filename, std::ios::binary | std::ios::in);
    if (loadStream.is_open()) {
        std::wcout << L"Opened file: " << filename   << std::endl
                   << L"Loading joints positions..." << std::endl;

        JointPosFrame inputJointFrame;    
        struct joint joint;
        int numJointsRead = 0, totalJointsRead = 0, totalFramesRead = 0;
        while (loadStream.good()) {
            memset(&joint, 0, sizeof(struct joint));
            loadStream.read((char *)&joint, sizeof(struct joint));
            inputJointFrame[joint.index] = joint;
            ++totalJointsRead;
            
            mn.x = std::min(mn.x, joint.position.x);
            mn.y = std::min(mn.y, joint.position.y);
            mn.z = std::min(mn.z, joint.position.z);

            mx.x = std::max(mx.x, joint.position.x);
            mx.y = std::max(mx.y, joint.position.y);
            mx.z = std::max(mx.z, joint.position.z);

            if (++numJointsRead == NUI_SKELETON_POSITION_COUNT) {
                numJointsRead = 0; 
                ++totalFramesRead;
                jointPositionFrames.push_back(inputJointFrame);
            }
        }

        loadStream.close();
        loaded = true;
        std::wstringstream ss;
        ss << _T("Done loading '") << filename << _T("'");
        MessageBox(NULL,ss.str().c_str(),_T("Open"),MB_OK);
    }

    std::cout << "min,max = (" << mn.x << "," << mn.y << "," << mn.z << ")"
              <<        " , (" << mx.x << "," << mx.y << "," << mx.z << ")"
              << std::endl;

    // Normalize the z values for each joint in each frame
    for (auto frame : jointPositionFrames) {
        for (auto joints : frame) {
            struct joint &joint = joints.second;
            joint.position.z /= mx.z;
        }
    }

    // Set the current frame as the middle frame of the sequence
    jointFrameIndex = jointPositionFrames.size() / 2;
    jointFrameVis   = &jointPositionFrames[jointFrameIndex];
    std::string name; name.assign(filename.begin(), filename.end());
    gui.setFileName(name);
    gui.setProgress((float) jointFrameIndex / (float) jointPositionFrames.size());
    gui.setIndex(jointFrameIndex);
}

void Application::moveToNextFrame()
{
    if (!loaded) return;
    
    const int nextFrame = jointFrameIndex + 1;
    const int numFrames = jointPositionFrames.size();
    if (nextFrame >= 0 && nextFrame < numFrames) {
        jointFrameIndex = nextFrame;
        jointFrameVis   = &jointPositionFrames[jointFrameIndex];
        gui.setProgress((float) jointFrameIndex / (float) (numFrames - 1));
        gui.setIndex(jointFrameIndex);
    }
}

void Application::moveToPreviousFrame()
{
    if (!loaded) return;

    const int prevFrame = jointFrameIndex - 1;
    const int numFrames = jointPositionFrames.size();
    if (prevFrame >= 0 && prevFrame < numFrames) {
        jointFrameIndex = prevFrame;
        jointFrameVis   = &jointPositionFrames[jointFrameIndex];
        gui.setProgress((float) jointFrameIndex / (float) (numFrames - 1));
        gui.setIndex(jointFrameIndex);
    }
}
