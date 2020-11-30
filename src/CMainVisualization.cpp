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

#include "libmath/CMath.hpp"
#include "CMainVisualization.hpp"
#include "mainvis/CRenderWindow.hpp"

#include <iostream>
#include <string>
#include <stdlib.h>
#include <time.h>

#include "mainvis/CConfig.hpp"

#include "lbm/CLbmOpenClInterface.hpp"


// include CCL.hpp after gl3.h to support sharing of contexts !!!
#include "libopencl/CCLSkeleton.hpp"

#include "libmath/CMath.hpp"
#include "lib/CStopwatch.hpp"

#include "libgl/hud/CGlFreeType.hpp"
#include "libgl/hud/CGlWindow.hpp"
#include "libgl/hud/CGlRenderOStream.hpp"
#include "libgl/hud/CGlHudConfig.hpp"
#include "libgl/core/CGlGetString.hpp"

#include "libmath/CGlSlMath.hpp"

#include "mainvis/CRenderPass.hpp"

/**
 * \brief main visualization class to handle initialization, rendering and control of the visualization
 */
template <typename T>
void CMainVisualization<T>::callback_key_down(int key, int mod, int scancode, int unicode)
{
	switch(key)
	{
		case 'w':				// switch window mode / fullscreen
			cRenderWindow.setWindowFullscreenState(!cRenderWindow.fullscreen_active);
			if (cRenderWindow.error())
				std::cerr << cRenderWindow.error.getString();
			break;

		case 'e':				// soft reset angle
			resetPerspectiveAndPosition();
			break;

		case CRenderWindow::KEY_PAGEUP:
			cConfig.perspective_zoom += 0.5;
			break;

		case CRenderWindow::KEY_PAGEDOWN:
			cConfig.perspective_zoom -= 0.5;
			break;

		default:
			cSwitches.key_pressed(key, mod);
	}
}

template <typename T>
void CMainVisualization<T>::callback_key_up(int key, int mod, int scancode, int unicode)
{
}

template <typename T>
void CMainVisualization<T>::callback_quit()
{
	cConfig.quit = true;
}

	/**
	 * callback handlers for simulation control
	 */
template <typename T>
void CMainVisualization<T>::updateViscosityCallback(void *user_ptr)
{
	CMainVisualization &v = (*(CMainVisualization*)user_ptr);
	v.updateViscosity();
}

template <typename T>
void CMainVisualization<T>::updateViscosity()
{
	cLbmOpenCl_ptr->updateViscosity(cConfig.lbm_simulation_viscosity);
}

template <typename T>
void CMainVisualization<T>::updateMassExchangeFactorCallback(void *user_ptr)
{
	CMainVisualization &v = (*(CMainVisualization*)user_ptr);
	v.updateMassExchangeFactor();

}

template <typename T>
void CMainVisualization<T>::updateMassExchangeFactor()
{
	cLbmOpenCl_ptr->updateMassExchangeFactor(cConfig.lbm_simulation_mass_exchange_factor);
}


template <typename T>
void CMainVisualization<T>::updateDomainXLengthCallback(void *user_ptr)
{
	CMainVisualization &v = (*(CMainVisualization*)user_ptr);
	v.updateDomainXLength();
}

template <typename T>
void CMainVisualization<T>::updateDomainXLength()
{
	cLbmOpenCl_ptr->updateDomainXLength(cConfig.lbm_simulation_domain_x_length);
}


template <typename T>
void CMainVisualization<T>::updateGravitationCallback(void *user_ptr)
{
	CMainVisualization &v = (*(CMainVisualization*)user_ptr);
	v.updateGravitation();
}


template <typename T>
void CMainVisualization<T>::updateGravitation()
{
	if (cLbmOpenCl_ptr != NULL && cRenderPass != NULL)
	{
		gravitation[1] = cConfig.lbm_simulation_gravitation;
		float gravitation_length = GLSL::length(gravitation);

		if (gravitation_length > 0.0000000001)
		{
			GLSL::vec3 gravitation_vector;

			gravitation_vector = GLSL::mat3(GLSL::transpose(cRenderPass->cMatrices.model_matrix))*gravitation;
			gravitation_vector = GLSL::normalize(gravitation_vector)*gravitation_length;

			cLbmOpenCl_ptr->updateGravitation(CVector<3,T>(gravitation_vector[0], gravitation_vector[1], gravitation_vector[2]));
		}
		else
		{
			cLbmOpenCl_ptr->updateGravitation(CVector<3,T>(0,0,0));
		}

		if (cLbmOpenCl_ptr->error())
		{
			lbm_simulation_error = true;
			lbm_simulation_error_message = cLbmOpenCl_ptr->error.getString();
		}
		else
		{
			lbm_simulation_error = false;
		}
	}
}



template <typename T>
void CMainVisualization<T>::callback_mouse_motion(int x, int y)
{
	float px = (float)x;
	float py = (float)y;

	if (button_down[2])			// middle mouse button
	{
		cModelEyeBall.rotate(-old_mouse_x + px, cViewEyeBall.up);
		cModelEyeBall.rotate(-old_mouse_y + py, cViewEyeBall.right);

		updateGravitation();
	}
	else if (button_down[3])			// right mouse button
	{
		cGroundEyeBall.rotate(-old_mouse_x + px, cViewEyeBall.up);
		cGroundEyeBall.rotate(-old_mouse_y + py, cViewEyeBall.right);
	}
	else if (button_down[1])						// left mouse button
	{
		cViewEyeBall.rotate(-old_mouse_x + px, -old_mouse_y + py, 0);
	}

	old_mouse_x = px;
	old_mouse_y = py;

	cConfig.mouse_motion(x, cRenderWindow.window_height - y);
}


template <typename T>
void CMainVisualization<T>::callback_mouse_button_down(int button)
{
	// if the button was pressed down within the config window, do nothing
	if (cConfig.mouse_button_down(button))
		return;

#if 0
	if (button <= 3)
		button_down[button] = true;
	else if (button == 4)
		perspective_zoom--;
	else if (button == 5)
		perspective_zoom++;
#else
	if (button <= 3)
		button_down[button] = true;
#endif
}


template <typename T>
void CMainVisualization<T>::callback_mouse_wheel(int x, int y)
{
	if (cConfig.mouse_wheel(x, y))
		return;

	cConfig.distance_to_center += -0.33*(float)y;
}

template <typename T>
void CMainVisualization<T>::callback_mouse_button_up(int button)
{
	if (button <= 3)
		button_down[button] = false;

	cConfig.mouse_button_up(button);
}


template <typename T>
void CMainVisualization<T>::callback_viewport_changed(int width, int height)
{
	if (initialized)
	{
		cRenderPass->viewport_resize(width, height);
	}

	debug_window.setPosition(GLSL::ivec2(10, cRenderWindow.window_height-210));
	debug_window.setSize(GLSL::ivec2(400, 200));
}

/**
 * constructor for the main visualization class
 *
 * this constructor creates the cGui which initializes the window and rendering context
 *
 * after that, the context can be used to derive the OpenCL context
 */

template <typename T>
CMainVisualization<T>::CMainVisualization(
		CWiiBalanceBoard &p_cWiiBalanceBoard,
		bool p_verbose
	)		:
	initialized(false),
//		cGui(*this, "Free Surface Lattice Boltzmann, OpenCL 1.0 & OpenGL 3.2 Core Mode", 1024, 800)
	cRenderWindow(*this, "Free Surface Lattice Boltzmann, OpenCL 1.0 & OpenGL 3.2 Core Mode", 800, 600),
	cSwitches(cConfig),
	rostream(free_type),
	cWiiBalanceBoard(p_cWiiBalanceBoard)
{
	verbose = p_verbose;

	if (verbose)
	{
		std::cout << cRenderWindow.message.getString();

		std::cout << "GL Vendor: " << CGlGetString::getVendor() << std::endl;
		std::cout << "GL Version: " << CGlGetString::getVersion() << std::endl;
		std::cout << "GL Renderer: " << CGlGetString::getRenderer() << std::endl;
	}

	cLbmOpenCl_ptr = NULL;
	cCLSkeleton_ptr = NULL;

	button_down[0] = button_down[1] = button_down[2] = button_down[3] = button_down[4] = false;

	ticks = 0;
	initialized = true;
}


/**
 * reset view and model matrix as well as zoom and distance to center
 */

template <typename T>
void CMainVisualization<T>::resetPerspectiveAndPosition()
{
	cViewEyeBall.reset();
	cModelEyeBall.reset();
	cGroundEyeBall.reset();

	cConfig.resetPerspectiveAndPosition();

	if (cConfig.lbm_simulation_run && cLbmOpenCl_ptr != NULL)
	{
		updateGravitation();
	}
}


/**
 * initialize the visualisation
 */

template <typename T>
void CMainVisualization<T>::init(	CLbmOpenClInterface<T>	*p_cLbmOpenCl_ptr,			///< pointer to OpenCL Lattice Boltzmann simulation class or NULL to deactivate
			CCLSkeleton		*p_cCLSkeleton_ptr,			///< pointer to OpenCL skeleton or NULL to deactivate OpenCL usage
			CVector<3,int>	&p_domain_cells,			///< domain size for test datasets
			CVector<3,float>	&p_gravitation
		)
{
	cLbmOpenCl_ptr = p_cLbmOpenCl_ptr;
	cCLSkeleton_ptr = p_cCLSkeleton_ptr;
	domain_cells = p_domain_cells;
	gravitation = p_gravitation;
}



template <typename T>
void CMainVisualization<T>::setup_lbm_init_flags()
{
	/*
	 * setup initialization flags for fluid data
	 */
	int lbm_init_flags = 0;
	if (cConfig.lbm_simulation_init_breaking_dam)			lbm_init_flags |= CLbmOpenClInterface<T>::INIT_CREATE_BREAKING_DAM;
	if (cConfig.lbm_simulation_init_pool)					lbm_init_flags |= CLbmOpenClInterface<T>::INIT_CREATE_POOL;
	if (cConfig.lbm_simulation_init_water_sphere)			lbm_init_flags |= CLbmOpenClInterface<T>::INIT_CREATE_SPHERE;
	if (cConfig.lbm_simulation_init_obstacle_half_sphere)	lbm_init_flags |= CLbmOpenClInterface<T>::INIT_CREATE_OBSTACLE_HALF_SPHERE;
	if (cConfig.lbm_simulation_init_obstacle_vertical_bar)	lbm_init_flags |= CLbmOpenClInterface<T>::INIT_CREATE_OBSTACLE_VERTICAL_BAR;
	if (cConfig.lbm_simulation_init_fluid_with_gas_sphere)	lbm_init_flags |= CLbmOpenClInterface<T>::INIT_CREATE_FLUID_WITH_GAS_SPHERE;
	if (cConfig.lbm_simulation_init_filled_domain)			lbm_init_flags |= CLbmOpenClInterface<T>::INIT_CREATE_FILLED_CUBE;

	cLbmOpenCl_ptr->setupInitFlags(lbm_init_flags);

}


/**
 * \brief this starts the main rendering loop and returns if the window is going to be closed
 */

template <typename T>
void CMainVisualization<T>::renderLoop()
{
	/*
	 * SETUP HUD
	 */
	free_type.setup(12);
	CError_AppendReturn(free_type);

	free_type.setNewlineXPosition(10);

	// set viewport size to window size
//		free_type.viewportChanged(window.size);

	cConfig.setup(free_type, rostream);

	debug_window.setSize(GLSL::ivec2(100, 100));
	debug_window.setBackgroundColor(GLSL::vec4(0.9, 0.9, 0.5, 0.8));

	CStopwatch stopwatch;

	cRenderPass = NULL;
	resetPerspectiveAndPosition();

	T *fluid_mass_buffer = NULL;
	cl_int *fluid_flag_buffer = NULL;

	if (cLbmOpenCl_ptr != NULL)
	{
		fluid_mass_buffer = new T[domain_cells.elements()];
		fluid_flag_buffer = new cl_int[domain_cells.elements()];

		cConfig.lbm_simulation_gravitation = gravitation[1];
		cConfig.set_callback(&cConfig.lbm_simulation_gravitation, updateGravitationCallback, this);

		cConfig.lbm_simulation_viscosity = cLbmOpenCl_ptr->params.d_viscosity;
		cConfig.set_callback(&cConfig.lbm_simulation_viscosity, updateViscosityCallback, this);

		cConfig.lbm_simulation_domain_x_length = cLbmOpenCl_ptr->params.d_domain_x_length;
		cConfig.set_callback(&cConfig.lbm_simulation_domain_x_length, updateDomainXLengthCallback, this);

		cConfig.lbm_simulation_mass_exchange_factor = cLbmOpenCl_ptr->params.mass_exchange_factor;
		cConfig.set_callback(&cConfig.lbm_simulation_mass_exchange_factor, updateMassExchangeFactorCallback, this);
	}


	/**
	 * TIMERS
	 */
	float current_fps = 0;
	float last_ticks_taken = cRenderWindow.getTicks();
	float last_fps_taken = 0;
	float last_frame_counter = 0;
	float frame_counter = 0;

	while (!cConfig.quit)
	{
		/// create and initialize render pass
		CRenderPass<T> r(cRenderWindow, *this, cConfig, cLbmOpenCl_ptr, verbose);
		cRenderPass = &r;

		CError_AppendReturn(r);

		/**
		 * initialize rendering
		 */
		r.init();
		CError_AppendReturn(r);

		// setup flags
		CGlStateEnable state_cull_face(GL_CULL_FACE);
		CGlStateEnable state_depth_test(GL_DEPTH_TEST);

// TODO: disabled since NV driver version 260.19.12 segfaults
//			CGlStateEnable state_cube_map_seamless(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		// initialize viewport
		callback_viewport_changed(cRenderWindow.window_width, cRenderWindow.window_height);

		if (cConfig.lbm_simulation_run && cLbmOpenCl_ptr != NULL)
			updateGravitation();

		std::ostringstream screenshot_drop_message;

		cConfig.reset = false;
		while (!cConfig.reset && !cConfig.quit)
		{
			CGlErrorCheck();
			cRenderWindow.eventLoop();

			/*
			 * TIMERS
			 */
			double current_ticks = cRenderWindow.getTicks();

			float delta_ticks = current_ticks - last_ticks_taken;
			if (cConfig.increment_ticks)
				ticks += delta_ticks;
			last_ticks_taken = current_ticks;

			double delta_fps_ticks = current_ticks - last_fps_taken;

			if (delta_fps_ticks >= 1.0)
			{
				current_fps = (frame_counter-last_frame_counter) / delta_fps_ticks;
				last_fps_taken = current_ticks;
				last_frame_counter = frame_counter;
			}

			/**
			 * setup light position
			 */
			if (cConfig.light0_rotation)
				r.cGlLights.light0_world_pos4 = GLSL::rotate((float)ticks*50, 0, 1, 0)*cConfig.light0_world_position;
			else
				r.cGlLights.light0_world_pos4 = cConfig.light0_world_position;

			r.cGlLights.light0_enabled = true;
			r.cGlLights.light0_ambient_color3 = cConfig.light0_ambient_color3;
			r.cGlLights.light0_diffuse_color3 = cConfig.light0_diffuse_color3;
			r.cGlLights.light0_specular_color3 = cConfig.light0_specular_color3;

			/**
			 * INITIALIZE MATRICES
			 */

			/**
			 * model matrix
			 */
			r.cMatrices.model_matrix = GLSL::mat4();	/// initialize with identity matrix

			if (cConfig.random_model_rotation)
			{
				r.cMatrices.model_matrix *= GLSL::rotate(((float)rand()*360.0f)/(float)RAND_MAX, 1.0f, 0.0f, 0.0f);
				r.cMatrices.model_matrix *= GLSL::rotate(((float)rand()*360.0f)/(float)RAND_MAX, 0.0f, 1.0f, 0.0f);
				r.cMatrices.model_matrix *= GLSL::rotate(((float)rand()*360.0f)/(float)RAND_MAX, 0.0f, 0.0f, 1.0f);
			}

#ifdef CWIIBALANCEBOARD
			cWiiBalanceBoard.updateVectorAndWeight();
//			std::cout << cWiiBalanceBoard.vector_x << " " << cWiiBalanceBoard.vector_y << std::endl;

			cModelEyeBall.rotate(cWiiBalanceBoard.vector_x, cWiiBalanceBoard.vector_y, 0);
#endif
			// rotation by user interaction
			cModelEyeBall.reconstructRotationMatrix();
			r.cMatrices.model_matrix *= cModelEyeBall.rotationMatrix;

			if (cConfig.rotate_central_object)
			{
				r.cMatrices.model_matrix *= GLSL::rotate(	-(float)ticks*1.0f+0.0f,
															CMath::sin<float>(ticks*0.6)*0.6,
															0.55+CMath::cos<float>(ticks*0.5)*0.5,
															CMath::sin<float>(ticks*0.79)*0.635
														);
			}

			updateGravitation();

			cGroundEyeBall.reconstructRotationMatrix();
			r.diffuse_receivers_model_matrix = GLSL::scale(2.5f, 2.5f, 2.5f);
			r.diffuse_receivers_model_matrix *= GLSL::translate(0.0f, -1.6f, 0.0f);
			r.diffuse_receivers_model_matrix *= cGroundEyeBall.rotationMatrix;


			/*
			 * view and projection matrix
			 */
			if (cConfig.view_pos_at_light_space_box)
			{
				// initialize the expandable viewbox for this frame (TODO: fix me: this is done twice - again in render state)
				r.prepare_expandable_viewbox();

				r.cMatrices.view_matrix = cRenderPass->cGlExpandableViewBox.lsb_view_matrix;

				r.cMatrices.projection_matrix = GLSL::scale(0.9, 0.9, 1.0)*cRenderPass->cGlExpandableViewBox.lsb_projection_matrix;
			}
			else
			{
				float near_plane = 0.5f;
				float far_plane = 40.0f;
				float open_angle = 120.0f*(1.0f/180.0f)*CMath::PI<float>();
				float open_angle_factor = 1.0f/CMath::cos<float>(open_angle*0.5f);
				float inv_zoom = CMath::exp<float>(cConfig.perspective_zoom*0.05f)*near_plane*open_angle_factor;
				r.cMatrices.projection_matrix = GLSL::frustum(
						-inv_zoom, inv_zoom,
						-inv_zoom/cRenderWindow.aspect_ratio, inv_zoom/cRenderWindow.aspect_ratio,
						near_plane, far_plane
						);

				r.cMatrices.view_matrix = GLSL::translate(0.0f, 0.0f, -cConfig.distance_to_center);

				cViewEyeBall.reconstructRotationMatrixInverse();
				r.cMatrices.view_matrix *= cViewEyeBall.rotationMatrix;

				r.cMatrices.view_matrix *= GLSL::translate(cConfig.view_translation);
			}

			if (cConfig.random_view_rotation)
			{
				r.cMatrices.view_matrix *= GLSL::rotate(((float)rand()*360.0f)/(float)RAND_MAX, 1.0f, 0.0f, 0.0f);
				r.cMatrices.view_matrix *= GLSL::rotate(((float)rand()*360.0f)/(float)RAND_MAX, 0.0f, 1.0f, 0.0f);
				r.cMatrices.view_matrix *= GLSL::rotate(((float)rand()*360.0f)/(float)RAND_MAX, 0.0f, 0.0f, 1.0f);
			}


			/**
			 * initialize more matrices
			 */
			r.cMatrices.view_model_matrix = r.cMatrices.view_matrix * r.cMatrices.model_matrix;
			r.cMatrices.view_model_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(r.cMatrices.view_model_matrix));
			r.cMatrices.view_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(r.cMatrices.view_matrix));

			r.cMatrices.pvm_matrix = r.cMatrices.projection_matrix * r.cMatrices.view_model_matrix;

			/**************************
			 * SIMULATION CONTROL
			 **************************/

			/**************************
			 * simulation reset & reset of fluid
			 **************************/
			if (cConfig.lbm_simulation_reset_simulation)
			{
				if (cLbmOpenCl_ptr != NULL)
				{
					setup_lbm_init_flags();
					cLbmOpenCl_ptr->reload();
#if LBM_OPENCL_FS_DEBUG_CHECKSUM_VELOCITY
					lbm_sim_count = 0;
#endif

					if (cLbmOpenCl_ptr->error())
						std::cout << "ERROR ON SIM RESET: " << cLbmOpenCl_ptr->error.getString() << "|" << std::endl;
				}
				cConfig.lbm_simulation_reset_simulation = false;
			}

			if (cConfig.lbm_simulation_reset_fluid)
			{
				if (cLbmOpenCl_ptr != NULL)
				{
					setup_lbm_init_flags();
					cLbmOpenCl_ptr->resetFluid();
				}
				cConfig.lbm_simulation_reset_fluid = false;
			}

			/**
			 * SIMULATION STEP
			 */

			// time of the last simulation step
			static double last_simulation_time_step = stopwatch.getTime();
			float simulation_mass = 0.0;
			int lbm_simulation_timesteps_to_do = 0;

			if (!cConfig.lbm_simulation_run)
			{
				last_simulation_time_step = stopwatch.getTime();
			}

			if (cLbmOpenCl_ptr != NULL)
			{
				if (cConfig.lbm_simulation_run)
				{
#if LBM_OPENCL_FS_DEBUG_CHECKSUM_VELOCITY
					int max_count = 100;
					if (ALPHA_KERNEL_AS_PROPAGATION || BETA_KERNEL_AS_PROPAGATION)
						max_count *= 2;

					if (lbm_sim_count < max_count)
					{
						cLbmOpenCl_ptr->simulationStep();
						lbm_sim_count++;
					}
					else
					{
						cLbmOpenCl_ptr->storeMass(fluid_mass_buffer);
						cLbmOpenCl_ptr->storeFlags(fluid_flag_buffer);
						simulation_mass = 0.0;
						for (int i = 0; i < domain_cells.elements(); i++)
						{
							if (fluid_flag_buffer[i] & (LBM_FLAG_FLUID | LBM_FLAG_INTERFACE))
								simulation_mass += fluid_mass_buffer[i];
						}

						std::cout << "simulation density/mass checksum: " << cLbmOpenCl_ptr->getDensityChecksum() << " " << simulation_mass << std::endl;
						cLbmOpenCl_ptr->reload();
						lbm_sim_count = 0;
					}
#else
					if (cConfig.lbm_simulation_multiple_timesteps_per_frame)
					{
						double current_time = stopwatch.getTime();
						double delta_time = current_time - last_simulation_time_step;

						lbm_simulation_timesteps_to_do = delta_time/(cLbmOpenCl_ptr->params.d_timestep);

						int max_timesteps = 100;
						screenshot_drop_message.str("");
						if (lbm_simulation_timesteps_to_do > max_timesteps)
						{
							screenshot_drop_message << "[" << current_time << "]: timestep dropping (" << lbm_simulation_timesteps_to_do << ")! (more than " << max_timesteps << " timesteps per frame)" << std::endl;

							lbm_simulation_timesteps_to_do = max_timesteps;
							last_simulation_time_step = current_time;
						}
						else
						{
							if (verbose)
								screenshot_drop_message << lbm_simulation_timesteps_to_do << " timesteps per frame";
							last_simulation_time_step += (double)lbm_simulation_timesteps_to_do * (double)cLbmOpenCl_ptr->params.d_timestep;
						}

//							std::cout << lbm_simulation_timesteps_to_do << std::endl;
						for (int i = 0; i < lbm_simulation_timesteps_to_do; i++)
							cLbmOpenCl_ptr->simulationStep();
					}
					else
					{
						cLbmOpenCl_ptr->simulationStep();
					}
#endif
				}

				if (cConfig.lbm_simulation_count_simulation_mass)
				{
					simulation_mass = cLbmOpenCl_ptr->getMassReduction();
				}

				/**
				 * bunch of validation checks
				 *
				 * COMMENT OUT if not necessary!!!
				 */
#if 0
				static bool validation_output = false;
//					if (!validation_output)
				{
					 validation_output = cLbmOpenCl_ptr->validationChecks();
				}
#endif

				if (cConfig.lbm_simulation_normalize_simulation_mass)
					cLbmOpenCl_ptr->normalizeSimulationMass();

			}


			/**
			 * PREPARE RENDERING
			 */

			// clear background
			glClearColor(1.0, 1.0, 1.0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//				glClear(GL_DEPTH_BUFFER_BIT);


			/*****************************************
			 * RENDER"
			 *****************************************/
			r.render();


			/*****************************************
			 * HUD
			 *****************************************/
			if (cConfig.render_hud)
			{
				cConfig.render();
			}

			/**
			 * output only fps
			 */
			if (false)
			if (cConfig.output_fps_and_parameters)
			{
				debug_window.startRendering();

				free_type.viewportChanged(debug_window.size);
				free_type.setPosition(GLSL::ivec2(5, debug_window.size[1]-1 - 5 - free_type.font_size));
				free_type.setColor(GLSL::vec4(0,0,0,1));

				rostream << "FPS: " << current_fps;
#if LBM_OPENCL_FS_DEBUG
				rostream << "    WARNING: DEBUG MODE ACTIVE (slower)" << std::endl;
#endif

				if (cLbmOpenCl_ptr != NULL)
				{
					if (cConfig.lbm_simulation_count_simulation_mass)
						rostream << "    simulation mass: " << simulation_mass << std::endl;

					rostream << "    simulation mass on reset: " << cLbmOpenCl_ptr->simulation_mass_on_reset << std::endl;

					rostream << "SIM simulation step counter: " << cLbmOpenCl_ptr->simulation_step_counter << std::endl;
					rostream << "SIM gravitation: " << cLbmOpenCl_ptr->params.gravitation << std::endl;
					rostream << "SIM d_timestep: " << cLbmOpenCl_ptr->params.d_timestep << std::endl;
					rostream << "SIM inv_tau: " << cLbmOpenCl_ptr->params.inv_tau << std::endl;
					rostream << "SIM inv_trt_tau: " << cLbmOpenCl_ptr->params.inv_trt_tau << std::endl;
					rostream << "SIM domain x length: " << cLbmOpenCl_ptr->params.d_domain_x_length << std::endl;

					if (lbm_simulation_error)
						rostream << "SIM error message: " << lbm_simulation_error_message << std::endl;
				}

				rostream << screenshot_drop_message.str() << std::endl;

				debug_window.finishRendering();
			}

			if (cConfig.output_world_position_at_mouse_cursor)
			{
				free_type.setPosition(GLSL::ivec2(5, cRenderWindow.window_height-1 - 5 - free_type.font_size*2));
				free_type.setColor(GLSL::vec4(0,0,0,1));

				GLSL::vec4 screen_pos;
				screen_pos[0] = ((float)old_mouse_x + 0.5f)/(float)cRenderWindow.window_width;
				screen_pos[1] = ((float)(cRenderWindow.window_height - old_mouse_y - 1) + 0.5f)/(float)cRenderWindow.window_height;
				glReadPixels(old_mouse_x, cRenderWindow.window_height - old_mouse_y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &screen_pos[2]);

				screen_pos[0] = screen_pos[0]*2.0-1.0;
				screen_pos[1] = screen_pos[1]*2.0-1.0;
				screen_pos[2] = screen_pos[2]*2.0-1.0;
				screen_pos[3] = 1.0f;

				GLSL::vec4 world_vec4 = GLSL::inverse(r.cMatrices.projection_matrix*r.cMatrices.view_matrix)*screen_pos;
				world_vec4[0] /= world_vec4[3];
				world_vec4[1] /= world_vec4[3];
				world_vec4[2] /= world_vec4[3];
				world_vec4[3] /= world_vec4[3];
				rostream << "World position for mouse cursor: " << GLSL::vec3(world_vec4) << std::endl;
			}


			if (cConfig.take_screenshot)
			{
				cRenderWindow.saveScreenshotWithThread("screenshot.bmp");
				cConfig.take_screenshot = false;
			}

			if (cConfig.take_screenshot_series)
			{
				static double next_screenshot_time = stopwatch.getTime();

				double current_time = stopwatch.getTime();
				int screenshot_fps = 20;

				double delta_time = 1.0/(double)screenshot_fps;

				if (next_screenshot_time <= current_time)
				{
					static int screenshot_nr = 0;
					std::ostringstream screenshot_file_stream;
					screenshot_file_stream << "screenshots/screenshot_";
					screenshot_file_stream.setf(std::ios::fixed, std::ios::basefield);
					screenshot_file_stream.width(8);
					screenshot_file_stream.fill('0');
					screenshot_file_stream << screenshot_nr << ".bmp";

					std::string screenshot_file = screenshot_file_stream.str();
					cRenderWindow.saveScreenshotWithThread(screenshot_file);
					screenshot_nr++;

					next_screenshot_time += delta_time;
					if (next_screenshot_time < current_time)
					{
						std::cout << "dropping screenshot(s)" << std::endl;
						next_screenshot_time = current_time;
					}
				}
			}

			cRenderWindow.swapBuffer();
			frame_counter++;

		}	// reset-while

		cRenderPass = NULL;
	}	// quit-while


	if (cLbmOpenCl_ptr != NULL)
	{
		delete [] fluid_flag_buffer;
		delete [] fluid_mass_buffer;
	}
}

/**
 * this function has to be called after the run() function returned
 *
 * the opencl part has to be shut down separately because first the opencl parts have to be freed,
 * then the gui can quit because OpenCL interops depend on an existing OpenGL context
 */

template <typename T>
void CMainVisualization<T>::shutdownOpenCLParts()
{
}


template class CMainVisualization<float>;
//template class CMainVisualization<double>;
