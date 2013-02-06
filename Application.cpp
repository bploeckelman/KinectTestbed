#include <Windows.h>
#include <Ole2.h>

#define WIN32_LEAN_AND_MEAN

#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <NuiApi.h>
#include <NuiSensor.h>
#include <NuiImageCamera.h>

#include <iostream>

#include "Application.h"
#include "Config.h"


const sf::VideoMode Application::videoMode = sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP);

// Note: these can't be member vars of Application b/c it causes a 
// stackoverflow on startup before main() is entered
//GLubyte colorData[STREAM_WIDTH * STREAM_HEIGHT * 4];
//GLubyte depthData[STREAM_WIDTH * STREAM_HEIGHT * 4];

// ----------------------------------------------------------------------------
Application::Application()
    : window(Application::videoMode, "Kinect Testbed")
    , gui()
    , colorTextureId(0)
    , depthTextureId(0)
    , colorData(new GLubyte[STREAM_WIDTH * STREAM_HEIGHT * 4])
    , depthData(new GLubyte[STREAM_WIDTH * STREAM_HEIGHT * 4])
    , colorStream()
    , depthStream()
    , numSensors(-1)
    , sensor(nullptr)
{}

Application::~Application()
{
    delete[] colorData;
    delete[] depthData;
}

void Application::run()
{
    if (!initKinect()) {
        std::cerr << "Unable to initialize Kinect." << std::endl;
        exit(1);
    }
    
    initOpenGL();
    mainLoop();
    shutdownOpenGL();
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

    glBindTexture(GL_TEXTURE_2D, colorTextureId);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
        glTexCoord2f(1, 0); glVertex3f(WINDOW_WIDTH / 2, 0, 0);
        glTexCoord2f(1, 1); glVertex3f(WINDOW_WIDTH / 2, (float) WINDOW_HEIGHT, 0);
        glTexCoord2f(0, 1); glVertex3f(0, (float) WINDOW_HEIGHT, 0);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, depthTextureId);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(WINDOW_WIDTH / 2, 0, 0);
        glTexCoord2f(1, 0); glVertex3f((float) WINDOW_WIDTH, 0, 0);
        glTexCoord2f(1, 1); glVertex3f((float) WINDOW_WIDTH, (float) WINDOW_HEIGHT, 0);
        glTexCoord2f(0, 1); glVertex3f(WINDOW_WIDTH / 2, (float) WINDOW_HEIGHT, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);

    gui.draw(window);

    window.display();
}


// ----------------------------------------------------------------------------
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

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 1, -1);
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

    NuiCreateSensorByIndex(0, &sensor);
    if (sensor < 0) {
        std::cerr << "Unable to create Kinect sensor 0." << std::endl;
        return false;
    }

    // Initialize sensor
    sensor->NuiInitialize(
          NUI_INITIALIZE_FLAG_USES_DEPTH
        | NUI_INITIALIZE_FLAG_USES_COLOR);
    sensor->NuiImageStreamOpen(
          NUI_IMAGE_TYPE_COLOR
        , NUI_IMAGE_RESOLUTION_640x480 
        , 0    // Image stream flags, eg. near mode...
        , 2    // Number of frames to buffer
        , NULL // Event handle
        , &colorStream);
    sensor->NuiImageStreamOpen(
          NUI_IMAGE_TYPE_DEPTH
        , NUI_IMAGE_RESOLUTION_640x480 
        , 0    // Image stream flags, eg. near mode...
        , 2    // Number of frames to buffer
        , NULL // Event handle
        , &depthStream);

    return (sensor != nullptr);
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
