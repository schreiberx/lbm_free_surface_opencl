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
 * CConfig.hpp
 *
 *  Created on: Mar 22, 2010
 *      Author: martin
 */

#ifndef CCONFIG_HPP_
#define CCONFIG_HPP_

#include "libgl/hud/CGlWindow.hpp"
#include "libgl/hud/CGlHudConfig.hpp"
#include "libgl/hud/CGlFreeType.hpp"
#include "libgl/hud/CGlRenderOStream.hpp"

/**
 * main gui configuration section
 *
 * this class is intended to hold all configurable parameters for the visualization.
 */
class CConfig
{
	CGlWindow windowMain;
	CGlHudConfig cGlHudConfigMainLeft;
	CGlHudConfig cGlHudConfigMainRight;

	CGlWindow windowLights;
	CGlHudConfig cGlHudConfigLights;

public:

	bool main_drag_n_drop_active;
	bool lights_drag_n_drop_active;

	/////////////////////////////////////////////////////////////////////

	bool reset;
	bool quit;

	bool render_scene;
	bool render_mesh_bunny;
	bool render_mesh_bunny_reflected;
	bool render_mesh_sphere;
	bool render_mesh_sphere_reflected;

	bool render_animated_balls;
	bool render_animated_meshes;
	bool render_animated_quads;

	bool render_table;

	bool create_cube_map;
	bool display_flat_cube_map;

	bool render_skybox;

	bool refraction_active;

	bool volume_simple;
	bool volume_cube_steps;
	bool volume_interpolated;
	bool volume_marching_cubes;
	bool volume_dummy;

	bool render_bounding_box;

	bool render_marching_cubes_geometry_shader;
	bool render_marching_cubes_vertex_array;
	bool render_marching_cubes_vertex_array_rgba;
	bool render_marching_cubes_vertex_array_front_back_refractions;
	float render_marching_cubes_vertex_array_front_back_step_size;
	bool render_marching_cubes_vertex_array_front_back_depth_voxels_refractions;
	bool render_marching_cubes_vertex_array_front_back_total_reflections;
	bool render_marching_cubes_vertex_array_multi_layered_refractions;
	bool render_marching_cubes_vertex_array_disable_mc_generation;
	bool render_marching_cubes_dummy;
	bool render_view_space_refractions_peeling_textures;

	bool rotate_central_object;

	bool take_screenshot;
	bool take_screenshot_series;

	bool output_fps_and_parameters;
	bool output_world_position_at_mouse_cursor;

	bool render_hud;

	bool random_model_rotation;
	bool random_view_rotation;

	// SIMULATION
	bool lbm_simulation_run;
	bool lbm_simulation_reset_fluid;
	bool lbm_simulation_reset_simulation;
	bool lbm_simulation_copy_fluid_fraction_to_visualization;
	bool lbm_simulation_multiple_timesteps_per_frame;
	bool lbm_simulation_count_simulation_mass;
	bool lbm_simulation_normalize_simulation_mass;
	bool lbm_simulation_disable_visualization;
	float lbm_simulation_gravitation;
	float lbm_simulation_viscosity;
	float lbm_simulation_domain_x_length;
	float lbm_simulation_mass_exchange_factor;

	bool lbm_simulation_init_breaking_dam;
	bool lbm_simulation_init_pool;
	bool lbm_simulation_init_water_sphere;
	bool lbm_simulation_init_obstacle_half_sphere;
	bool lbm_simulation_init_obstacle_vertical_bar;
	bool lbm_simulation_init_fluid_with_gas_sphere;
	bool lbm_simulation_init_filled_domain;

	bool render_light_space_box;
	bool view_pos_at_light_space_box;

	bool increment_ticks;

	// PHOTON MAP
	bool photon_mapping_front_back;
	bool photon_mapping_front_back_caustic_map;
	float photon_mapping_front_back_caustic_map_step_size;
	bool photon_mapping_front_back_faces_from_polygons;
	bool photon_mapping_shadows;
	float photon_mapping_light0_energy;
	float photon_mapping_light0_splat_size;
	int photon_mapping_peeling_texture_width;
	int photon_mapping_peeling_texture_height;
	int photon_mapping_photon_texture_width;
	int photon_mapping_photon_texture_height;
	bool render_photon_mapping_peeling_textures;

	float refraction_index;
	float volume_step_size;
	float volume_gradient_distance;

	float distance_to_center;
	float perspective_zoom;

	GLSL::vec3 view_translation;

	/************************************
	 * LIGHT
	 ************************************/
	bool light0_rotation;
	GLSL::vec3 light0_world_position;

	bool light0_enabled;

	GLSL::vec3 light0_ambient_color3;
	GLSL::vec3 light0_diffuse_color3;
	GLSL::vec3 light0_specular_color3;

	// water material
	GLSL::vec3 water_ambient_color3;
	GLSL::vec3 water_diffuse_color3;
	GLSL::vec3 water_specular_color3;
	float water_specular_exponent;
	float water_reflectance_at_normal_incidence;


	/////////////////////////////////////////////////////////////////////
private:
	int mouse_x;
	int mouse_y;

	CGlFreeType *free_type;

public:
	CConfig()
	{
		main_drag_n_drop_active = false;
		lights_drag_n_drop_active = false;
	}


	void setup(	CGlFreeType &p_free_type,
				CGlRenderOStream &rostream
			)
	{
		free_type = &p_free_type;

		mouse_x = -1;
		mouse_y = -1;

		CGlHudConfig::COption o;

		// main window

		windowMain.setPosition(GLSL::ivec2(3, 3));
		windowMain.setSize(GLSL::ivec2(530, 594));
		windowMain.setBackgroundColor(GLSL::vec4(74.0/256.0, 96.0/256.0, 150.0/256.0, 0.7));

		cGlHudConfigMainLeft.insert(o.setupText("|--- MAIN CONTROL ---|"));
		cGlHudConfigMainLeft.insert(o.setupBoolean("Reset visualization", &reset));							reset = false;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Quit program", &quit));									quit = false;

		cGlHudConfigMainLeft.insert(o.setupLinebreak());
		cGlHudConfigMainLeft.insert(o.setupText("|--- RENDERERS ---|"));
		cGlHudConfigMainLeft.insert(o.setupBoolean("Render Scene", &render_scene));							render_scene = true;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Mesh Bunny", &render_mesh_bunny));							render_mesh_bunny = false;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Mesh Bunny with reflections", &render_mesh_bunny_reflected));	render_mesh_bunny_reflected = false;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Mesh Sphere", &render_mesh_sphere));					render_mesh_sphere = false;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Mesh Sphere with reflections", &render_mesh_sphere_reflected));	render_mesh_sphere_reflected = false;

		cGlHudConfigMainLeft.insert(o.setupLinebreak());
		cGlHudConfigMainLeft.insert(o.setupBoolean("Animated balls", &render_animated_balls));				render_animated_balls = true;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Animated meshes", &render_animated_meshes));			render_animated_meshes = false;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Animated quads", &render_animated_quads));				render_animated_quads = true;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Table", &render_table));								render_table = true;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Sky Box", &render_skybox));								render_skybox = true;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Render model bounding box", &render_bounding_box));		render_bounding_box = true;

		cGlHudConfigMainLeft.insert(o.setupLinebreak());
		cGlHudConfigMainLeft.insert(o.setupText("|--- CUBE MAP ---|"));
		cGlHudConfigMainLeft.insert(o.setupBoolean("Create Cube Map each frame", &create_cube_map));			create_cube_map = true;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Display Flat Cube Map", &display_flat_cube_map));			display_flat_cube_map = false;

		cGlHudConfigMainLeft.insert(o.setupLinebreak());
		cGlHudConfigMainLeft.insert(o.setupText("|--- BOGUS STUFF ---|"));
		cGlHudConfigMainLeft.insert(o.setupBoolean("Output FPS and parameters", &output_fps_and_parameters));				output_fps_and_parameters = true;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Output world position at mouse cursor", &output_world_position_at_mouse_cursor));				output_world_position_at_mouse_cursor = false;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Screenshot (screenshot.bmp)", &take_screenshot));	take_screenshot = false;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Screenshot series", &take_screenshot_series));				take_screenshot_series = false;
		cGlHudConfigMainLeft.insert(o.setupText("(screenshots/screenshot_#####.bmp)"));
		cGlHudConfigMainLeft.insert(o.setupBoolean("Increment ticks (disable to pause)", &increment_ticks));		increment_ticks = true;

		cGlHudConfigMainLeft.insert(o.setupLinebreak());
		cGlHudConfigMainLeft.insert(o.setupText("|--- POSITION/VIEW ---|"));
		cGlHudConfigMainLeft.insert(o.setupFloat("Zoom for perspective matrix", &perspective_zoom, 0.3));
		cGlHudConfigMainLeft.insert(o.setupFloat("Distance to center", &distance_to_center, 0.3));
		cGlHudConfigMainLeft.insert(o.setupBoolean("Rotate model using random numbers", &random_model_rotation));		random_model_rotation = false;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Rotate view using random numbers", &random_view_rotation));			random_view_rotation = false;
		cGlHudConfigMainLeft.insert(o.setupBoolean("Rotate central object", &rotate_central_object));			rotate_central_object = false;

		cGlHudConfigMainLeft.insert(o.setupFloat("Translation X", &view_translation[0], 0.1));	// view translation is already set to (0,0,0)
		cGlHudConfigMainLeft.insert(o.setupFloat("Translation Y", &view_translation[1], 0.1));
		cGlHudConfigMainLeft.insert(o.setupFloat("Translation Z", &view_translation[2], 0.1));

		cGlHudConfigMainLeft.insert(o.setupLinebreak());
		cGlHudConfigMainLeft.insert(o.setupText("Press [SPACE] to show/hide HUDs!"));


		cGlHudConfigMainRight.insert(o.setupText("|--- VOLUME CASTING ---|"));
		cGlHudConfigMainRight.insert(o.setupBoolean("Activate / deactivate refractions", &refraction_active));			refraction_active = true;
		cGlHudConfigMainRight.insert(o.setupBoolean("Simple", &volume_simple));						volume_simple = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Interpolated", &volume_interpolated));		volume_interpolated = true;
		cGlHudConfigMainRight.insert(o.setupBoolean("Cube steps", &volume_cube_steps));	volume_cube_steps = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Marching cubes", &volume_marching_cubes));		volume_marching_cubes = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Dummy renderer", &volume_dummy));					volume_dummy = false;

		cGlHudConfigMainRight.insert(o.setupLinebreak());
		cGlHudConfigMainRight.insert(o.setupText("|--- MARCHING CUBES ---|"));
		cGlHudConfigMainRight.insert(o.setupBoolean("version using geometry shaders", &render_marching_cubes_geometry_shader));
		render_marching_cubes_geometry_shader = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("HP shaded (R)", &render_marching_cubes_vertex_array));
		render_marching_cubes_vertex_array = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("HP shaded (RGBA, GL_INTEROP)", &render_marching_cubes_vertex_array_rgba));
		render_marching_cubes_vertex_array_rgba = false;

		cGlHudConfigMainRight.insert(o.setupBoolean("refractions (front/back)", &render_marching_cubes_vertex_array_front_back_refractions));
		render_marching_cubes_vertex_array_front_back_refractions = false;
		cGlHudConfigMainRight.insert(o.setupFloat("F/B - step size", &render_marching_cubes_vertex_array_front_back_step_size, 0.1, 0.1, 1024));		render_marching_cubes_vertex_array_front_back_step_size = 1.0;

		cGlHudConfigMainRight.insert(o.setupBoolean("refractions (front/back, depth voxels)", &render_marching_cubes_vertex_array_front_back_depth_voxels_refractions));
		render_marching_cubes_vertex_array_front_back_depth_voxels_refractions = false;

		cGlHudConfigMainRight.insert(o.setupBoolean("refractions (front/back, total refl.)", &render_marching_cubes_vertex_array_front_back_total_reflections));
		render_marching_cubes_vertex_array_front_back_total_reflections = false;

		cGlHudConfigMainRight.insert(o.setupBoolean("refractions (multi layered) [TODO]", &render_marching_cubes_vertex_array_multi_layered_refractions));
		render_marching_cubes_vertex_array_multi_layered_refractions = false;

		cGlHudConfigMainRight.insert(o.setupBoolean("refractions: disable MC generation", &render_marching_cubes_vertex_array_disable_mc_generation));
		render_marching_cubes_vertex_array_disable_mc_generation = false;

		cGlHudConfigMainRight.insert(o.setupBoolean("dummy renderer for GL_INTEROPS", &render_marching_cubes_dummy));
		render_marching_cubes_dummy = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Render peeling textures", &render_view_space_refractions_peeling_textures));				render_view_space_refractions_peeling_textures = false;

		cGlHudConfigMainRight.insert(o.setupLinebreak());
		cGlHudConfigMainRight.insert(o.setupText("|--- GENERIC CONTROL ---|"));
		cGlHudConfigMainRight.insert(o.setupFloat("Refraction index", &refraction_index, 0.03));								refraction_index = 1.33;
		cGlHudConfigMainRight.insert(o.setupFloat("Step size for volume sampling", &volume_step_size, 0.05, 0.05));				volume_step_size = 0.5;
		cGlHudConfigMainRight.insert(o.setupFloat("Distance to compute volume gradient", &volume_gradient_distance, 0.05));		volume_gradient_distance = 1.0;


		/************************
		 * SIMULATION
		 ************************/

		cGlHudConfigMainRight.insert(o.setupLinebreak());
		cGlHudConfigMainRight.insert(o.setupText("|--- SIMULATION CONTROL ---|"));
		cGlHudConfigMainRight.insert(o.setupBoolean("Run lattice boltzmann simulation", &lbm_simulation_run));						lbm_simulation_run = true;
		cGlHudConfigMainRight.insert(o.setupBoolean("Reinitialize simulation fluid", &lbm_simulation_reset_fluid));					lbm_simulation_reset_fluid = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Reset lattice boltzmann simulation", &lbm_simulation_reset_simulation));		lbm_simulation_reset_simulation = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Copy fluid fraction from sim for vis.", &lbm_simulation_copy_fluid_fraction_to_visualization));				lbm_simulation_copy_fluid_fraction_to_visualization = true;
		cGlHudConfigMainRight.insert(o.setupBoolean("Multiple sim steps per frame", &lbm_simulation_multiple_timesteps_per_frame));	lbm_simulation_multiple_timesteps_per_frame = true;
		cGlHudConfigMainRight.insert(o.setupFloat("Gravitation length: ", &lbm_simulation_gravitation, 0.1, -100, 0));							lbm_simulation_gravitation = -9.81;
		cGlHudConfigMainRight.insert(o.setupFloatHI("Viscosity: ", &lbm_simulation_viscosity, 0.01, CMath::numeric_min<float>(), CMath::numeric_max<float>()));								lbm_simulation_viscosity = 0.00001;
		cGlHudConfigMainRight.insert(o.setupFloatHI("Domain X length: ", &lbm_simulation_domain_x_length, 0.01, CMath::numeric_min<float>(), CMath::numeric_max<float>()));					lbm_simulation_domain_x_length = 0.03;
		cGlHudConfigMainRight.insert(o.setupFloat("Mass exchange factor: ", &lbm_simulation_mass_exchange_factor, 0.1, 0.1, 10000));		lbm_simulation_mass_exchange_factor = 20.0;

		cGlHudConfigMainRight.insert(o.setupBoolean("Count simulation mass", &lbm_simulation_count_simulation_mass));				lbm_simulation_count_simulation_mass = true;
		cGlHudConfigMainRight.insert(o.setupBoolean("Normalize simulation mass", &lbm_simulation_normalize_simulation_mass));		lbm_simulation_normalize_simulation_mass = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Switch Sim&Vis / test dataset", &lbm_simulation_disable_visualization));		lbm_simulation_disable_visualization = false;

		cGlHudConfigMainRight.insert(o.setupText("|--- Initialization ---|"));
		cGlHudConfigMainRight.insert(o.setupLinebreak());
		cGlHudConfigMainRight.insert(o.setupBoolean("Breaking dam", &lbm_simulation_init_breaking_dam));		lbm_simulation_init_breaking_dam = true;
		cGlHudConfigMainRight.insert(o.setupBoolean("Pool", &lbm_simulation_init_pool));						lbm_simulation_init_pool = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Water sphere", &lbm_simulation_init_water_sphere));		lbm_simulation_init_water_sphere = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Obstacle half sphere", &lbm_simulation_init_obstacle_half_sphere));		lbm_simulation_init_obstacle_half_sphere = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Obstacle vertical bar", &lbm_simulation_init_obstacle_vertical_bar));		lbm_simulation_init_obstacle_vertical_bar = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Fluid with gas sphere", &lbm_simulation_init_fluid_with_gas_sphere));		lbm_simulation_init_fluid_with_gas_sphere = false;
		cGlHudConfigMainRight.insert(o.setupBoolean("Filled domain", &lbm_simulation_init_filled_domain));		lbm_simulation_init_filled_domain = false;


		int sx = 10;
		int sy = windowMain.size[1]-10;
		cGlHudConfigMainLeft.setup(*free_type, rostream, sx, sy);
		cGlHudConfigMainRight.setup(*free_type, rostream, sx + cGlHudConfigMainLeft.area_width-20, sy);


		/***********************
		 * LIGHT
		 ***********************/

		cGlHudConfigLights.insert(o.setupText("|--- LIGHT 0 ---|"));

		cGlHudConfigLights.insert(o.setupBoolean("enabled", &light0_enabled));					light0_enabled = true;
		cGlHudConfigLights.insert(o.setupFloat("position X", &light0_world_position[0], 0.1));	light0_world_position = GLSL::vec3(0.5, 4.5, 0.5);
		cGlHudConfigLights.insert(o.setupFloat("position Y", &light0_world_position[1], 0.1));
		cGlHudConfigLights.insert(o.setupFloat("position Z", &light0_world_position[2], 0.1));

		cGlHudConfigLights.insert(o.setupFloat("AMBIENT R", &light0_ambient_color3[0], 0.01, 0.0, 1.0));	light0_ambient_color3 = GLSL::vec3(1,1,1);
		cGlHudConfigLights.insert(o.setupFloat("AMBIENT G", &light0_ambient_color3[1], 0.01, 0.0, 1.0));
		cGlHudConfigLights.insert(o.setupFloat("AMBIENT B", &light0_ambient_color3[2], 0.01, 0.0, 1.0));
		cGlHudConfigLights.insert(o.setupFloat("DIFFUSE R", &light0_diffuse_color3[0], 0.01, 0.0, 1.0));	light0_diffuse_color3 = GLSL::vec3(1,1,1);
		cGlHudConfigLights.insert(o.setupFloat("DIFFUSE G", &light0_diffuse_color3[1], 0.01, 0.0, 1.0));
		cGlHudConfigLights.insert(o.setupFloat("DIFFUSE B", &light0_diffuse_color3[2], 0.01, 0.0, 1.0));
		cGlHudConfigLights.insert(o.setupFloat("SPECULAR R", &light0_specular_color3[0], 0.01, 0.0, 1.0));	light0_specular_color3 = GLSL::vec3(1,1,1);
		cGlHudConfigLights.insert(o.setupFloat("SPECULAR G", &light0_specular_color3[1], 0.01, 0.0, 1.0));
		cGlHudConfigLights.insert(o.setupFloat("SPECULAR B", &light0_specular_color3[2], 0.01, 0.0, 1.0));

		cGlHudConfigLights.insert(o.setupBoolean("Light0 rotation", &light0_rotation));				light0_rotation = false;


		// water material
		cGlHudConfigLights.insert(o.setupLinebreak());
		cGlHudConfigLights.insert(o.setupText("|--- transparent WATER parameters ---|"));
		cGlHudConfigLights.insert(o.setupFloat("AMBIENT R", &water_ambient_color3[0], 0.01, 0.0, 111.0));		water_ambient_color3 = GLSL::vec3(0.78, 0.88, 1.0);
		cGlHudConfigLights.insert(o.setupFloat("AMBIENT G", &water_ambient_color3[1], 0.01, 0.0, 111.0));
		cGlHudConfigLights.insert(o.setupFloat("AMBIENT B", &water_ambient_color3[2], 0.01, 0.0, 111.0));
		cGlHudConfigLights.insert(o.setupFloat("DIFFUSE R", &water_diffuse_color3[0], 0.01, 0.0, 111.0));		water_diffuse_color3 = GLSL::vec3(0.0f);
		cGlHudConfigLights.insert(o.setupFloat("DIFFUSE G", &water_diffuse_color3[1], 0.01, 0.0, 111.0));
		cGlHudConfigLights.insert(o.setupFloat("DIFFUSE B", &water_diffuse_color3[2], 0.01, 0.0, 111.0));
		cGlHudConfigLights.insert(o.setupFloat("SPECULAR exponent", &water_specular_exponent, 0.1, 0.0));	water_specular_exponent = 20.0f;
		cGlHudConfigLights.insert(o.setupFloat("SPECULAR R", &water_specular_color3[0], 0.01, 0.0, 111.0));	water_specular_color3 = GLSL::vec3(6.5,6.3,6);
		cGlHudConfigLights.insert(o.setupFloat("SPECULAR G", &water_specular_color3[1], 0.01, 0.0, 111.0));
		cGlHudConfigLights.insert(o.setupFloat("SPECULAR B", &water_specular_color3[2], 0.01, 0.0, 111.0));
		cGlHudConfigLights.insert(o.setupFloat("Water normal reflection", &water_reflectance_at_normal_incidence, 0.001, 0.0, 1.0));	water_reflectance_at_normal_incidence = 0.07;

		cGlHudConfigLights.insert(o.setupLinebreak());
		cGlHudConfigLights.insert(o.setupText("|--- PHOTON MAPPING ---|"));
		cGlHudConfigLights.insert(o.setupBoolean("Photon Mapping (Front/Back)", &photon_mapping_front_back));						photon_mapping_front_back = false;
		cGlHudConfigLights.insert(o.setupBoolean("Photon Mapping (F/B Caustic Map)", &photon_mapping_front_back_caustic_map));		photon_mapping_front_back_caustic_map = true;
		cGlHudConfigLights.insert(o.setupFloat("F/B Caustic Map - step size", &photon_mapping_front_back_caustic_map_step_size, 0.1, 0.1, 1024));		photon_mapping_front_back_caustic_map_step_size = 1.0;
		cGlHudConfigLights.insert(o.setupBoolean("F/B from Polyg.(ON) / Vol.cast.(OFF)", &photon_mapping_front_back_faces_from_polygons));				photon_mapping_front_back_faces_from_polygons = false;
		cGlHudConfigLights.insert(o.setupBoolean("Shadows", &photon_mapping_shadows));												photon_mapping_shadows = true;
		cGlHudConfigLights.insert(o.setupFloat("Light0 energy", &photon_mapping_light0_energy, 1, 0.0));							photon_mapping_light0_energy = 15;
		cGlHudConfigLights.insert(o.setupFloat("Light0 splat size", &photon_mapping_light0_splat_size, 0.001, 0.0));				photon_mapping_light0_splat_size = 0.01;

		cGlHudConfigLights.insert(o.setupInt("Peeling texture width", &photon_mapping_peeling_texture_width, 128, 128, 4096));		photon_mapping_peeling_texture_width = 1024;
		cGlHudConfigLights.insert(o.setupInt("Peeling texture height", &photon_mapping_peeling_texture_height, 128, 128, 4096));	photon_mapping_peeling_texture_height = 1024;
		cGlHudConfigLights.insert(o.setupInt("Photon texture width", &photon_mapping_photon_texture_width, 128, 128, 4096));		photon_mapping_photon_texture_width = 1024;
		cGlHudConfigLights.insert(o.setupInt("Photon texture height", &photon_mapping_photon_texture_height, 128, 128, 4096));		photon_mapping_photon_texture_height = 1024;
		cGlHudConfigLights.insert(o.setupBoolean("Render LightSpaceBox", &render_light_space_box));									render_light_space_box = false;
		cGlHudConfigLights.insert(o.setupBoolean("View through LightSpaceBox", &view_pos_at_light_space_box));						view_pos_at_light_space_box = false;
		cGlHudConfigLights.insert(o.setupBoolean("Render peel/phot/postproc. textures", &render_photon_mapping_peeling_textures));	render_photon_mapping_peeling_textures = false;

		windowLights.setPosition(GLSL::ivec2(530, 10));
		windowLights.setBackgroundColor(GLSL::vec4(128.0/256.0, 128.0/256.0, 128.0/256.0, 0.7));
		windowLights.setSize(GLSL::ivec2(260, 570));

		sx = 10;
		sy = windowLights.size[1]-10;
		cGlHudConfigLights.setup(*free_type, rostream, sx, sy);

		resetPerspectiveAndPosition();

	}


	/**
	 * setup a callback handler which is called, when the value changed
	 */
	void set_callback(	void *value_ptr,							///< pointer to value
						void (*callback_handler)(void *user_ptr),	///< callback handler
						void *user_ptr
					)
	{
		cGlHudConfigMainLeft.set_callback(value_ptr, callback_handler, user_ptr);
		cGlHudConfigMainRight.set_callback(value_ptr, callback_handler, user_ptr);

		cGlHudConfigLights.set_callback(value_ptr, callback_handler, user_ptr);
	}

	/**
	 * reset some values
	 */
	void resetPerspectiveAndPosition()
	{
		perspective_zoom = -12;
		distance_to_center = 4;
		view_translation[0] = 0.0;
		view_translation[1] = 0.0;
		view_translation[2] = 0.0;

	}

	/**
	 * render windowMain with configuration information
	 */
	void render()
	{
		free_type->viewportChanged(windowLights.size);
		windowLights.startRendering();
		cGlHudConfigLights.renderConfigContent();
		windowLights.finishRendering();

		free_type->viewportChanged(windowMain.size);
		windowMain.startRendering();
		cGlHudConfigMainLeft.renderConfigContent();
		cGlHudConfigMainRight.renderConfigContent();
		windowMain.finishRendering();
	}

	bool mouse_button_down(int button)
	{
		if (render_hud)
		{
			if (windowMain.inWindow(mouse_x, mouse_y))
			{
				cGlHudConfigMainLeft.mouse_button_down(button);
				cGlHudConfigMainRight.mouse_button_down(button);

				if (cGlHudConfigMainLeft.active_option == NULL && cGlHudConfigMainRight.active_option == NULL)
				{
					if (button == CRenderWindow::MOUSE_BUTTON_LEFT)
						main_drag_n_drop_active = true;
				}
				return true;
			}
			else if (windowLights.inWindow(mouse_x, mouse_y))
			{
				cGlHudConfigLights.mouse_button_down(button);

				if (cGlHudConfigLights.active_option == NULL && cGlHudConfigLights.active_option == NULL)
				{
					if (button == CRenderWindow::MOUSE_BUTTON_LEFT)
						lights_drag_n_drop_active = true;
				}
				return true;
			}
		}

		return false;
	}

	void mouse_button_up(int button)
	{
		if (render_hud)
		{
			cGlHudConfigMainLeft.mouse_button_up(button);
			cGlHudConfigMainRight.mouse_button_up(button);

			cGlHudConfigLights.mouse_button_up(button);
		}

		if (button == CRenderWindow::MOUSE_BUTTON_LEFT)
		{
			// always disable drag_n_drop, when
			main_drag_n_drop_active = false;
			lights_drag_n_drop_active = false;
		}
	}


	bool mouse_wheel(int x, int y)
	{
		if (render_hud)
		{

			if (windowMain.inWindow(mouse_x, mouse_y))
			{
				cGlHudConfigMainLeft.mouse_wheel(x, y);
				cGlHudConfigMainRight.mouse_wheel(x, y);
				return true;
			}

			if (windowLights.inWindow(mouse_x, mouse_y))
			{
				cGlHudConfigLights.mouse_wheel(x, y);
				return true;
			}
		}
		return false;
	}

	void mouse_motion(int x, int y)
	{
		int dx = x - mouse_x;
		int dy = y - mouse_y;

		if (main_drag_n_drop_active)
			windowMain.setPosition(windowMain.pos + GLSL::ivec2(dx, dy));

		if (lights_drag_n_drop_active)
			windowLights.setPosition(windowLights.pos + GLSL::ivec2(dx, dy));

		mouse_x = x;
		mouse_y = y;

		if (render_hud)
		{
			cGlHudConfigMainLeft.mouse_motion(mouse_x-windowMain.pos[0], mouse_y-windowMain.pos[1]);
			cGlHudConfigMainRight.mouse_motion(mouse_x-windowMain.pos[0], mouse_y-windowMain.pos[1]);

			cGlHudConfigLights.mouse_motion(mouse_x-windowLights.pos[0], mouse_y-windowLights.pos[1]);
		}
	}
};


#endif /* CCONFIG_HPP_ */
