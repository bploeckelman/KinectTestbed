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
#include <glm/gtc/matrix_transform.hpp>

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
#include "Constants.h"
#include "Kinect/Kinect.h"
#include "Util/RenderUtils.h"
#include "Util/ImageManager.h"

using namespace constants;

const sf::VideoMode Application::videoMode = sf::VideoMode(window_width, window_height, window_bpp);
const sf::ContextSettings contextSettings(depth_bits, stencil_bits, aa_level);

std::wstring showFileChooser();

// ----------------------------------------------------------------------------
Application::Application()
	: window(Application::videoMode, "Kinect Testbed", screen_mode, contextSettings)
	, clock()
	, gui()
	, camera()
	, colorTextureId(0)
	, depthTextureId(0)
	, colorData(new GLubyte[Kinect::COLOR_STREAM_BYTES])
	, depthData(new GLubyte[Kinect::DEPTH_STREAM_BYTES])
	, rightMouseDown(false)
	, leftMouseDown(false)
	, shiftDown(false)
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
	// TODO : tweak output to not complain about
	// not finding things in working directory
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
	const std::string& name = filename.substr(filename.find_last_of("\\") + 1);
	Performance p(name);
	if (p.load(filename)) {
		kinect.getSkeleton().addPerformance(p);
		gui.setFileName(name);
	} else {
		gui.setFileName("No file loaded");
	}
}

void Application::closeFile()
{
	Performance *performance = &kinect.getSkeleton().getPerformance();	
	if (performance->getName() == "Live") {
		// TODO : message box?
		std::cout << "Live performance cannot be closed" << std::endl;
		return;
	}

	// TODO : this is super lame, make it more reasonable
	auto& performances = kinect.getSkeleton().getPerformances();
	for(auto& p = performances.begin(); p != performances.end(); ++p) {
		const auto& name = p->getName();
		if (name == performance->getName()) {
			gui.removePerformance(name);
			performance->clear();
			performances.erase(p);
			break;
		}
	}
	performance = nullptr;
	gui.setFileName("No file loaded");
}

void Application::moveToNextFrame()
{
	kinect.getSkeleton().nextFrame();
	if (gui.isAutoPlayEnabled() && kinect.getSkeleton().getFrameIndex() >= kinect.getSkeleton().getNumFrames() - 1) {
		kinect.getSkeleton().setFrameIndex(0);
	}
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
	gui.setIndex(kinect.getSkeleton().getPerformance().getCurrentFrameIndex());
}

void Application::mainLoop()
{
	clock.restart();    
	while (window.isOpen()) {
		update();
		render();
	}
}

void Application::update()
{
	processEvents();
}

void Application::render()
{
	window.setActive();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Skeleton& skeleton = kinect.getSkeleton();

	glPushMatrix();
		camera.update();
		kinect.update();

		if (gui.isAutoPlayEnabled() && skeleton.isLoaded()) {
			// Calculate the delta time between this frame and the next
			// Use 60 fps as a default delta time
			float frameDelta = 0.01667f; // 60 fps
			const unsigned int nextIndex = skeleton.getFrameIndex() + 1;
			if (nextIndex < skeleton.getNumFrames()) {
				const JointFrame& thisFrame = skeleton.getPerformance().getCurrentFrame();
				const JointFrame& nextFrame = skeleton.getPerformance().getFrames()[nextIndex];
				const float nextFrameTime = nextFrame.timestamp;
				const float thisFrameTime = thisFrame.timestamp;
				frameDelta = nextFrameTime - thisFrameTime;
				//std::cout << "Animation frame delta = " << frameDelta << std::endl;
			}

			const float thisFrameTime = clock.getElapsedTime().asSeconds();
			const float delta = thisFrameTime - gui.getLastFrameTime();
			if (delta >= frameDelta) {
				gui.setLastFrameTime(thisFrameTime);
				std::stringstream ss;
				ss << "Frame delta: " << frameDelta << ",  actual delta: " << delta << " (seconds)";
				gui.setPlayLabel(ss.str());
				moveToNextFrame();
			}
		}

		// Draw reflected skeleton first
		if (gui.isShowingSkeleton()) {
			glPushMatrix();
			glScalef(1.f, -1.f, 1.f);
			skeleton.render();
			glPopMatrix();
		}

		// Create imperfect reflector effect by blending ground over reflected scene with alpha
		glClear (GL_DEPTH_BUFFER_BIT);
		glPushAttrib (0xffffffff);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		Render::ground(0.5f);
		glDisable(GL_BLEND);
		glPopAttrib();

		Render::basis();

		skeleton.render();

		// Draw hand/camera orientation basis
		//Render::basis(1.f, origin + worldY, binormal, normal, tangent);

		drawKinectImageStreams();
	glPopMatrix();

	gui.draw(window);

	window.display();
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
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) shiftDown = true;
		}

		if (event.type == sf::Event::KeyReleased) {
			if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) shiftDown = false;
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
			const int threshold = 1;
			static int accum = 0;
			accum += event.mouseWheel.delta;
			if (accum < -threshold || accum > threshold) {
				camera.position.z -= (shiftDown ? 1.f : 0.1f) * accum / threshold;
				accum = 0;
			}
		}
	}
}

// ----------------------------------------------------------------------------
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
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glPointSize(10.f);
	glEnable(GL_POINT_SMOOTH);

	GLfloat mat_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat mat_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat mat_shininess[] = { 50.0f };
	GLfloat light_position[] = { 0.0f, 0.5f, 0.0f, 1.0f };
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glShadeModel (GL_SMOOTH);

	camera.setViewport();
	camera.setProjection();
	camera.setModelview();
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
	if ((gui.isShowingColor() || gui.isShowingDepth()) && kinect.isInitialized()) {
		updateKinectImageStreams();

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);

		// Draw color and depth images
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(3.75f,2.25f,-5.f); // fix in upper right corner of window
		if (gui.isShowingColor()) {
			glBindTexture(GL_TEXTURE_2D, colorTextureId);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex3f(0.f, -1.f, 0.f);
			glTexCoord2f(1, 1); glVertex3f(2.f, -1.f, 0.f);
			glTexCoord2f(1, 0); glVertex3f(2.f,  1.f, 0.f);
			glTexCoord2f(0, 0); glVertex3f(0.f,  1.f, 0.f);
			glEnd();
		}
		if (gui.isShowingDepth()) {
			glBindTexture(GL_TEXTURE_2D, depthTextureId);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex3f(0.f, -3.f, 0.f);
			glTexCoord2f(1, 1); glVertex3f(2.f, -3.f, 0.f);
			glTexCoord2f(1, 0); glVertex3f(2.f, -1.f, 0.f);
			glTexCoord2f(0, 0); glVertex3f(0.f, -1.f, 0.f);
			glEnd();
		}
		glPopMatrix();

		glEnable(GL_LIGHTING);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

// ----------------------------------------------------------------------------
std::wstring showFileChooser() {
	// Initialize COM
	if(CoInitializeEx(0,COINIT_APARTMENTTHREADED) != S_OK) {
		MessageBox(NULL,_T("Error opening browse window"),_T("ERROR"),MB_OK);
		CoUninitialize();
		return std::wstring();
	}

	// Get a pointer to the shell memory allocator
	LPMALLOC lpMalloc;
	if(SHGetMalloc(&lpMalloc) != S_OK) {
		MessageBox(NULL,_T("Error opening browse window"),_T("ERROR"),MB_OK);
		CoUninitialize();
		return std::wstring();
	}

	char   pszBuffer[MAX_PATH];
	LPWSTR filepath = (LPWSTR)pszBuffer;

	BROWSEINFO browseInfo;
	browseInfo.hwndOwner       = NULL;
	browseInfo.pidlRoot        = NULL;
	browseInfo.pszDisplayName  = filepath;
	browseInfo.lpszTitle       = _T("Select a joint data file");
	browseInfo.ulFlags         = BIF_BROWSEINCLUDEFILES;
	browseInfo.lpfn            = NULL;
	browseInfo.lParam          = 0;

	LPITEMIDLIST itemIdList;
	if(itemIdList = SHBrowseForFolder(&browseInfo)) {
		// Copy the path directory to the buffer
		if(SHGetPathFromIDList(itemIdList, filepath)) {
			// buf now holds the directory path
			std::wstringstream ss;
			ss << _T("You selected: ") << filepath << std::endl;
		}
		lpMalloc->Free(itemIdList);
	}
	lpMalloc->Release();
	CoUninitialize();

	return filepath;
}
