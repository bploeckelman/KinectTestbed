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
#include "Config.h"
#include "Constants.h"
#include "Kinect/Kinect.h"
#include "Util/RenderUtils.h"
#include "Util/ImageManager.h"

const sf::VideoMode Application::videoMode = sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP);

const sf::ContextSettings contextSettings(16, 0, 2); // depth bits, stencil bits, aa level

// ----------------------------------------------------------------------------
Application::Application()
	: window(Application::videoMode, "Kinect Testbed")//, sf::Style::Fullscreen, contextSettings)
	, clock()
	, gui()
	, colorTextureId(0)
	, depthTextureId(0)
	, colorData(new GLubyte[Kinect::COLOR_STREAM_BYTES])
	, depthData(new GLubyte[Kinect::DEPTH_STREAM_BYTES])
	, showColor(true)
	, showDepth(true)
	, showSkeleton(true)
	, handControl(false)
	, autoPlay(false)
	, lastFrameTime(0.f)
	, rightMouseDown(false)
	, leftMouseDown(false)
	, shiftDown(false)
	, camerax(0.f)
	, cameray(constants::initial_camera_y)
	, cameraz(constants::initial_camera_z)
	, lastBinormal(constants::worldX)
	, lastNormal(constants::worldY)
	, lastTangent(constants::worldZ)
	, projection(1.f)
	, modelview(1.f)
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
	const std::string& name = filename.substr(filename.find_last_of("\\") + 1);
	Performance p(name);
	if (p.loadFile(filename)) {
		kinect.getSkeleton().addPerformance(p);
		gui.setFileName(name);
	} else {
		gui.setFileName("No file loaded");
	}
}

void Application::closeFile()
{
	Performance *performance = &kinect.getSkeleton().getPerformance();	
	// TODO : this is super lame, make it more reasonable
	auto& performances = kinect.getSkeleton().getPerformances();
	for(auto& p = performances.begin(); p != performances.end(); ++p) {
		const auto& name = p->getName();
		if (name == performance->getName()) {
			gui.removePerformance(name);
			performance->clearLoadedFrames();
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
	if (autoPlay && kinect.getSkeleton().getFrameIndex() >= kinect.getSkeleton().getNumFrames() - 1) {
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
				cameraz -= (shiftDown ? 1.f : 0.1f) * accum / threshold;
				accum = 0;
			}
		}
	}
}

void Application::draw()
{
	window.setActive();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Skeleton& skeleton = kinect.getSkeleton();

	bool useLast = false;
	glm::vec3 binormal = constants::worldX;
	glm::vec3 normal   = constants::worldY;
	glm::vec3 tangent  = constants::worldZ;
	if (handControl && !skeleton.getCurrentJointFrame().empty()) {
		const Joint& rightHand = skeleton.getCurrentRightHand();
		const Joint& leftHand  = skeleton.getCurrentLeftHand();
		//const Joint& head      = skeleton.getCurrentJointFrame().at(Skeleton::HEAD);
		if (rightHand.trackingState == TRACKED && leftHand.trackingState == TRACKED) {
			// Build a coordinate frame, but only if hands are reasonably far apart
			if (glm::distance(rightHand.position, leftHand.position) > 0.35f) {
				// Build a coordinate frame using left hand as origin and norm(right - left) as Y axis vector 
				//glm::vec3 tempBinormal;
				//normal       = glm::normalize(rightHand.position - leftHand.position);
				//tempBinormal = glm::normalize(glm::cross(constants::worldZ, normal));
				//tangent      = glm::normalize(glm::cross(normal, tempBinormal));
				//binormal     = glm::normalize(glm::cross(tangent, normal));

				// Build a coordinate frame using left hand as origin and norm(right - left) as X axis vector
				//glm::vec3 tempTangent;
				//binormal    = glm::normalize(rightHand.position - leftHand.position);
				//tempTangent = glm::normalize(glm::cross(binormal, constants::worldY));
				//normal      = glm::normalize(glm::cross(tempTangent, binormal));
				//tangent     = glm::normalize(glm::cross(binormal, normal));

				glm::vec3 tempTangent;
				binormal    = glm::normalize(rightHand.position - leftHand.position);
				//const float d = glm::dot(binormal, constants::worldX);
				//const float angle = acos(d) * 180.f / constants::pi;
				tempTangent = glm::normalize(glm::cross(binormal, constants::worldY));
				normal      = glm::normalize(glm::cross(tempTangent, binormal));
				tangent     = glm::normalize(glm::cross(binormal, normal));
			} else {
				binormal = lastBinormal;
				normal   = lastNormal;
				tangent  = lastTangent;
			}

			// Only enable hand distance zoom control if both hands are above the head
			//if (leftHand.position.y >= head.position.y && rightHand.position.y >= head.position.y) {
				const float dist = glm::distance(rightHand.position, leftHand.position) * constants::zoom_scale;
				cameraz = (dist < constants::min_zoom) ? constants::min_zoom 
						: (dist > constants::max_zoom) ? constants::max_zoom : dist;
			//}
		} else {
			binormal = lastBinormal;
			normal   = lastNormal;
			tangent  = lastTangent;
		}
	}

	glPushMatrix();
		modelview = glm::translate(glm::mat4(1), glm::vec3(-camerax, -cameray, -cameraz));
		if (handControl) {
			modelview[0][0] = binormal.x; modelview[0][1] = binormal.y; modelview[0][2] = binormal.z;
			modelview[1][0] = normal.x;   modelview[1][1] = normal.y;   modelview[1][2] = normal.z;
			modelview[2][0] = tangent.x;  modelview[2][1] = tangent.y;  modelview[2][2] = tangent.z;
			modelview = glm::rotate(modelview, 180.f, constants::worldY); // Face skeleton
		} else {
			modelview = glm::rotate(modelview, getCameraRotationX(), constants::worldX);
			modelview = glm::rotate(modelview, getCameraRotationY(), constants::worldY);
			modelview = glm::rotate(modelview, 180.f, constants::worldY);
		}
		glLoadMatrixf(glm::value_ptr(modelview));

		kinect.update();

		if (autoPlay && skeleton.isLoaded()) {
			// Calculate the delta time between this frame and the next
			// Use 60 fps as a default delta time
			float frameDelta = 0.01667f; // 60 fps
			const unsigned int nextIndex = skeleton.getFrameIndex() + 1;
			if (nextIndex < skeleton.getNumFrames()) {
				const JointFrame& thisFrame = skeleton.getPerformance().getCurrentFrame();
				const JointFrame& nextFrame = skeleton.getPerformance().getFrames()[nextIndex];
				const float nextFrameTime = nextFrame.at(SHOULDER_CENTER).timestamp;
				const float thisFrameTime = thisFrame.at(SHOULDER_CENTER).timestamp;
				frameDelta = nextFrameTime - thisFrameTime;
				//std::cout << "Animation frame delta = " << frameDelta << std::endl;
			}

			const float thisFrameTime = clock.getElapsedTime().asSeconds();
			const float delta = thisFrameTime - lastFrameTime;
			if (delta >= frameDelta) {
				lastFrameTime = thisFrameTime;
				std::stringstream ss;
				ss << "Frame delta: " << frameDelta << ",  actual delta: " << delta << " (seconds)";
				gui.setPlayLabel(ss.str());
				moveToNextFrame();
			}
		}

		// Draw reflected skeleton first
		if (showSkeleton) {
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
		//Render::basis(1.f, constants::origin + constants::worldY, binormal, normal, tangent);

		drawKinectImageStreams();
	glPopMatrix();

	gui.draw(window);

	window.display();

	lastBinormal = binormal;
	lastNormal   = normal;
	lastTangent  = tangent;
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
	static const float scale         = 10.f;

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
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glPointSize(10.f);
	glEnable(GL_POINT_SMOOTH);

	// Set viewport and projection
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	const float aspect = (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT;
	projection = glm::perspective(constants::camera_fov, aspect, constants::camera_z_near, constants::camera_z_far);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(projection));

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(modelview));

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

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);

		// Draw color and depth images
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(3.75f,2.25f,-5.f); // fix in upper right corner of window
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
