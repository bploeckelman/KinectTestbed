#pragma once

#include <glm/glm.hpp>

namespace constants
{

	const float one_third    = 1.f / 3.f;
	const float e            = 2.7182818284590452354f;
	const float pi           = 3.1415926535897932384f;
	const float two_pi       = 2.f   * pi;
	const float four_pi      = 4.f   * pi;
	const float half_pi      = 0.5f  * pi;
	const float quarter_pi   = 0.25f * pi;
	const float one_third_pi = one_third * pi;
	const float pi_over_180  = pi / 180.f;

	const float initial_camera_y = 0.5f;
	const float initial_camera_z = 2.f;

	const float min_zoom      = 2.f;
	const float max_zoom      = 7.f;
	const float zoom_scale    = 5.f;

	const float camera_z_near = 0.1f;
	const float camera_z_far  = 100.f;
	const float camera_fov    = 66.f;

	const glm::vec3 worldX(1,0,0);
	const glm::vec3 worldY(0,1,0);
	const glm::vec3 worldZ(0,0,1);
	const glm::vec3 origin(0,0,0);

	const NUI_TRANSFORM_SMOOTH_PARAMETERS joint_smooth_params_low[]  = { 0.5f, 0.5f, 0.5f, 0.05f, 0.04f };
	const NUI_TRANSFORM_SMOOTH_PARAMETERS joint_smooth_params_med[]  = { 0.5f, 0.1f, 0.5f, 0.1f , 0.1f  };
	const NUI_TRANSFORM_SMOOTH_PARAMETERS joint_smooth_params_high[] = { 0.7f, 0.3f, 1.0f, 1.0f , 1.0f  };

};
