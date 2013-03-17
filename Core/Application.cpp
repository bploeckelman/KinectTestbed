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
#include "Kinect/Kinect.h"
#include "Util/RenderUtils.h"
#include "Util/ImageManager.h"

const sf::VideoMode Application::videoMode = sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP);


// ----------------------------------------------------------------------------
Application::Application()
	: window(Application::videoMode, "Kinect Testbed")
	, clock()
	, gui()
	, colorTextureId(0)
	, depthTextureId(0)
	, colorData(new GLubyte[Kinect::COLOR_STREAM_BYTES])
	, depthData(new GLubyte[Kinect::DEPTH_STREAM_BYTES])
	, showColor(false)
	, showDepth(false)
	, showSkeleton(true)
	, rightMouseDown(false)
	, leftMouseDown(false)
	, cameray(constants::initial_camera_y)
	, cameraz(constants::initial_camera_z)
{
	memset(colorData, 0, Kinect::COLOR_STREAM_BYTES);
	memset(depthData, 0, Kinect::DEPTH_STREAM_BYTES);
}

Application::~Application()
{
	delete[] colorData;
	delete[] depthData;
}

void Application::startup()
{
	ImageManager::get().addResourceDir("../../Res/");

	kinect.initialize();
	gui.setInfo(kinect.getDeviceId());

	initOpenGL();
	mainLoop();
	shutdownOpenGL();
}

void Application::shutdown()
{
	window.close();
}

void Application::loadFile()
{
	std::wstring wfilename(showFileChooser());
	std::string filename; filename.assign(wfilename.begin(), wfilename.end());
	if (kinect.getSkeleton().loadFile(filename)) {
		gui.setFileName(filename);
	} else {
		gui.setFileName("No file loaded");
	}
}

void Application::moveToNextFrame()
{
	kinect.getSkeleton().nextFrame();
	gui.setProgress(kinect.getSkeleton().getFrameIndex() / (float) (kinect.getSkeleton().getNumFrames() - 1));
	gui.setIndex(kinect.getSkeleton().getFrameIndex());
}

void Application::moveToPreviousFrame()
{
	kinect.getSkeleton().prevFrame();
	gui.setProgress(kinect.getSkeleton().getFrameIndex() / (float) (kinect.getSkeleton().getNumFrames() - 1));
	gui.setIndex(kinect.getSkeleton().getFrameIndex());
}

void Application::setJointFrameIndex( const float fraction )
{
	kinect.getSkeleton().setFrameIndex(fraction);
	gui.setProgress(fraction);
	gui.setIndex(kinect.getSkeleton().getFrameIndex());
}

void Application::mainLoop()
{
	clock.restart();    
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
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left )) leftMouseDown  = true;
		}
		if (event.type == sf::Event::MouseButtonReleased) {
			if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) rightMouseDown = false;
			if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left )) leftMouseDown  = false;
		}

		if (event.type == sf::Event::MouseWheelMoved) {
			const int threshold = 2;
			static int accum = 0;
			accum += event.mouseWheel.delta;
			if (accum < -threshold || accum > threshold) {
				cameraz -= 0.1f * accum / threshold;
				accum = 0;
			}
		}
	}
}

void Application::draw()
{
	window.setActive();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
		glTranslatef(0, -cameray, -cameraz);
		glRotatef(getCameraRotationX(), 1.f, 0.f, 0.f);
		glRotatef(getCameraRotationY(), 0.f, 1.f, 0.f);

		Render::ground();
		Render::basis();

		kinect.update();
		kinect.getSkeleton().render();
		drawKinectImageStreams();
	glPopMatrix();

	gui.draw(window);

	window.display();
}

// TODO : this is ugly as hell, make it cleaner 
float Application::getCameraRotationX()
{
	static const int middleY = WINDOW_HEIGHT / 2;
	static const int scrollThreshold = 20;
	static const float timeThreshold = 0.05f;
	static const float rotBound      = 90;
	static const float scale         = 5.f;

	static float rot = 0.f;
	static float lastTime = clock.getElapsedTime().asSeconds();

	float thisTime = clock.getElapsedTime().asSeconds();
	float delta    = thisTime - lastTime;
	if (delta > timeThreshold) {
		if (rightMouseDown) {
			const int currY = sf::Mouse::getPosition(window).y;
			const int delta = currY - middleY;
			if (delta < -scrollThreshold || delta > scrollThreshold) {
				rot += static_cast<int>(scale * ((float)delta / (float)middleY));
				if (rot > rotBound) rot = rotBound;
				if (rot < 0.f) rot = 0.f;
			}
		}
		lastTime = thisTime;
	}

	return rot;
}

// TODO : this is ugly as hell, make it cleaner 
float Application::getCameraRotationY()
{
	static const int middleX = WINDOW_WIDTH / 2;
	static const int scrollThreshold = 20;
	static const float timeThreshold = 0.05f;
	static const float rotBound      = 360.f;
	static const float scale         = 5.f;

	static float rot = 0.f;
	static float lastTime = clock.getElapsedTime().asSeconds();

	float thisTime = clock.getElapsedTime().asSeconds();
	float delta    = thisTime - lastTime;
	if (delta > timeThreshold) {
		if (rightMouseDown) {
			const int currX = sf::Mouse::getPosition(window).x;
			const int delta = currX - middleX;
			if (delta < -scrollThreshold || delta > scrollThreshold) {
				rot += static_cast<int>(scale * ((float)delta / (float)middleX));
				if (rot > rotBound) rot = 0;
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
		Kinect::COLOR_STREAM_WIDTH, Kinect::COLOR_STREAM_HEIGHT,
		0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid *) colorData);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &depthTextureId);
	glBindTexture(GL_TEXTURE_2D, depthTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
		Kinect::DEPTH_STREAM_WIDTH, Kinect::DEPTH_STREAM_HEIGHT,
		0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid *) depthData);
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

void Application::updateKinectImageStreams()
{
	glBindTexture(GL_TEXTURE_2D, colorTextureId);
	kinect.getStreamData(colorData, COLOR, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
		Kinect::COLOR_STREAM_WIDTH, Kinect::COLOR_STREAM_HEIGHT,
		GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid *) colorData);

	glBindTexture(GL_TEXTURE_2D, depthTextureId);
	kinect.getStreamData(depthData, DEPTH, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
		Kinect::DEPTH_STREAM_WIDTH, Kinect::DEPTH_STREAM_HEIGHT,
		GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid *) depthData);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Application::drawKinectImageStreams()
{
	// Get kinect frame and update textures
	if ((showColor || showDepth) && kinect.isInitialized()) {
		updateKinectImageStreams();

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
