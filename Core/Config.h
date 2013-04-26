#pragma once

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int WINDOW_BPP = 32;

// linker warning... already defined (Main.obj, Application.obj)
//const char *WINDOW_TITLE = "Kinect Testbed";

const int STREAM_WIDTH = 640;
const int STREAM_HEIGHT = 480;

const int DEPTH_BITS = 16;
const int STENCIL_BITS = 0;
const int AA_LEVEL = 2;

// NOTE: this is sf::Style::Fullscreen...
const int FULLSCREEN = 8; // sf::Style::Fullscreen
const int WINDOWED   = 7; // sf::Style::Default
const int BORDERLESS = 0; // sf::Style::None

const int SCREEN_MODE = WINDOWED;
