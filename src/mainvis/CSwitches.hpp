/*
 * Copyright 2010 Martin Schreiber
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * CSwitches.hpp
 *
 *  Created on: Mar 22, 2010
 *      Author: martin
 */

#ifndef CSWITCHES_HPP_
#define CSWITCHES_HPP_



class CSwitches
{
private:

	struct SSwitches
	{
		bool *switch_ptr;			// pointer to boolean switch
		const int key;				// key which is pressed to activate / deactivate switch
		const char *description;
		bool init_value;			// initial value
	};

	// switch list terminated by 0 key value
	SSwitches *switches;

public:
	CSwitches(CConfig &cConfig)	:
		switches(0)
	{
		static SSwitches static_switches[] =
		{
				{&cConfig.reset,					'r', "Reset", false},
				{&cConfig.quit,						'q', "Quit", false},
				{NULL, ' ', "", false},

				// Renderable objects
				{&cConfig.render_mesh_bunny,				'a', "Render Bunny Mesh", false},
				{&cConfig.render_mesh_bunny_reflected,		'A', "Render Bunny Mesh with reflections", false},
				{&cConfig.render_animated_balls,			's', "Render Animated Balls", true},
				{&cConfig.render_animated_meshes,			'F', "Render Animated Meshes", false},
				{&cConfig.render_animated_quads,			'd', "Render Animated Quads", true},
				{&cConfig.render_mesh_sphere,				'f', "Render Mesh Sphere", false},
				{&cConfig.render_mesh_sphere_reflected,	'F', "Render Mesh Sphere with reflections", false},
				{&cConfig.render_table,				'g', "Render table", true},
				{NULL, ' ', "", false},

				// direct volume casting renderers
				{&cConfig.refraction_active,	'Z', "Activate / deactivate refractions", true},
				{&cConfig.volume_simple,		'z', "Volume tracing (simple)", false},
				{&cConfig.volume_interpolated,	'x', "Volume tracing (simple interpolated)", false},
				{&cConfig.volume_cube_steps,	'c', "Volume tracing (cube steps interpolated)", false},
				{&cConfig.volume_marching_cubes,'v', "Volume tracing (marching cubes)", false},
				{&cConfig.volume_dummy,			'V', "Volume tracing (dummy renderer to measure GPU<->CPU<->GPU copy)", false},
				{&cConfig.render_bounding_box,	'b', "Render model bounding box", true},
				{NULL, ' ', "", false},

				// photon mapping stuff
				{&cConfig.render_light_space_box,		'i', "Render LightSpaceBox", false},
				{&cConfig.view_pos_at_light_space_box,	'I', "View through LightSpaceBox", false},
				{NULL, ' ', "", false},

				// volume renderers using marching cubes
				{&cConfig.render_marching_cubes_geometry_shader,								CRenderWindow::KEY_F4, "MC: version using geometry shaders", false},
				{&cConfig.render_marching_cubes_vertex_array,									CRenderWindow::KEY_F5, "MC: vertex arrays (red channel version)", false},
				{&cConfig.render_marching_cubes_vertex_array_rgba,								CRenderWindow::KEY_F6, "MC: vertex array (RGBA version and GL_INTEROP)", false},
				{&cConfig.render_marching_cubes_vertex_array_front_back_refractions,			CRenderWindow::KEY_F7, "MC: vertex array refractions (front/back only)", false},
//				{&cConfig.render_marching_cubes_vertex_array_front_back_multiple_refractions,	CRenderWindow::KEY_F8, "MC: vertex array refractions (front/back only, multiple reflections)", true},
				{&cConfig.render_marching_cubes_vertex_array_multi_layered_refractions,			CRenderWindow::KEY_F9, "MC: vertex array refractions (multi layered)", true},
				{&cConfig.render_marching_cubes_vertex_array_disable_mc_generation,				CRenderWindow::KEY_F10, "MC: vertex array refractions: disable MC generation", false},
				{&cConfig.render_marching_cubes_dummy,											CRenderWindow::KEY_F11, "MC: dummy renderer to test GL_INTEROPS", false},
				{NULL, ' ', "", false},

				{&cConfig.output_fps_and_parameters, 				'o', "Output FPS and parameters", true},
				{&cConfig.take_screenshot, 			'm', "Take screenshot (store as screenshot.bmp)", false},
				{&cConfig.take_screenshot_series,	'M', "Take screenshot series (screenshots/screenshot_#####.bmp)", false},
				{NULL, ' ', "", false},

				// simulation control
				{&cConfig.lbm_simulation_run, 					'j', "Run lattice boltzmann simulation", true},
				{&cConfig.lbm_simulation_reset_fluid, 			'k', "Reinitialize simulation fluid", false},
				{&cConfig.lbm_simulation_reset_simulation, 		'K', "Reset lattice boltzmann simulation", false},
				{&cConfig.lbm_simulation_copy_fluid_fraction_to_visualization, 	'l', "Copy fluid fraction from simulation to visualization", true},
				{&cConfig.lbm_simulation_multiple_timesteps_per_frame, 	'.', "Multiple simulation timesteps per frame", true},
				{&cConfig.lbm_simulation_count_simulation_mass, 	':', "Count simulation mass", false},
				{NULL, ' ', "", false},

				{&cConfig.render_hud, 	' ', "Render Hud", true},
				{NULL, 0, NULL, },
		};

		switches = static_switches;

		for (int i = 0; switches[i].key != 0; i++)
			if (switches[i].switch_ptr != NULL)
				*(switches[i].switch_ptr) = switches[i].init_value;
	}

	void key_pressed(int key, int mod)
	{
		if (mod & CRenderWindow::KEY_MOD_SHIFT)
			key -= 32;

		for (int i = 0; switches[i].key != 0; i++)
		{
			if (switches[i].switch_ptr == NULL)
				continue;

			if (switches[i].key == key)
			{
				*(switches[i].switch_ptr) = !*(switches[i].switch_ptr);
				break;
			}
		}
	}

	std::string getKeyInformations()
	{
		std::ostringstream stream;

		for (int i = 0; switches[i].key != 0; i++)
		{
			if (switches[i].switch_ptr == NULL)
			{
				stream << std::endl;
				continue;
			}

			std::string key_string;
			if (switches[i].key > 255)
			{
				switch(switches[i].key)
				{
					case CRenderWindow::KEY_F1:	key_string = "F1";	break;
					case CRenderWindow::KEY_F2:	key_string = "F2";	break;
					case CRenderWindow::KEY_F3:	key_string = "F3";	break;
					case CRenderWindow::KEY_F4:	key_string = "F4";	break;
					case CRenderWindow::KEY_F5:	key_string = "F5";	break;
					case CRenderWindow::KEY_F6:	key_string = "F6";	break;
					case CRenderWindow::KEY_F7:	key_string = "F7";	break;
					case CRenderWindow::KEY_F8:	key_string = "F8";	break;
					case CRenderWindow::KEY_F9:	key_string = "F9";	break;
					case CRenderWindow::KEY_F10:	key_string = "F10";	break;
					case CRenderWindow::KEY_F11:	key_string = "F11";	break;
					case CRenderWindow::KEY_F12:	key_string = "F12";	break;
					default:		key_string = "unknown key"; break;
				}
			}
			else
			{
				key_string = (char)switches[i].key;
			}
			stream << key_string << ": " << switches[i].description << " (" << (*(switches[i].switch_ptr) ? "ON" : "OFF") << ")" << std::endl;
		}
		return stream.str();
	}
};


#endif /* CSWITCHES_HPP_ */
