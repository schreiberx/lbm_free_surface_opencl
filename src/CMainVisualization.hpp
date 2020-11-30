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

#ifndef CMAIN_VISUALIZATION_HPP
#define CMAIN_VISUALIZATION_HPP

#include "mainvis/CConfig.hpp"
#include "libgl/core/CGlTexture.hpp"
#include "mainvis/CSwitches.hpp"
#include "mainvis/CRenderPass.hpp"

#include "libmath/CVector.hpp"
#include "lib/CError.hpp"
#include "lib/CEyeBall.hpp"
#include "CWiiBalanceBoard.hpp"

#include "mainvis/CRenderWindow.hpp"
/**
 * \brief main visualization class to handle initialization, rendering and control of the visualization
 */
template <typename T>
class CMainVisualization : public RenderWindowEventCallbacks
{

public:
	CError error;	///< error handler

	CVector<3,int> domain_cells;	///< storage for domain cells to drive the simulation or to create the dummy visualization data on
	double ticks;	///< ticks in seconds. this is useful for animations

private:
	bool initialized;	///< initialization (constructor) successful
	bool verbose;	///< be verbose

	// parameter for view matrix
	CEyeBall<float> cViewEyeBall;

	// parameter for model matrix
	CEyeBall<float> cModelEyeBall;

	// parameter for ground table rotation
	CEyeBall<float> cGroundEyeBall;

	// last mouse position
	float old_mouse_x, old_mouse_y;

	// gravitation vector
	CVector<3,float> gravitation;

	// pressed mouse buttons
	bool button_down[5];

	CRenderWindow cRenderWindow;

	CGlTexture *test_volume_texture;


	CSwitches cSwitches;
	CConfig cConfig;

	CRenderPass<T> *cRenderPass;


	bool lbm_simulation_error;
	std::string lbm_simulation_error_message;

	void callback_key_down(int key, int mod, int scancode, int unicode);

	void callback_key_up(int key, int mod, int scancode, int unicode);

	void callback_quit();

private:
	/**
	 * callback handlers for simulation control
	 */
	static void updateViscosityCallback(void *user_ptr);

	void updateViscosity();

	static void updateMassExchangeFactorCallback(void *user_ptr);

	void updateMassExchangeFactor();

	static void updateDomainXLengthCallback(void *user_ptr);

	void updateDomainXLength();

	static void updateGravitationCallback(void *user_ptr);

	void updateGravitation();

public:
	void callback_mouse_motion(int x, int y);

	void callback_mouse_button_down(int button);

	void callback_mouse_wheel(int x, int y);

	void callback_mouse_button_up(int button);

	void callback_viewport_changed(int width, int height);

public:

	CLbmOpenClInterface<T>	*cLbmOpenCl_ptr;				///< pointer to lbm OpenCl simulation class or NULL
	CCLSkeleton			*cCLSkeleton_ptr;				///< pointer to OpenCl skeleton or NULL

	CGlFreeType free_type;		///< standard font class to draw some characters on screen
	CGlRenderOStream rostream;	///< ro stream to draw strings using operator<< on screen using the class free_type as default font

	CWiiBalanceBoard &cWiiBalanceBoard;

	CGlWindow debug_window;			///< window with debug information

	/**
	 * constructor for the main visualization class
	 *
	 * this constructor creates the cGui which initializes the window and rendering context
	 *
	 * after that, the context can be used to derive the OpenCL context
	 */
	CMainVisualization(
			CWiiBalanceBoard &p_cWiiBalanceBoard,
			bool p_verbose = true
		);

	/**
	 * reset view and model matrix as well as zoom and distance to center
	 */
	void resetPerspectiveAndPosition();


	/**
	 * initialize the visualisation
	 */
	void init(	CLbmOpenClInterface<T>	*p_cLbmOpenCl_ptr,			///< pointer to OpenCL Lattice Boltzmann simulation class or NULL to deactivate
				CCLSkeleton		*p_cCLSkeleton_ptr,			///< pointer to OpenCL skeleton or NULL to deactivate OpenCL usage
				CVector<3,int>	&p_domain_cells,			///< domain size for test datasets
				CVector<3,float>	&p_gravitation
			);

private:

	void setup_lbm_init_flags();


public:

	/**
	 * \brief this starts the main rendering loop and returns if the window is going to be closed
	 */
	void renderLoop();

	/**
	 * this function has to be called after the run() function returned
	 *
	 * the opencl part has to be shut down separately because first the opencl parts have to be freed,
	 * then the gui can quit because OpenCL interops depend on an existing OpenGL context
	 */
	void shutdownOpenCLParts();
};

#endif
