#pragma once
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN
#include <NuiApi.h>

#include <glm/glm.hpp>

#include <numeric>

namespace constants
{

	const int window_width = 1280;
	const int window_height = 720;
	const int window_bpp = 32;

	// linker warning... already defined (main.obj, application.obj)
	//const char *window_title = "kinect testbed";

	const int stream_width = 640;
	const int stream_height = 480;

	const int depth_bits = 16;
	const int stencil_bits = 0;
	const int aa_level = 2;

	// note: this is sf::style::fullscreen...
	const int fullscreen = 8; // sf::style::fullscreen
	const int windowed   = 7; // sf::style::default
	const int borderless = 0; // sf::style::none

	const int screen_mode = windowed;

	const float min_zoom      = 2.f;
	const float max_zoom      = 7.f;
	const float zoom_scale    = 5.f;

	const float camera_z_near = 0.1f;
	const float camera_z_far  = 100.f;
	const float camera_fov    = 66.f;

	const glm::vec3 initial_camera_pos(0.f, 0.5f, 2.f);
	const glm::vec3 worldX(1,0,0);
	const glm::vec3 worldY(0,1,0);
	const glm::vec3 worldZ(0,0,1);
	const glm::vec3 origin(0,0,0);

	const float red[]   = { 1.f, 0.f, 0.f, 1.f };
	const float red2[]   = { 0.5f, 0.f, 0.f, 1.f };
	const float green[] = { 0.f, 1.f, 0.f, 1.f };
	const float green2[] = { 0.f, 0.5f, 0.f, 1.f };
	const float blue[]  = { 0.f, 0.f, 1.f, 1.f };
	const float blue2[]  = { 0.f, 0.f, 0.5f, 1.f };
	const float gray[]  = { 0.5f, 0.5f, 0.5f, 1.f };
	const float white[] = { 1.f, 1.f, 1.f, 1.f };
	const float black[] = { 0.f, 0.f, 0.f, 1.f };

	const float one_third    = 1.f / 3.f;
	const float e            = 2.7182818284590452354f;
	const float pi           = 3.1415926535897932384f;
	const float two_pi       = 2.f   * pi;
	const float four_pi      = 4.f   * pi;
	const float half_pi      = 0.5f  * pi;
	const float quarter_pi   = 0.25f * pi;
	const float one_third_pi = one_third * pi;
	const float pi_over_180  = pi / 180.f;

	const float min_float = std::numeric_limits<float>::min();
	const float max_float = std::numeric_limits<float>::max();

	const NUI_TRANSFORM_SMOOTH_PARAMETERS joint_smooth_params_low[]  = { 0.5f, 0.5f, 0.5f, 0.05f, 0.04f };
	const NUI_TRANSFORM_SMOOTH_PARAMETERS joint_smooth_params_med[]  = { 0.5f, 0.1f, 0.5f, 0.1f , 0.1f  };
	const NUI_TRANSFORM_SMOOTH_PARAMETERS joint_smooth_params_high[] = { 0.7f, 0.3f, 1.0f, 1.0f , 1.0f  };

};
