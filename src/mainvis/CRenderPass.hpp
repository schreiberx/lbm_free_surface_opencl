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
* CRenderPass.hpp
*
*  Created on: Mar 22, 2010
*      Author: martin
*/

#ifndef CRENDERPASS_HPP_
#define CRENDERPASS_HPP_

#include "libmath/CMath.hpp"
#include "libgl/core/CGlTexture.hpp"


// define GL_INTEROP to load OpenGL INTEROP support (usually set up via preprocessor variable)
#include "lbm/CLbmOpenClInterface.hpp"

#include "CRenderWindow.hpp"
#include "mainvis/CConfig.hpp"
#include "mainvis/CMarchingCubesRenderers.hpp"
#include "mainvis/CObjectRenderers.hpp"
#include "mainvis/CShaders.hpp"
#include "mainvis/CSwitches.hpp"
#include "mainvis/CTextures.hpp"
#include "mainvis/CVolumeTexturesAndRenderers.hpp"
#include "mainvis/CMatrices.hpp"

#include "libgl/CGlExpandableViewBox.hpp"
#define DEBUG_CHECKSUM_VELOCITY		0
#include "libgl/CGlRenderCallbackTypes.hpp"

/**
 * this class implements the rendering functions:
 *
 *  - initialization
 *  - rendering
 *  - cleanup
 *
 * when created, it does all the rendering stuff (except e.g. the HUD which is handled by CMainVisualization).
 */
template <typename T> class CMainVisualization;
template <typename T> class CRenderPass_Private;

// use typename for renderpass which has to be of the same type as CMainVisualization<T>
template <typename T>
class CRenderPass
{
	bool verbose;

	CGlTexture *volume_texture;				///< pointer to volume texture which has to be rendered

	CRenderWindow &cRenderWindow;
	class CMainVisualization<T> &cMain;
	CConfig &cConfig;
	CLbmOpenClInterface<T> *cLbmOpenCl_ptr;

	class CMarchingCubesRenderers cMarchingCubesRenderers;

	class CObjectRenderers cObjectRenderers;
	class CTextures cTextures;
	class CShaders cShaders;

public:
	class CGlLights cGlLights;

	class CRenderPass_Private<T> *private_class;

	// 3d texture to store fluid fraction (copying directly, is still not supported with current nvidia drivers
	CGlTexture fluid_fraction_volume_texture;

	CGlExpandableViewBox cGlExpandableViewBox;

	CError error;				///< error information

	CMatrices cMatrices;		///< matrices class used for modifications and parameter for renderers

	GLSL::mat4 central_object_scale_matrix;		///< scale matrix for central object
	GLSL::mat4 refraction_object_model_matrix;	///< model matrix for refractive objects
	GLSL::mat4 diffuse_receivers_model_matrix;	///< model matrix for diffuse receivers (e. g. table)



	CRenderPass(	CRenderWindow &p_cRenderWindow,
					class CMainVisualization<T> &p_cMain,
					CConfig &p_cConfig,
					CLbmOpenClInterface<T> *p_cLbmOpenCl_ptr,
					bool p_verbose
			);


	static void createCubeMapCallback(GLSL::mat4 &projection_matrix, GLSL::mat4 &view_matrix, void *user_data);


	/**
	 * this function is called from CMain when the viewport has to be resized
	 */
	void viewport_resize(GLsizei width, GLsizei height);


	/**
	 * this callback function is called, when the peeling texture for photon mapping should be resized
	 */
	static void callback_photon_mapping_peeling_texture_resized(void *user_ptr);

	/**
	 * this callback function is called, when the photon texture for photon mapping should be resized
	 */
	static void callback_photon_mapping_photon_texture_resized(void *user_ptr);

	/**
	 * initialize everything
	 */
	void init();


private:

	/**
	 * render some textured quads
	 */
	void render_animated_quads(	GLSL::mat4 &projection_matrix,
								GLSL::mat4 &view_matrix
							);

	/**
	 * render fancy rotating balls
	 */
	void render_animated_balls(	GLSL::mat4 &projection_matrix,
								GLSL::mat4 &view_matrix
							);
	/**
	 * render fancy rotating meshes
	 */
	void render_animated_meshes(	GLSL::mat4 &projection_matrix,
									GLSL::mat4 &view_matrix
								);


	void render_table(	GLSL::mat4 &projection_matrix,
						GLSL::mat4 &view_matrix
	);


	/**
	 * render table without lighting
	 */
	void render_table_no_lighting(	GLSL::mat4 &projection_matrix,
									GLSL::mat4 &view_matrix
					);


	/**
	 * render table with shadow mapping
	 */
	void render_table_shadow_mapping(
						GLSL::mat4 &projection_matrix,
						GLSL::mat4 &view_matrix,
						CGlTexture &depth_texture,
						GLSL::mat4 &lsb_view_matrix,
						GLSL::mat4 &lsb_projection_matrix
					);


	/**
	 * render table with shadow and caustic mapping
	 */
	void render_table_shadow_and_caustic_mapping(
						GLSL::mat4 &projection_matrix,
						GLSL::mat4 &view_matrix,
						CGlTexture &depth_texture,
						CGlTexture &caustic_map_texture,
						CGlTexture &caustic_map_depth_texture,
						GLSL::mat4 &lsb_view_matrix,
						GLSL::mat4 &lsb_projection_matrix
					);

	void render_scene_objects(GLSL::mat4 &projection_matrix, GLSL::mat4 &view_matrix);

	void render_fbo_test();

	void render_marching_cubes_with_refractions();

	void render_volume_casting(CGlTexture *volume_texture);

	/**
	 * convert a flat texture created by copying from opencl to a flat texture with adjacent slices
	 *
	 * then, extract the marching cubes
	 *
	 * this function is usually called by every function which relies on the extracted marching cubes vertices
	 */
	void render_flat_volume_texture_to_flat_texture_and_extract(CGlTexture *volume_texture);

	void render_marching_cubes_solid(CGlTexture *volume_texture);

	static void render_photonmapping_front_back_volume_callback(
						class CGlVolumeRendererInterpolatedFrontBackFaces &renderer,
						void* user_data,
						const GLSL::mat4 &projection_matrix,
						const GLSL::mat4 &view_matrix	);

	/**
	 * PHOTON MAPPING preparation
	 */
	void render_photonmapping_front_back_faces_prepare(CGlExpandableViewBox &cGlExpandableViewBox);


	/*
	 * PHOTON MAPPING using only front and back faces
	 */
	void render_photonmapping_front_back_faces(CGlExpandableViewBox &cGlExpandableViewBox);


	/**
	 * render refractive objects only (for photon mapping, caustics, refractions, etc.)
	 */
	static void callback_render_refractive_objects(
									const GLSL::mat4 &projection_matrix,	///< projection matrix
									const GLSL::mat4 &view_matrix,		///< view matrix
									typename CGlRenderCallbackTypes::setup_view_model_matrix *callback,	///< callback
									void *callback_param,				///< parameter for callback
									void *user_data						///< user data (pointer to cMainVisualization, because this is a static function)
							);

	/**
	 * render diffuse objects where photons are catched
	 */
	static void callback_render_diffuse_objects(
									const GLSL::mat4 &projection_matrix,	///< projection matrix
									const GLSL::mat4 &view_matrix,		///< view matrix
									typename CGlRenderCallbackTypes::setup_view_model_matrix *callback,	///< callback
									void *callback_param,				///< parameter for callback
									void *user_data						///< user data (pointer to cMainVisualization, because this is a static function)
							);

public:

	void prepare_expandable_viewbox();


	/**
	 * this function actually renders the objects and is also calling the simulation runtime
	 */
	void render();

	void cleanup();

	~CRenderPass();
};


#endif /* CRENDERPASS_HPP_ */
