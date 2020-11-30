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

#include "CRenderPass.hpp"

#include "libgl/core/CGlBuffer.hpp"
#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlViewport.hpp"
#include "libgl/core/CGlState.hpp"
#include "libgl/CGlCubeMap.hpp"

#include "libgl/peeling/CGlDepthPeeling.hpp"

#include "libgl/draw/CGlDrawRectangle.hpp"
#include "libgl/draw/CGlDrawTexturedQuad.hpp"
#include "libgl/draw/CGlDrawTexturedArrayQuad.hpp"
#include "libgl/draw/CGlDrawIntTexturedQuad.hpp"
#include "libgl/draw/CGlDrawFlatCubeMap.hpp"
#include "libgl/draw/CGlDrawVolumeBox.hpp"
#include "libgl/draw/CGlDrawSkyBox.hpp"
#include "libgl/draw/CGlDrawWireframeBox.hpp"

#include "libgl/CGlLights.hpp"
#include "libgl/CGlMaterial.hpp"

// define GL_INTEROP to load OpenGL INTEROP support (usually set up via preprocessor variable)
#include "lbm/CLbmOpenClInterface.hpp"


#include "libgl/opencl/CGlVolumeTextureToFlat.hpp"
#include "libgl/opencl/CGlRawVolumeTextureToFlat.hpp"



#include "lib/CFlatTextureLayout.hpp"

#include "lib/CEyeBall.hpp"

#include "mainvis/CConfig.hpp"
#include "mainvis/CMarchingCubesRenderers.hpp"
#include "mainvis/CObjectRenderers.hpp"
#include "mainvis/CShaders.hpp"
#include "mainvis/CSwitches.hpp"
#include "mainvis/CTextures.hpp"
#include "mainvis/CVolumeTexturesAndRenderers.hpp"
#include "mainvis/CMatrices.hpp"

#include "libgl/CGlProjectionMatrix.hpp"
#include "libgl/CGlExpandableViewBox.hpp"

#define DEBUG_CHECKSUM_VELOCITY		0

#include "libgl/CGlRenderCallbackTypes.hpp"

#include "libgl/viewspace_refractions/CGlViewspaceRefractionsFrontBack.hpp"
#include "libgl/viewspace_refractions/CGlViewspaceRefractionsFrontBackDepthVoxels.hpp"
#include "libgl/viewspace_refractions/CGlViewspaceRefractionsFrontBackTotalReflections.hpp"
#include "libgl/viewspace_refractions/CGlViewspaceRefractionsMultiLayered.hpp"
#include "libgl/photon_mapping/CGlPhotonMappingFrontBack.hpp"
#include "libgl/photon_mapping/CGlPhotonsRenderer.hpp"
#include "libgl/photon_mapping/CGlPhotonMappingCausticMapFrontBack.hpp"
#include "libgl/photon_mapping/CGlPhotonsCreateCausticMap.hpp"

#include "CMainVisualization.hpp"


template <typename T>
class CRenderPass_Private
{
public:
	CRenderPass_Private(bool verbose)	:
		cVolumeTexturesAndRenderers(verbose)
	{
	}

	CGlCubeMap cGlCubeMap;
	CGlDrawFlatCubeMap cGlDrawFlatCubeMap;
	CGlDrawTexturedQuad cGlDrawTexturedQuad;
	CGlDrawTexturedArrayQuad cGlDrawTexturedArrayQuad;
	CGlDrawIntTexturedQuad cGlDrawIntTexturedQuad;
	CGlDrawVolumeBox cGlDrawVolumeBox;
	CGlTexture fbo_texture;
	CGlFbo test_fbo;
	CGlDrawRectangle draw_rectangle;

	// temporary texture to store the raw memory buffer
	CGlTexture fluid_fraction_raw_texture;

	// final fluid fraction texture with the standard flat texture layout
	CGlTexture fluid_fraction_flat_texture;

	/**
	 * simulation parts
	 */
	T *fluid_fraction_buffer;
	CGlRawVolumeTextureToFlat cGlRawVolumeTextureToFlat;
	CGlVolumeTextureToFlat cGlVolumeTextureToFlat;

	/**
	 * flat texture
	 */
	CFlatTextureLayout cFlatTextureLayout;

	/**
	 * depth peeling for current view
	 */
	CGlDepthPeeling cGlDepthPeeling;

	/**
	 * depth peeling for refractions
	 */
	CGlViewspaceRefractionsFrontBack cGlViewspaceRefractionsFrontBack;
	CGlViewspaceRefractionsFrontBackDepthVoxels cGlViewspaceRefractionsFrontBackDepthVoxels;
	CGlViewspaceRefractionsFrontBackTotalReflections cGlViewspaceRefractionsFrontBackTotalReflections;
	CGlViewspaceRefractionsMultiLayered cGlViewspaceRefractionsMultiLayered;

	/**
	 * Light Space Photon Mapping
	 */
	CGlPhotonMappingFrontBack cGlPhotonMappingFrontBack;
	CGlPhotonsRenderer cGlPhotonsRenderer;
	CGlPhotonMappingCausticMapFrontBack cGlPhotonMappingCausticMapFrontBack;
	CGlPhotonsCreateCausticMap cGlPhotonsCreateCausticMap;

	/**
	 * stopwatch
	 */
	CStopwatch stopwatch;

	/**
	 * other stuff
	 */
	CGlDrawSkyBox cGlDrawSkyBox;
	CGlWireframeBox cGlWireframeBox;

	CGlViewport viewport;

	GLsizei depth_peeling_width;	///< width of depth peeling texture
	GLsizei depth_peeling_height;	///< height of depth peeling texture
	int depth_peeling_layers;		///< layers for depthpeeling

	CGlLights lights_non_transparent_water;

	class CVolumeTexturesAndRenderers cVolumeTexturesAndRenderers;
};

// use typename for renderpass which has to be of the same type as CMainVisualization<T>
template <typename T>
CRenderPass<T>::CRenderPass(
					CRenderWindow &p_cRenderWindow,
					class CMainVisualization<T> &p_cMain,
					CConfig &p_cConfig,
					CLbmOpenClInterface<T> *p_cLbmOpenCl_ptr,
					bool p_verbose
			):
	verbose(p_verbose),
	cRenderWindow(p_cRenderWindow),
	cMain(p_cMain),
	cConfig(p_cConfig),
	cLbmOpenCl_ptr(p_cLbmOpenCl_ptr),


	cMarchingCubesRenderers(verbose),

	cObjectRenderers(verbose),
	cTextures(verbose),
	cShaders(verbose),

	// initialize fluid_fraction_volume_texture here because rebinding a texture to a different target
	// creates an error
	fluid_fraction_volume_texture(GL_TEXTURE_3D, GL_R32F, GL_RED, GL_FLOAT)
{
	private_class = new CRenderPass_Private<T>(verbose);
}

template <typename T>
void CRenderPass<T>::createCubeMapCallback(GLSL::mat4 &projection_matrix, GLSL::mat4 &view_matrix, void *user_data)
{
	CRenderPass *cRenderPass = (CRenderPass*)user_data;

	cRenderPass->render_scene_objects(projection_matrix, view_matrix);

	if (cRenderPass->cConfig.render_table)
		cRenderPass->render_table(projection_matrix, view_matrix);
}


/**
 * this function is called from CMain when the viewport has to be resized
 */
template <typename T>
void CRenderPass<T>::viewport_resize(GLsizei width, GLsizei height)
{
	private_class->cGlViewspaceRefractionsFrontBack.resize(width, height);
	private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.resize(width, height);
	private_class->cGlViewspaceRefractionsFrontBackTotalReflections.resize(width, height);

	private_class->cGlViewspaceRefractionsMultiLayered.resizeViewport(width, height);

	private_class->cGlPhotonsRenderer.resizeViewport(width, height);

	private_class->cGlViewspaceRefractionsMultiLayered.cGlDepthPeelingVoxels.resize(width, height, width, height);
}


/**
 * this callback function is called, when the peeling texture for photon mapping should be resized
 */
template <typename T>
void CRenderPass<T>::callback_photon_mapping_peeling_texture_resized(void *user_ptr)
{
	CRenderPass &r = *(CRenderPass*)user_ptr;
	r.private_class->cGlPhotonMappingFrontBack.resizePeelingTexture(r.cConfig.photon_mapping_peeling_texture_width, r.cConfig.photon_mapping_peeling_texture_height);
	r.private_class->cGlPhotonMappingCausticMapFrontBack.resizePeelingTexture(r.cConfig.photon_mapping_peeling_texture_width, r.cConfig.photon_mapping_peeling_texture_height);
}

/**
 * this callback function is called, when the photon texture for photon mapping should be resized
 */
template <typename T>
void CRenderPass<T>::callback_photon_mapping_photon_texture_resized(void *user_ptr)
{
	CRenderPass &r = *(CRenderPass*)user_ptr;
	r.private_class->cGlPhotonMappingFrontBack.resizePhotonTexture(r.cConfig.photon_mapping_photon_texture_width, r.cConfig.photon_mapping_photon_texture_height);
	r.private_class->cGlPhotonsRenderer.resizeIndexBuffer(r.cConfig.photon_mapping_photon_texture_width*r.cConfig.photon_mapping_photon_texture_height);

	r.private_class->cGlPhotonMappingCausticMapFrontBack.resizePhotonTexture(r.cConfig.photon_mapping_photon_texture_width, r.cConfig.photon_mapping_photon_texture_height);
	r.private_class->cGlPhotonsCreateCausticMap.resizeIndexBuffer(r.cConfig.photon_mapping_photon_texture_width*r.cConfig.photon_mapping_photon_texture_height);
	r.private_class->cGlPhotonsCreateCausticMap.resizeCausticMap(r.cConfig.photon_mapping_photon_texture_width, r.cConfig.photon_mapping_photon_texture_height);
}

/**
 * initialize everything
 */
template <typename T>
void CRenderPass<T>::init()
{
	private_class->fluid_fraction_buffer = NULL;

	central_object_scale_matrix = GLSL::scale(1.4, 1.4, 1.4);
//		central_object_scale_matrix = GLSL::scale(2, 2, 2);

	/**
	 * OTHER STUFF
	 */

	if (verbose)	std::cout << "RL: cube map, draw flat cube map, draw textured quad, draw int textured quad, draw volume box" << std::endl;

//		cGlCubeMap.resize(512, 512, 512);
	private_class->cGlCubeMap.resize(1024, 1024, 1024);
	CError_AppendReturn(private_class->cGlCubeMap);
	CError_AppendReturn(private_class->cGlDrawFlatCubeMap);

	CError_AppendReturn(private_class->cGlDrawTexturedQuad);
	CError_AppendReturn(private_class->cGlDrawIntTexturedQuad);
	CError_AppendReturn(private_class->cGlDrawVolumeBox);

	if (verbose)	std::cout << "RL: fbo_texture, test_fbo, draw_rectangle" << std::endl;

	private_class->fbo_texture.setTextureParameters(GL_TEXTURE_2D);
	private_class->fbo_texture.bind();
	private_class->fbo_texture.resize(512,512);
	private_class->fbo_texture.unbind();
	CError_AppendReturn(private_class->fbo_texture);

	private_class->test_fbo.bind();
	private_class->test_fbo.bindTexture(private_class->fbo_texture);
	private_class->test_fbo.unbind();
	CError_AppendReturn(private_class->test_fbo);
	CError_AppendReturn(private_class->draw_rectangle);

	/**
	 * OBJECT RENDERERS
	 */
	if (verbose)	std::cout << "Object Renderers..." << std::endl;
	cObjectRenderers.setup();
	CError_AppendReturn(cObjectRenderers);


	/**
	 * TEXTURES
	 */
	if (verbose)	std::cout << "Textures..." << std::endl;
	cTextures.setup();
	CError_AppendReturn(cTextures);


	/**
	 * SHADERS
	 */
	if (verbose)	std::cout << "Shader..." << std::endl;
	CError_AppendReturn(cShaders);


	/**
	 * WATER MATERIAL AND LIGHTS
	 */
	private_class->lights_non_transparent_water.light0_enabled = true;
	private_class->lights_non_transparent_water.light0_ambient_color3 = cConfig.light0_ambient_color3;
	private_class->lights_non_transparent_water.light0_diffuse_color3 = cConfig.light0_diffuse_color3;
	private_class->lights_non_transparent_water.light0_specular_color3 = cConfig.light0_specular_color3;


	/**
	 * VOLUME TEXTURES AND RENDERERS
	 */

	if (verbose)	std::cout << "Volume Textures..." << std::endl;
	if (cMain.domain_cells.max() <= 64)
	{
		private_class->cVolumeTexturesAndRenderers.setup(true, cMain.domain_cells);
	}
	else
	{
		std::cout << "INFO: disabling test volume: 'domain_cells.max() > 64' to save memory" << std::endl;
		private_class->cVolumeTexturesAndRenderers.setup(cMain.cLbmOpenCl_ptr == NULL, cMain.domain_cells);
	}
	CError_AppendReturn(private_class->cVolumeTexturesAndRenderers);


	/**
	 * SKYBOX
	 */
	if (verbose)	std::cout << "SkyBox..." << std::endl;
#if 0
	private_class->CGlDrawSkyBox.initWithTextures(
						"data/textures/skybox/img_7888.jpg",
						"data/textures/skybox/img_8093.jpg",
						"data/textures/skybox/img_6295.jpg",
						"data/textures/skybox/dsc00087.jpg",
						"data/textures/skybox/img_5264.jpg",
						"data/textures/skybox/dsc00040.jpg"
				);
#else
	private_class->cGlDrawSkyBox.initWithTextures(
						"data/textures/weilheimer_huette/left.jpg",
						"data/textures/weilheimer_huette/right.jpg",
						"data/textures/weilheimer_huette/top.jpg",
						"data/textures/weilheimer_huette/bottom.jpg",
						"data/textures/weilheimer_huette/back.jpg",
						"data/textures/weilheimer_huette/front.jpg"
				);
#endif
	CError_AppendReturn(private_class->cGlDrawSkyBox);


	/**
	 * setup fixed matrices
	 */
	cMatrices.ortho_matrix = GLSL::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);



	/**
	 * MARCHING CUBES EXTRACTION
	 */
	if (verbose)	std::cout << "RL: MC Renderers" << std::endl;
	cMarchingCubesRenderers.reset(cMain.domain_cells);
	CError_AppendReturn(cMarchingCubesRenderers);


	/**
	 * 3D volume and 2D flat texture conversions
	 */
	// temporary texture to store the raw memory buffer
	private_class->fluid_fraction_raw_texture.setTextureParameters(GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT);

	// final fluid fraction texture with the standard flat texture layout
	private_class->fluid_fraction_flat_texture.setTextureParameters(GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT);


	/**
	 * OPENCL STUFF
	 */
#if LBM_OPENCL_FS_DEBUG_CHECKSUM_VELOCITY
	int lbm_sim_count = 0;
#endif

	if (cLbmOpenCl_ptr != NULL)
	{
		private_class->fluid_fraction_buffer = new T[cLbmOpenCl_ptr->params.domain_cells.elements()];
		/*
		 * we have to use GL_RGBA for initialization cz. opencl does not support GL_RED memory objects!
		 */

		fluid_fraction_volume_texture.bind();
		fluid_fraction_volume_texture.resize(	cLbmOpenCl_ptr->params.domain_cells[0],
												cLbmOpenCl_ptr->params.domain_cells[1],
													cLbmOpenCl_ptr->params.domain_cells[2]
									);
		fluid_fraction_volume_texture.unbind();
	}


	/*
	 * prepare conversion to flat texture layout for speedup using marching cubes and vertex arrays
	 */
	private_class->cFlatTextureLayout.init(cMain.domain_cells);

	if (	(private_class->cFlatTextureLayout.ft_z_width & 1) != 0	||
			(private_class->cFlatTextureLayout.ft_z_height & 1) != 0
	)
	{
		std::cerr << "width and height of flat texture for fluid fraction has to be a multiple of 2 to use GL INTEROPS!!!" << std::endl;
		exit(1);
	}

	private_class->fluid_fraction_flat_texture.bind();
	private_class->fluid_fraction_flat_texture.setNearestNeighborInterpolation();
	private_class->fluid_fraction_flat_texture.resize(
			private_class->cFlatTextureLayout.ft_z_width >> 1,
			private_class->cFlatTextureLayout.ft_z_height >> 1	);
	private_class->fluid_fraction_flat_texture.unbind();
	CGlErrorCheck();

	if (cLbmOpenCl_ptr != NULL)
	{
		private_class->fluid_fraction_raw_texture.bind();
		private_class->fluid_fraction_raw_texture.setNearestNeighborInterpolation();
		private_class->fluid_fraction_raw_texture.resize(
				private_class->cFlatTextureLayout.ft_z_width >> 1,
				private_class->cFlatTextureLayout.ft_z_height >> 1	);
		private_class->fluid_fraction_raw_texture.unbind();
#ifdef LBM_OPENCL_GL_INTEROP
		cLbmOpenCl_ptr->initFluidFractionMemObject2D(private_class->fluid_fraction_raw_texture);
#endif
		CGlErrorCheck();
	}


	std::ostringstream raw_to_flat_program_defines;
	raw_to_flat_program_defines << "#version 150" << std::endl;
	raw_to_flat_program_defines << "#define TEXTURE_WIDTH	(" << (private_class->cFlatTextureLayout.ft_z_width >> 1) << ")" << std::endl;
	raw_to_flat_program_defines << "#define TEXTURE_HEIGHT	(" << (private_class->cFlatTextureLayout.ft_z_height >> 1) << ")" << std::endl;

	raw_to_flat_program_defines << "#define DOMAIN_CELLS_X	(" << cMain.domain_cells[0] << ")" << std::endl;
	raw_to_flat_program_defines << "#define DOMAIN_CELLS_Y	(" << cMain.domain_cells[1] << ")" << std::endl;
	raw_to_flat_program_defines << "#define DOMAIN_CELLS_Z	(" << cMain.domain_cells[2] << ")" << std::endl;

	raw_to_flat_program_defines << "#define FT_Z_MOD		(" << private_class->cFlatTextureLayout.ft_z_mod << ")" << std::endl;


	/*
	 * GL PROGRAM: RAW TO FLAT
	 */
	private_class->cGlRawVolumeTextureToFlat.setup(private_class->fluid_fraction_flat_texture, private_class->cFlatTextureLayout);
	CError_AppendReturn(private_class->cGlRawVolumeTextureToFlat);

	private_class->cGlVolumeTextureToFlat.setup(private_class->fluid_fraction_flat_texture, private_class->cFlatTextureLayout);
	CError_AppendReturn(private_class->cGlVolumeTextureToFlat);


	/**
	 * DEPTH PEELING PARAMETERS
	 */
	private_class->depth_peeling_width = 64;
	private_class->depth_peeling_height = 64;
	private_class->depth_peeling_width = 256;
	private_class->depth_peeling_height = 256;
	private_class->depth_peeling_layers = 4;


	/**
	 * DEPTH PEELING OF MESH
	 */
	private_class->cGlDepthPeeling.setup(private_class->depth_peeling_width, private_class->depth_peeling_height, private_class->depth_peeling_layers);
	CError_AppendReturn(private_class->cGlDepthPeeling);


	/**
	 * DEPTH PEELING IN VIEWSPACE FOR REFRACTIONS
	 */
	CError_AppendReturn(private_class->cGlViewspaceRefractionsFrontBack);
	CError_AppendReturn(private_class->cGlViewspaceRefractionsFrontBackDepthVoxels);
	CError_AppendReturn(private_class->cGlViewspaceRefractionsFrontBackTotalReflections);

	private_class->cGlViewspaceRefractionsMultiLayered.setup(private_class->depth_peeling_layers);
	CError_AppendReturn(private_class->cGlViewspaceRefractionsMultiLayered);


	/**
	 * Light Space Photon Mapping
	 */
	private_class->cGlPhotonMappingFrontBack.setup();
	CError_AppendReturn(private_class->cGlPhotonMappingFrontBack);

	private_class->cGlPhotonsRenderer.setup(private_class->cGlPhotonMappingFrontBack.photon_position_texture);
	CError_AppendReturn(private_class->cGlPhotonsRenderer);

	private_class->cGlPhotonMappingCausticMapFrontBack.setup();
	CError_AppendReturn(private_class->cGlPhotonMappingCausticMapFrontBack);

	private_class->cGlPhotonsCreateCausticMap.setup(private_class->cGlPhotonMappingCausticMapFrontBack.photon_position_texture);
	CError_AppendReturn(private_class->cGlPhotonsCreateCausticMap);

	cConfig.set_callback(&cConfig.photon_mapping_peeling_texture_width, &callback_photon_mapping_peeling_texture_resized, this);
	cConfig.set_callback(&cConfig.photon_mapping_peeling_texture_height, &callback_photon_mapping_peeling_texture_resized, this);
	callback_photon_mapping_peeling_texture_resized(this);

	cConfig.set_callback(&cConfig.photon_mapping_photon_texture_width, &callback_photon_mapping_photon_texture_resized, this);
	cConfig.set_callback(&cConfig.photon_mapping_photon_texture_height, &callback_photon_mapping_photon_texture_resized, this);
	callback_photon_mapping_photon_texture_resized(this);


	private_class->stopwatch.start();

	viewport_resize(cRenderWindow.window_width, cRenderWindow.window_height);
}


/**
 * render some textured quads
 */
template <typename T>
void CRenderPass<T>::render_animated_quads(	GLSL::mat4 &projection_matrix,
							GLSL::mat4 &view_matrix
						)
{
	CGlStateDisable state_cull_face(GL_CULL_FACE);

	GLSL::mat4 p_model_matrix;
	GLSL::mat4 p_normal_matrix;
	GLSL::mat4 p_pvm_matrix;

	cShaders.cTexturize.use();

	// front
	p_model_matrix = GLSL::rotate((float)cMain.ticks*30.0f+0.0f, 0.0f, 1.0f, 0.0f);
	p_model_matrix *= GLSL::translate(0.0f, 0.0f, -13.0f);
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;

	cShaders.cTexturize.pvm_matrix_uniform.set(p_pvm_matrix);
	cTextures.texture1.bind();
	cObjectRenderers.cGlDrawQuad.render();
	cTextures.texture1.unbind();

	// left
	p_model_matrix = GLSL::rotate((float)cMain.ticks*30.0f+90.0f, 0.0f, 1.0f, 0.0f);
	p_model_matrix *= GLSL::translate(0.0f, 0.0f, -12.5f);
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;

	cShaders.cTexturize.pvm_matrix_uniform.set(p_pvm_matrix);
	cTextures.texture2.bind();
	cObjectRenderers.cGlDrawQuad.render();
	cTextures.texture2.unbind();

	// back
	p_model_matrix = GLSL::rotate((float)cMain.ticks*30.0f+180.0f, 0.0f, 1.0f, 0.0f);
	p_model_matrix *= GLSL::translate(0.0f, 0.0f, -13.0f);
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;

	cShaders.cTexturize.pvm_matrix_uniform.set(p_pvm_matrix);
	cTextures.texture3.bind();
	cObjectRenderers.cGlDrawQuad.render();
	cTextures.texture3.unbind();

	// right
	p_model_matrix = GLSL::rotate((float)cMain.ticks*30.0f+270.0f, 0.0f, 1.0f, 0.0f);
	p_model_matrix *= GLSL::translate(0.0f, 0.0f, -12.5f);
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;

	cShaders.cTexturize.pvm_matrix_uniform.set(p_pvm_matrix);
	cTextures.texture4.bind();
	cObjectRenderers.cGlDrawQuad.render();
	cTextures.texture4.unbind();

	// top
	p_model_matrix = GLSL::rotate((float)cMain.ticks*30.0f+180.0f, 1.0f, 0.0f, 0.0f);
	p_model_matrix *= GLSL::translate(0.0f, 0.0f, -11.0f);
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;

	cShaders.cTexturize.pvm_matrix_uniform.set(p_pvm_matrix);
	cTextures.texture3.bind();
	cObjectRenderers.cGlDrawQuad.render();
	cTextures.texture3.unbind();

	// bottom
	p_model_matrix = GLSL::rotate((float)cMain.ticks*30.0f, 1.0f, 0.0f, 0.0f);
	p_model_matrix *= GLSL::translate(0.0f, 0.0f, -10.0f);
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;

	cShaders.cTexturize.pvm_matrix_uniform.set(p_pvm_matrix);
	cTextures.texture4.bind();
	cObjectRenderers.cGlDrawQuad.render();
	cTextures.texture4.unbind();

	cShaders.cTexturize.disable();

	CGlErrorCheck();
}

/**
 * render fancy rotating balls
 */
template <typename T>
void CRenderPass<T>::render_animated_balls(	GLSL::mat4 &projection_matrix,
							GLSL::mat4 &view_matrix
						)
{
	CGlProgramUse program(cShaders.cBlinn);
	cShaders.cBlinn.texture0_enabled.set1b(false);

	static CGlMaterial m;
	m.ambient_color3 = GLSL::vec3(0.1);
	m.diffuse_color3 = GLSL::vec3(0.1, 0.1, 1.0);
	m.specular_color3 = GLSL::vec3(1);
	m.specular_exponent = 20;

	cShaders.cBlinn.setupUniforms(m, cGlLights, view_matrix*cGlLights.light0_world_pos4);

	GLSL::mat4 p_model_matrix;
	GLSL::mat4 p_view_model_matrix;
	GLSL::mat4 p_pvm_matrix;
	GLSL::mat3 p_normal_matrix3;
#define MINIMUM_DISTANCE 6.5f
	// BALL 0
	p_model_matrix = GLSL::rotate((float)cMain.ticks*30.0f+50.0f, 1.0f, 1.0f, 0.0f);
	p_model_matrix *= GLSL::translate(-0.1f-MINIMUM_DISTANCE, 0.2f+MINIMUM_DISTANCE, 0.1f+MINIMUM_DISTANCE);
	p_view_model_matrix = view_matrix * p_model_matrix;
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;
	p_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(p_view_model_matrix));

	cShaders.cBlinn.light0_diffuse_color3_uniform.set(GLSL::vec3(1,0,0));
	cShaders.cBlinn.setupUniformsMatrices(p_pvm_matrix, p_view_model_matrix, p_normal_matrix3);
	cObjectRenderers.cGlDrawSphere.renderWithoutProgram();

	// BALL 1
	p_model_matrix = GLSL::rotate((float)cMain.ticks*50.0f-50.0f, 0.4f, 1.0f, 0.4f);
	p_model_matrix *= GLSL::translate(-0.1f-MINIMUM_DISTANCE, -0.1f-MINIMUM_DISTANCE, 0.2f+MINIMUM_DISTANCE);
	p_view_model_matrix = view_matrix * p_model_matrix;
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;
	p_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(p_view_model_matrix));

	cShaders.cBlinn.light0_diffuse_color3_uniform.set(GLSL::vec3(0,1,0));
	cShaders.cBlinn.setupUniformsMatrices(p_pvm_matrix, p_view_model_matrix, p_normal_matrix3);
	cObjectRenderers.cGlDrawSphere.renderWithoutProgram();

	// BALL 2
	p_model_matrix = GLSL::rotate((float)cMain.ticks*70.0f+50.0f, 1.1f, 0.4f, -1.0f);
	p_model_matrix *= GLSL::translate(-0.2f-MINIMUM_DISTANCE, 0.3f+MINIMUM_DISTANCE, -0.1f-MINIMUM_DISTANCE);
	p_view_model_matrix = view_matrix * p_model_matrix;
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;
	p_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(p_view_model_matrix));

	cShaders.cBlinn.light0_diffuse_color3_uniform.set(GLSL::vec3(0,0,1));
	cShaders.cBlinn.vertex_color.set(GLSL::vec4(0,0,1,0));
	cShaders.cBlinn.setupUniformsMatrices(p_pvm_matrix, p_view_model_matrix, p_normal_matrix3);
	cObjectRenderers.cGlDrawSphere.renderWithoutProgram();
}


/**
 * render fancy rotating meshes
 */
template <typename T>
void CRenderPass<T>::render_animated_meshes(	GLSL::mat4 &projection_matrix,
								GLSL::mat4 &view_matrix
							)
{
	CGlProgramUse program(cShaders.cBlinn);
	cShaders.cBlinn.texture0_enabled.set1b(false);

	GLSL::mat4 p_model_matrix;
	GLSL::mat4 p_view_model_matrix;
	GLSL::mat4 p_pvm_matrix;
	GLSL::mat3 p_normal_matrix3;

	// BUNNY 0
	p_model_matrix = GLSL::rotate((float)cMain.ticks*30.0f+70.0f, 1.1f, 0.4f, -1.0f);
	p_model_matrix *= GLSL::translate(-0.1f-MINIMUM_DISTANCE, 0.1f+MINIMUM_DISTANCE, 0.2f+MINIMUM_DISTANCE);
	p_model_matrix *= GLSL::scale(1.5f, 1.5f, 1.5f);
	p_view_model_matrix = view_matrix * p_model_matrix;
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;
	p_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(p_view_model_matrix));

	cShaders.cBlinn.light0_diffuse_color3_uniform.set(GLSL::vec3(1,1,0));
	cShaders.cBlinn.setupUniformsMatrices(p_pvm_matrix, p_view_model_matrix, p_normal_matrix3);
	cObjectRenderers.cGlRenderObjFileBunny.renderWithoutProgram();

	// BUNNY 1
	p_model_matrix = GLSL::rotate((float)cMain.ticks*20.0f+170.0f, 1.1f, 3.4f, -1.0f);
	p_model_matrix *= GLSL::translate(-0.3f-MINIMUM_DISTANCE, 0.1f+MINIMUM_DISTANCE, 0.2f+MINIMUM_DISTANCE);
	p_model_matrix *= GLSL::scale(1.8f, 1.8f, 1.8f);
	p_view_model_matrix = view_matrix * p_model_matrix;
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;
	p_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(p_view_model_matrix));

	cShaders.cBlinn.light0_diffuse_color3_uniform.set(GLSL::vec3(1,0,1));
	cShaders.cBlinn.setupUniformsMatrices(p_pvm_matrix, p_view_model_matrix, p_normal_matrix3);
	cObjectRenderers.cGlRenderObjFileBunny.renderWithoutProgram();

	// BUNNY 2
	p_model_matrix = GLSL::rotate((float)cMain.ticks*60.0f+70.0f, 1.1f, -0.4f, -1.1f);
	p_model_matrix *= GLSL::translate(0.1f+MINIMUM_DISTANCE, 0.0f+MINIMUM_DISTANCE, -0.2f-MINIMUM_DISTANCE);
	p_model_matrix *= GLSL::scale(1.7f, 1.7f, 1.7f);
	p_view_model_matrix = view_matrix * p_model_matrix;
	p_pvm_matrix = projection_matrix * view_matrix * p_model_matrix;
	p_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(p_view_model_matrix));

	cShaders.cBlinn.light0_diffuse_color3_uniform.set(GLSL::vec3(0,1,1));
	cShaders.cBlinn.setupUniformsMatrices(p_pvm_matrix, p_view_model_matrix, p_normal_matrix3);
	cObjectRenderers.cGlRenderObjFileBunny.renderWithoutProgram();
}


template <typename T>
void CRenderPass<T>::render_table(	GLSL::mat4 &projection_matrix,
					GLSL::mat4 &view_matrix
)
{
	if (cConfig.photon_mapping_shadows)
	{
		if (cConfig.photon_mapping_front_back)
		{
			render_table_shadow_mapping(	projection_matrix, view_matrix,
					private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontBackFaces.front_depth_texture,
											cGlExpandableViewBox.lsb_view_matrix,
											cGlExpandableViewBox.lsb_projection_matrix
										);
		}
		else
		{
			render_table_shadow_and_caustic_mapping(
											projection_matrix, view_matrix,
											(cConfig.photon_mapping_front_back_faces_from_polygons ? private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontBackFaces.front_depth_texture : private_class->cGlPhotonMappingCausticMapFrontBack.cGlVolumeRendererInterpolatedFrontBackFaces.front_depth_texture),
											private_class->cGlPhotonsCreateCausticMap.photon_caustic_map_texture,
											private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontDiffuseFaces.front_depth_texture,
											cGlExpandableViewBox.lsb_view_matrix,
											cGlExpandableViewBox.lsb_projection_matrix
										);
		}
	}
	else
	{
		render_table_no_lighting(projection_matrix, view_matrix);
	}
}



/**
 * render table without lighting
 */
template <typename T>
void CRenderPass<T>::render_table_no_lighting(	GLSL::mat4 &projection_matrix,
								GLSL::mat4 &view_matrix
				)
{
	GLSL::mat4 p_view_model_matrix = view_matrix * diffuse_receivers_model_matrix;
	GLSL::mat4 p_pvm_matrix = projection_matrix * view_matrix * diffuse_receivers_model_matrix;
	GLSL::mat3 p_view_model_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(p_view_model_matrix));
//	GLSL::mat3 p_view_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(view_matrix));


	cObjectRenderers.cGlRenderObjFileTable.render(cGlLights, p_pvm_matrix, p_view_model_matrix, p_view_model_normal_matrix3);
}


/**
 * render table with shadow mapping
 */
template <typename T>
void CRenderPass<T>::render_table_shadow_mapping(
					GLSL::mat4 &projection_matrix,
					GLSL::mat4 &view_matrix,
					CGlTexture &depth_texture,
					GLSL::mat4 &lsb_view_matrix,
					GLSL::mat4 &lsb_projection_matrix
				)
{
	GLSL::mat4 p_view_model_matrix = view_matrix * diffuse_receivers_model_matrix;
	GLSL::mat4 p_pvm_matrix = projection_matrix * p_view_model_matrix;
	GLSL::mat3 p_view_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(p_view_model_matrix));
//	GLSL::mat3 p_view_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(view_matrix));

	GLSL::mat4 shadow_map_matrix = lsb_projection_matrix * GLSL::inverse(view_matrix) * GLSL::scale((float)cRenderWindow.window_width, (float)cRenderWindow.window_height, 1.0);
	shadow_map_matrix = GLSL::scale(0.5, 0.5, 0.5)*GLSL::translate(1, 1, 1)*lsb_projection_matrix*lsb_view_matrix*GLSL::inverse(view_matrix);

	cObjectRenderers.cGlRenderObjFileTable.renderWithShadowMap(
			cGlLights, p_pvm_matrix, p_view_model_matrix, p_view_model_normal_matrix3, view_matrix,
			depth_texture, shadow_map_matrix
			);
}



/**
 * render table with shadow and caustic mapping
 */
template <typename T>
void CRenderPass<T>::render_table_shadow_and_caustic_mapping(
					GLSL::mat4 &projection_matrix,
					GLSL::mat4 &view_matrix,
					CGlTexture &depth_texture,
					CGlTexture &caustic_map_texture,
					CGlTexture &caustic_map_depth_texture,
					GLSL::mat4 &lsb_view_matrix,
					GLSL::mat4 &lsb_projection_matrix
				)
{
	GLSL::mat4 p_view_model_matrix = view_matrix * diffuse_receivers_model_matrix;
	GLSL::mat4 p_pvm_matrix = projection_matrix * p_view_model_matrix;
	GLSL::mat3 p_view_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(p_view_model_matrix));

	GLSL::mat4 shadow_map_matrix = lsb_projection_matrix *
				GLSL::inverse(view_matrix) *
				GLSL::scale((float)cRenderWindow.window_width, (float)cRenderWindow.window_height, 1.0);

	shadow_map_matrix = GLSL::scale(0.5, 0.5, 0.5)*GLSL::translate(1, 1, 1)*lsb_projection_matrix*lsb_view_matrix*GLSL::inverse(view_matrix);

	cObjectRenderers.cGlRenderObjFileTable.renderWithShadowAndCausticMap(
			cGlLights, p_pvm_matrix, p_view_model_matrix, p_view_model_normal_matrix3, view_matrix,
			depth_texture,
			caustic_map_texture,
			caustic_map_depth_texture,
			shadow_map_matrix
			);
}




template <typename T>
void CRenderPass<T>::render_scene_objects(GLSL::mat4 &projection_matrix, GLSL::mat4 &view_matrix)
{
	/**
	 * initialize lighting for standard cShaders
	 */

	if (cConfig.render_scene)
	{

		GLSL::mat4 p_view_model_matrix = view_matrix;
		GLSL::mat3 p_view_model_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(p_view_model_matrix));
		GLSL::mat4 p_pvm_matrix = projection_matrix * p_view_model_matrix;

		if (!cConfig.photon_mapping_shadows)
		{
			cObjectRenderers.cGlRenderScene.render(	cGlLights,
													p_pvm_matrix,
													p_view_model_matrix,
													p_view_model_normal_matrix3
												);
		}
		else
		{
			GLSL::mat4 shadow_map_matrix = GLSL::scale(0.5, 0.5, 0.5)*GLSL::translate(1, 1, 1)*cGlExpandableViewBox.lsb_projection_matrix*cGlExpandableViewBox.lsb_view_matrix*GLSL::inverse(view_matrix);

			cObjectRenderers.cGlRenderScene.renderWithShadowMap(
													cGlLights,
													p_pvm_matrix,
													p_view_model_matrix,
													p_view_model_normal_matrix3,
													view_matrix,
													private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontDiffuseFaces.front_depth_texture,
													shadow_map_matrix
												);

		}
	}

	if (cConfig.render_skybox)
	{
		// skip model matrix because we don't move the environment!
		GLSL::mat4 p_pvm_matrix = projection_matrix * GLSL::mat4(GLSL::mat3(view_matrix));
		private_class->cGlDrawSkyBox.renderWithProgram(p_pvm_matrix);
	}

	if (cConfig.render_animated_balls)
		render_animated_balls(projection_matrix, view_matrix);

	if (cConfig.render_animated_meshes)
		render_animated_meshes(projection_matrix, view_matrix);

	if (cConfig.render_animated_quads)
		render_animated_quads(projection_matrix, view_matrix);
}


template <typename T>
void CRenderPass<T>::render_fbo_test()
{
	cMatrices.p_model_matrix = cMatrices.model_matrix*GLSL::scale(2.0f, 2.0f, 2.0f);;
	cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.view_matrix * cMatrices.p_model_matrix;

	// render box to fbo
	private_class->test_fbo.bind();
	private_class->viewport.saveState();

		glViewport(0, 0, private_class->fbo_texture.width, private_class->fbo_texture.height);

		// clear background
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);

		// draw box
		glCullFace(GL_FRONT);
		private_class->cGlDrawVolumeBox.render(cMatrices.p_pvm_matrix);
		glCullFace(GL_BACK);

		private_class->viewport.restoreState();
		private_class->test_fbo.unbind();

	cMatrices.p_view_matrix = GLSL::translate(0.6f, 0.6f, 0.6f);
	cMatrices.p_model_matrix = GLSL::scale(0.25f, 0.25f, 0.25f);
	cMatrices.p_pvm_matrix = cMatrices.ortho_matrix * cMatrices.p_view_matrix * cMatrices.p_model_matrix;
	private_class->cGlDrawTexturedQuad.render(cMatrices.p_pvm_matrix, private_class->fbo_texture);
}


template <typename T>
void CRenderPass<T>::render_marching_cubes_with_refractions()
{

	cMatrices.p_model_matrix = cMatrices.model_matrix;
	cMatrices.p_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(cMatrices.p_model_matrix));
	cMatrices.p_view_model_matrix = cMatrices.view_matrix*cMatrices.p_model_matrix;
	cMatrices.p_view_model_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_view_model_matrix));
	cMatrices.p_view_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(cMatrices.view_matrix));

	cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.p_view_model_matrix;
	cMatrices.p_transposed_view_matrix3 = GLSL::mat3(GLSL::transpose(cMatrices.view_matrix));

	static CGlMaterial m;
	m.ambient_color3 = cConfig.water_ambient_color3;
	m.diffuse_color3 = cConfig.water_diffuse_color3;
	m.specular_color3 = cConfig.water_specular_color3;
	m.specular_exponent = cConfig.water_specular_exponent;

	if (cConfig.render_marching_cubes_vertex_array_front_back_refractions)
	{
		/**
		 * render MCs using vertex arrays with refractions using only front and back textures
		 */
		private_class->cGlViewspaceRefractionsFrontBack.create(
								cMatrices.projection_matrix,
								cMatrices.view_matrix,
								&callback_render_refractive_objects,
								this);

		private_class->cGlViewspaceRefractionsFrontBack.cGlProgram.use();
		private_class->cGlViewspaceRefractionsFrontBack.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);
		private_class->cGlViewspaceRefractionsFrontBack.cGlProgram.disable();

		private_class->cGlViewspaceRefractionsFrontBack.render(
										private_class->cGlCubeMap.texture_cube_map,
										GLSL::inverse(GLSL::mat3(cMatrices.view_matrix)),
										cMatrices.projection_matrix,
										cConfig.refraction_index,
										cConfig.water_reflectance_at_normal_incidence,
										cConfig.render_marching_cubes_vertex_array_front_back_step_size
									);

		if (cConfig.render_view_space_refractions_peeling_textures)
		{
			CGlViewport viewport;
			viewport.saveState();

			CGlStateDisable asdf(GL_DEPTH_TEST);

			int size_div = 4;
			int width = private_class->cGlViewspaceRefractionsFrontBack.cGlPeelingFrontBackFaces.width / size_div;
			int height = private_class->cGlViewspaceRefractionsFrontBack.cGlPeelingFrontBackFaces.height / size_div;

			viewport.set(0, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBack.cGlPeelingFrontBackFaces.front_depth_texture);
			viewport.set(0, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBack.cGlPeelingFrontBackFaces.front_normal_texture);
			viewport.set(0, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBack.cGlPeelingFrontBackFaces.front_texture);

			int x = width+1;
			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBack.cGlPeelingFrontBackFaces.back_depth_texture);
			viewport.set(x, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBack.cGlPeelingFrontBackFaces.back_normal_texture);
			viewport.set(x, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBack.cGlPeelingFrontBackFaces.back_texture);

			viewport.restoreState();
		}
	}

	/**
	 * render MCs using vertex arrays with refractions using only front and back textures
	 */
	if (cConfig.render_marching_cubes_vertex_array_front_back_depth_voxels_refractions)
	{
		private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.create(
								cMatrices.projection_matrix,
								cMatrices.view_matrix,
								&callback_render_refractive_objects,
								this);

		private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlProgram.use();
		private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);
		private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlProgram.disable();

		private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.render(	private_class->cGlCubeMap.texture_cube_map,
										GLSL::inverse(GLSL::mat3(cMatrices.view_matrix)),
										//GLSL::inverseTranspose(projection_matrix),
										cMatrices.projection_matrix,
										cConfig.refraction_index
									);


		if (cConfig.render_view_space_refractions_peeling_textures)
		{
			CGlViewport viewport;
			viewport.saveState();

			CGlStateDisable asdf(GL_DEPTH_TEST);

			int size_div = 4;
			int width = private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlPeelingFrontBackFaces.width / size_div;
			int height = private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlPeelingFrontBackFaces.height / size_div;

			viewport.set(0, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlPeelingFrontBackFaces.front_depth_texture);
			viewport.set(0, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlPeelingFrontBackFaces.front_normal_texture);
			viewport.set(0, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlPeelingFrontBackFaces.front_texture);

			int x = width+1;
			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlPeelingFrontBackFaces.back_depth_texture);
			viewport.set(x, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlPeelingFrontBackFaces.back_normal_texture);
			viewport.set(x, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackDepthVoxels.cGlPeelingFrontBackFaces.back_texture);

			viewport.restoreState();
		}
	}

	/**
	 * render MCs using vertex arrays with refractions using only front and back textures and total reflections
	 */
	if (cConfig.render_marching_cubes_vertex_array_front_back_total_reflections)
	{
		private_class->cGlViewspaceRefractionsFrontBackTotalReflections.create(
								cMatrices.projection_matrix,
								cMatrices.view_matrix,
								&callback_render_refractive_objects,
								this);

		private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlProgram.use();
		private_class->cGlViewspaceRefractionsFrontBackTotalReflections.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);
		private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlProgram.disable();

		private_class->cGlViewspaceRefractionsFrontBackTotalReflections.render(
				private_class->cGlCubeMap.texture_cube_map,
										GLSL::inverse(GLSL::mat3(cMatrices.view_matrix)),
										//GLSL::inverseTranspose(projection_matrix),
										cMatrices.projection_matrix,
										cConfig.refraction_index
									);

		if (cConfig.render_view_space_refractions_peeling_textures)
		{
			CGlViewport viewport;
			viewport.saveState();

			CGlStateDisable asdf(GL_DEPTH_TEST);

			int size_div = 4;
			int width = private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlPeelingFrontBackFaces.width / size_div;
			int height = private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlPeelingFrontBackFaces.height / size_div;

			viewport.set(0, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlPeelingFrontBackFaces.front_depth_texture);
			viewport.set(0, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlPeelingFrontBackFaces.front_normal_texture);
			viewport.set(0, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlPeelingFrontBackFaces.front_texture);

			int x = width+1;
			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlPeelingFrontBackFaces.back_depth_texture);
			viewport.set(x, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlPeelingFrontBackFaces.back_normal_texture);
			viewport.set(x, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsFrontBackTotalReflections.cGlPeelingFrontBackFaces.back_texture);

			viewport.restoreState();
		}
	}

	/**
	 * render MCs using vertex arrays with refractions using multiple layers
	 */
	if (cConfig.render_marching_cubes_vertex_array_multi_layered_refractions)
	{

		/**
		 * create depth peeling voxelization textures
		 */
		private_class->cGlViewspaceRefractionsMultiLayered.create(
								cMatrices.projection_matrix,
								cMatrices.view_matrix,
								&callback_render_refractive_objects,
								this);

		private_class->cGlViewspaceRefractionsMultiLayered.cGlProgram.use();
		private_class->cGlViewspaceRefractionsMultiLayered.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);
		private_class->cGlViewspaceRefractionsMultiLayered.cGlProgram.disable();

		private_class->cGlViewspaceRefractionsMultiLayered.render(
				private_class->cGlCubeMap.texture_cube_map,
										GLSL::inverse(GLSL::mat3(cMatrices.view_matrix)),
										//GLSL::inverseTranspose(projection_matrix),
										cMatrices.projection_matrix,
										cConfig.refraction_index
									);


		if (cConfig.render_view_space_refractions_peeling_textures)
		{
			CGlViewport viewport;
			viewport.saveState();

			CGlStateDisable asdf(GL_DEPTH_TEST);

			int size_div = 4;
			int width = private_class->cGlViewspaceRefractionsMultiLayered.cGlDepthPeelingVoxels.peeling_pass_width / size_div;
			int height = private_class->cGlViewspaceRefractionsMultiLayered.cGlDepthPeelingVoxels.peeling_pass_height / size_div;

			int x = 0;
			for (int i = 0; i < 4; i++)
			{
				viewport.set(x, 0, width, height);
				private_class->cGlDrawTexturedArrayQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsMultiLayered.cGlDepthPeelingVoxels.peeling_depth_texture, i);
				viewport.set(x, (height+1), width, height);
				private_class->cGlDrawTexturedArrayQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsMultiLayered.cGlDepthPeelingVoxels.peeling_normal_texture, i);
				viewport.set(x, (height+1)*2, width, height);
				private_class->cGlDrawTexturedArrayQuad.render(cMatrices.ortho_matrix, private_class->cGlViewspaceRefractionsMultiLayered.cGlDepthPeelingVoxels.peeling_voxel_texture, i);
				x += width+1;
			}

			viewport.restoreState();
		}
	}
}


template <typename T>
void CRenderPass<T>::render_volume_casting(CGlTexture *volume_texture)
{
	float max = CMath::max((float)volume_texture->width, (float)volume_texture->height);
	max = CMath::max(max, (float)volume_texture->depth);

	cMatrices.p_model_matrix = cMatrices.model_matrix*central_object_scale_matrix*GLSL::scale((float)volume_texture->width/max, (float)volume_texture->height/max, (float)volume_texture->depth/max);
	cMatrices.p_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(cMatrices.p_model_matrix));
	cMatrices.p_view_model_matrix = cMatrices.view_matrix*cMatrices.p_model_matrix;
	cMatrices.p_view_model_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_view_model_matrix));
	cMatrices.p_view_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(cMatrices.view_matrix));

	cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.p_view_model_matrix;
	cMatrices.p_transposed_view_matrix3 = GLSL::mat3(GLSL::transpose(cMatrices.view_matrix));


	volume_texture->setLinearInterpolation();
	volume_texture->unbind();

	if (!cConfig.refraction_active)
	{
		static CGlMaterial m;
		m.ambient_color3 = GLSL::vec3(0.1);
		m.diffuse_color3 = GLSL::vec3(0.1, 0.1, 1.0);
		m.specular_color3 = GLSL::vec3(1);
		m.specular_exponent = 20;


		if (cConfig.volume_simple)
		{
			private_class->cVolumeTexturesAndRenderers.volume_renderer_simple.cGlProgram.use();
			private_class->cVolumeTexturesAndRenderers.volume_renderer_simple.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);
			private_class->cVolumeTexturesAndRenderers.volume_renderer_simple.cGlProgram.disable();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_simple.render(
					cMatrices.p_pvm_matrix,
					cMatrices.view_matrix,
					cMatrices.p_model_matrix,
					cMatrices.p_view_model_matrix,
					cMatrices.p_view_model_normal_matrix3,
					*volume_texture,
					cConfig.volume_gradient_distance,
					cConfig.volume_step_size
				);
		}

		if (cConfig.volume_interpolated)
		{
			private_class->cVolumeTexturesAndRenderers.volume_renderer_interpolated.cGlProgram.use();
			private_class->cVolumeTexturesAndRenderers.volume_renderer_interpolated.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);
			private_class->cVolumeTexturesAndRenderers.volume_renderer_interpolated.cGlProgram.disable();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_interpolated.render(
					cMatrices.p_pvm_matrix,
					cMatrices.view_matrix,
					cMatrices.p_model_matrix,
					cMatrices.p_view_model_matrix,
					cMatrices.p_view_model_normal_matrix3,
					*volume_texture,
					cConfig.volume_gradient_distance,
					cConfig.volume_step_size
				);
		}

		if (cConfig.volume_cube_steps)
		{
			private_class->cVolumeTexturesAndRenderers.volume_renderer_cube_steps.cGlProgram.use();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_cube_steps.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);

			private_class->cVolumeTexturesAndRenderers.volume_renderer_cube_steps.cGlProgram.disable();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_cube_steps.render(
					cMatrices.p_pvm_matrix,
					cMatrices.view_matrix,
					cMatrices.p_model_matrix,
					cMatrices.p_view_model_matrix,
					cMatrices.p_view_model_normal_matrix3,
					*volume_texture,
					cConfig.volume_gradient_distance,
					cConfig.volume_step_size
				);
		}

		if (cConfig.volume_marching_cubes)
		{
			// prepare the marching cube indices and normal data using another class
			CGlMarchingCubesPreprocessing &preprocessing = cMarchingCubesRenderers.cGlMarchingCubesGeometryShader;
			preprocessing.prepare(*volume_texture);

			private_class->cVolumeTexturesAndRenderers.volume_renderer_marching_cubes.cGlProgram.use();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_marching_cubes.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);

			private_class->cVolumeTexturesAndRenderers.volume_renderer_marching_cubes.cGlProgram.disable();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_marching_cubes.render(
					cMatrices.p_pvm_matrix,
					cMatrices.view_matrix,
					cMatrices.p_model_matrix,
					cMatrices.p_view_model_matrix,
					cMatrices.p_view_model_normal_matrix3,
					preprocessing,
					cConfig.volume_gradient_distance,
					cConfig.volume_step_size
				);
		}
	}
	else
	{

		CGlMaterial m;
		m.ambient_color3 = cConfig.water_ambient_color3;
		m.diffuse_color3 = cConfig.water_diffuse_color3;
		m.specular_color3 = cConfig.water_specular_color3;
		m.specular_exponent = cConfig.water_specular_exponent;

		if (cConfig.volume_simple)
		{
			private_class->cVolumeTexturesAndRenderers.volume_renderer_simple_refraction.cGlProgram.use();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_simple_refraction.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);

			private_class->cVolumeTexturesAndRenderers.volume_renderer_simple_refraction.cGlProgram.disable();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_simple_refraction.render(
					cMatrices.p_pvm_matrix,
					cMatrices.view_matrix,
					cMatrices.p_model_matrix,
					cMatrices.p_model_normal_matrix3,
					cMatrices.p_view_model_matrix,
					cMatrices.p_view_model_normal_matrix3,

					*volume_texture,
					private_class->cGlCubeMap.texture_cube_map,
					cConfig.volume_gradient_distance,
					cConfig.refraction_index,
					cConfig.volume_step_size,
					cConfig.water_reflectance_at_normal_incidence
				);
		}

		if (cConfig.volume_interpolated)
		{
			private_class->cVolumeTexturesAndRenderers.volume_renderer_interpolated_refraction.cGlProgram.use();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_interpolated_refraction.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);

			private_class->cVolumeTexturesAndRenderers.volume_renderer_interpolated_refraction.cGlProgram.disable();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_interpolated_refraction.render(
					cMatrices.p_pvm_matrix,
					cMatrices.view_matrix,
					cMatrices.p_model_matrix,
					cMatrices.p_model_normal_matrix3,
					cMatrices.p_view_model_matrix,
					cMatrices.p_view_model_normal_matrix3,

					*volume_texture,
					private_class->cGlCubeMap.texture_cube_map,
					cConfig.volume_gradient_distance,
					cConfig.refraction_index,
					cConfig.volume_step_size,
					cConfig.water_reflectance_at_normal_incidence
			);
		}

		if (cConfig.volume_cube_steps)
		{
			private_class->cVolumeTexturesAndRenderers.volume_renderer_cube_steps_refraction.cGlProgram.use();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_cube_steps_refraction.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);

			private_class->cVolumeTexturesAndRenderers.volume_renderer_cube_steps_refraction.cGlProgram.disable();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_cube_steps_refraction.render(
					cMatrices.p_pvm_matrix,
					cMatrices.view_matrix,
					cMatrices.p_model_matrix,
					cMatrices.p_model_normal_matrix3,
					cMatrices.p_view_model_matrix,
					cMatrices.p_view_model_normal_matrix3,

					*volume_texture,
					private_class->cGlCubeMap.texture_cube_map,
					cConfig.volume_gradient_distance,
					cConfig.refraction_index,
					cConfig.volume_step_size,
					cConfig.water_reflectance_at_normal_incidence
			);
		}

		if (cConfig.volume_marching_cubes)
		{
			CGlMarchingCubesPreprocessing &preprocessing = cMarchingCubesRenderers.cGlMarchingCubesGeometryShader;
			preprocessing.prepare(*volume_texture);

			private_class->cVolumeTexturesAndRenderers.volume_renderer_marching_cubes_refraction.cGlProgram.use();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_marching_cubes_refraction.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);

			private_class->cVolumeTexturesAndRenderers.volume_renderer_marching_cubes_refraction.cGlProgram.disable();

			private_class->cVolumeTexturesAndRenderers.volume_renderer_marching_cubes_refraction.render(
					cMatrices.p_pvm_matrix,
					cMatrices.view_matrix,
					cMatrices.p_model_matrix,
					cMatrices.p_model_normal_matrix3,
					cMatrices.p_view_model_matrix,
					cMatrices.p_view_model_normal_matrix3,

					preprocessing,
					private_class->cGlCubeMap.texture_cube_map,
					cConfig.volume_gradient_distance,
					cConfig.refraction_index,
					cConfig.volume_step_size,
					cConfig.water_reflectance_at_normal_incidence
			);
		}
	}
}


/**
 * convert a flat texture created by copying from opencl to a flat texture with adjacent slices
 *
 * then, extract the marching cubes
 *
 * this function is usually called by every function which relies on the extracted marching cubes vertices
 */
template <typename T>
void CRenderPass<T>::render_flat_volume_texture_to_flat_texture_and_extract(CGlTexture *volume_texture)
{
	if (cConfig.render_marching_cubes_vertex_array_disable_mc_generation)
		return;

	if (cLbmOpenCl_ptr != NULL && !cConfig.lbm_simulation_disable_visualization)
		private_class->cGlRawVolumeTextureToFlat.convert(private_class->fluid_fraction_raw_texture, private_class->fluid_fraction_flat_texture);
	else
		private_class->cGlVolumeTextureToFlat.convert(*volume_texture, private_class->fluid_fraction_flat_texture);

	/*
	 * the flat texture is now prepared and we can start with computing the marching cubes indices
	 */
	cMarchingCubesRenderers.cGlMarchingCubesVertexArrayRGBA.prepare(private_class->fluid_fraction_flat_texture);
}


template <typename T>
void CRenderPass<T>::render_marching_cubes_solid(CGlTexture *volume_texture)
{
	static CGlMaterial m;
	m.ambient_color3 = GLSL::vec3(0.1);
	m.diffuse_color3 = GLSL::vec3(0.1, 0.1, 1.0);
	m.specular_color3 = GLSL::vec3(1);
	m.specular_exponent = 20;

	if (cConfig.render_marching_cubes_geometry_shader)
	{
		// VERSION USING GEOMETRY SHADER

		cMarchingCubesRenderers.cGlMarchingCubesGeometryShader.prepare(*volume_texture);

		cMatrices.p_model_matrix = cMatrices.model_matrix*central_object_scale_matrix;

		float max = CMath::max((float)volume_texture->width, (float)volume_texture->height);
		max = CMath::max(max, (float)volume_texture->depth);

		cMatrices.p_model_matrix *= GLSL::scale(2.0f/max, 2.0f/max, 2.0f/max);

		cMatrices.p_model_matrix *= GLSL::translate(	0.5f - (float)volume_texture->width*0.5f,
											0.5f - (float)volume_texture->height*0.5f,
											0.5f - (float)volume_texture->depth*0.5f);

		cMatrices.p_view_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(cMatrices.view_matrix));
		cMatrices.p_view_model_matrix = cMatrices.view_matrix * cMatrices.p_model_matrix;
		cMatrices.p_view_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(cMatrices.p_view_model_matrix));

		cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.p_view_model_matrix;

		cMarchingCubesRenderers.cGlMarchingCubesGeometryShader.cGlProgramRender.use();
		cMarchingCubesRenderers.cGlMarchingCubesGeometryShader.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);
		cMarchingCubesRenderers.cGlMarchingCubesGeometryShader.cGlProgramRender.disable();

		cMarchingCubesRenderers.cGlMarchingCubesGeometryShader.render(	cMatrices.p_pvm_matrix, GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_pvm_matrix)),
								cMatrices.p_view_model_matrix, GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_view_model_matrix))
							);
	}

	/**
	 * render MC using vertex array
	 */
	if (cConfig.render_marching_cubes_vertex_array)
	{
		if (!cMarchingCubesRenderers.cGlMarchingCubesVertexArray.valid)
			return;

		cMarchingCubesRenderers.cGlMarchingCubesVertexArray.prepare(*volume_texture);

		cMatrices.p_model_matrix = cMatrices.model_matrix*central_object_scale_matrix;

		float max = CMath::max((float)volume_texture->width, (float)volume_texture->height);
		max = CMath::max(max, (float)volume_texture->depth);

		cMatrices.p_model_matrix *= GLSL::scale(2.0f/max, 2.0f/max, 2.0f/max);

		cMatrices.p_model_matrix *= GLSL::translate(	0.5f - (float)volume_texture->width*0.5f,
											0.5f - (float)volume_texture->height*0.5f,
											0.5f - (float)volume_texture->depth*0.5f);

		cMatrices.p_view_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(cMatrices.view_matrix));
		cMatrices.p_view_model_matrix = cMatrices.view_matrix * cMatrices.p_model_matrix;
		cMatrices.p_view_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(cMatrices.p_view_model_matrix));

		cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.p_view_model_matrix;

		cMarchingCubesRenderers.cGlMarchingCubesVertexArray.cGlProgramRender.use();
		cMarchingCubesRenderers.cGlMarchingCubesVertexArray.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);
		cMarchingCubesRenderers.cGlMarchingCubesVertexArray.cGlProgramRender.disable();

		cMarchingCubesRenderers.cGlMarchingCubesVertexArray.render(
											cMatrices.p_pvm_matrix, GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_pvm_matrix)),
											cMatrices.p_view_model_matrix, GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_view_model_matrix))
										);
	}

	/**
	 * render MCs using flat texture and vertex array
	 */
	if (cConfig.render_marching_cubes_vertex_array_rgba)
	{
		cMatrices.p_model_matrix = cMatrices.model_matrix*central_object_scale_matrix;

		cMatrices.p_view_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(cMatrices.view_matrix));
		cMatrices.p_view_model_matrix = cMatrices.view_matrix * cMatrices.p_model_matrix;
		cMatrices.p_view_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(cMatrices.p_view_model_matrix));

		cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.p_view_model_matrix;

		cMarchingCubesRenderers.cGlMarchingCubesVertexArrayRGBA.cGlProgramRender.use();
		cMarchingCubesRenderers.cGlMarchingCubesVertexArrayRGBA.setupUniforms(m, cGlLights, cMatrices.view_matrix*cGlLights.light0_world_pos4);
		cMarchingCubesRenderers.cGlMarchingCubesVertexArrayRGBA.cGlProgramRender.disable();

		cMarchingCubesRenderers.cGlMarchingCubesVertexArrayRGBA.render(
												cMatrices.p_pvm_matrix, GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_pvm_matrix)),
												cMatrices.p_view_model_matrix, GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_view_model_matrix))
										);
	}

}

template <typename T>
void CRenderPass<T>::render_photonmapping_front_back_volume_callback(
					CGlVolumeRendererInterpolatedFrontBackFaces &renderer,
					void* user_data,
					const GLSL::mat4 &projection_matrix,
					const GLSL::mat4 &view_matrix
)
{
	CRenderPass &r = *(CRenderPass*)user_data;

	float max = CMath::max((float)r.volume_texture->width, (float)r.volume_texture->height);
	max = CMath::max(max, (float)r.volume_texture->depth);
	r.cMatrices.p_model_matrix = r.cMatrices.model_matrix*r.central_object_scale_matrix*GLSL::scale((float)r.volume_texture->width/max, (float)r.volume_texture->height/max, (float)r.volume_texture->depth/max);
	r.cMatrices.p_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(r.cMatrices.p_model_matrix));
	r.cMatrices.p_view_model_matrix = view_matrix*r.cMatrices.p_model_matrix;
	r.cMatrices.p_view_model_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(r.cMatrices.p_view_model_matrix));
	r.cMatrices.p_view_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(view_matrix));

	r.cMatrices.p_pvm_matrix = projection_matrix * r.cMatrices.p_view_model_matrix;
	r.cMatrices.p_transposed_view_matrix3 = GLSL::mat3(GLSL::transpose(view_matrix));

	renderer.render(
			r.cMatrices.p_pvm_matrix,
			view_matrix,
			r.cMatrices.p_model_matrix,
			r.cMatrices.p_view_model_matrix,
			r.cMatrices.p_view_model_normal_matrix3,
			*r.volume_texture,
			r.cConfig.volume_gradient_distance,
			r.cConfig.volume_step_size
		);
}

/**
 * PHOTON MAPPING preparation
 */
template <typename T>
void CRenderPass<T>::render_photonmapping_front_back_faces_prepare(CGlExpandableViewBox &cGlExpandableViewBox)
{
	if (cConfig.photon_mapping_front_back)
	{
		private_class->cGlPhotonMappingFrontBack.create(	cGlExpandableViewBox.lsb_projection_matrix,
											cGlExpandableViewBox.lsb_view_matrix,
											cConfig.refraction_index,
											&callback_render_refractive_objects,
											this,
											&callback_render_diffuse_objects,
											this
										);
	}

	if (cConfig.photon_mapping_front_back_caustic_map)
	{
		private_class->cGlPhotonMappingCausticMapFrontBack.create(	cGlExpandableViewBox.lsb_projection_matrix,
													cGlExpandableViewBox.lsb_view_matrix,
													cConfig.refraction_index,
													&callback_render_refractive_objects,
													this,
													&callback_render_diffuse_objects,
													this,
													cConfig.photon_mapping_front_back_caustic_map_step_size,
													cConfig.photon_mapping_front_back_faces_from_polygons,
													render_photonmapping_front_back_volume_callback,
													this
										);

		// area of light space
		float light_space_near_plane_area =	(cGlExpandableViewBox.lsb_frustum_right - cGlExpandableViewBox.lsb_frustum_left)
											* (cGlExpandableViewBox.lsb_frustum_top - cGlExpandableViewBox.lsb_frustum_bottom);

		float splat_size =	cConfig.photon_mapping_light0_splat_size *
							CMath::sqrt(
								((float)(private_class->cGlPhotonMappingCausticMapFrontBack.photon_position_texture.width*private_class->cGlPhotonMappingCausticMapFrontBack.photon_position_texture.height))	/
								((cGlExpandableViewBox.lsb_frustum_right-cGlExpandableViewBox.lsb_frustum_left)*(cGlExpandableViewBox.lsb_frustum_top-cGlExpandableViewBox.lsb_frustum_bottom))
							);

//			std::cout << splat_size << std::endl;

		private_class->cGlPhotonsCreateCausticMap.setupSplatEnergyAndSize(
												cConfig.photon_mapping_light0_energy,
												light_space_near_plane_area,
												private_class->cGlPhotonMappingCausticMapFrontBack.photon_position_texture.width,
												private_class->cGlPhotonMappingCausticMapFrontBack.photon_position_texture.height,
												splat_size,
												1.0/(cConfig.photon_mapping_light0_splat_size*cConfig.photon_mapping_light0_splat_size)
											);

		private_class->cGlPhotonsCreateCausticMap.renderCausticMap(
										cMatrices.projection_matrix,
										cMatrices.view_matrix,
										GLSL::inverseTranspose(GLSL::mat3(cMatrices.view_matrix)),
										GLSL::vec3(cConfig.water_ambient_color3),
										private_class->cGlPhotonMappingCausticMapFrontBack.photon_position_texture
									);

		if (cConfig.render_photon_mapping_peeling_textures)
		{
			CGlViewport viewport;
			viewport.saveState();

			int size_div = 8;
			int width = private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontBackFaces.width / size_div;
			int height = private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontBackFaces.height / size_div;

			bool c = cConfig.photon_mapping_front_back_faces_from_polygons;
/*
			CGlVolumeRendererInterpolatedFrontBackFaces &a = cGlPhotonMappingCausticMapFrontBack.cGlVolumeRendererInterpolatedFrontBackFaces;
			CGlTexture asdftex;
			asdftex.bind();
			asdftex.resize(1024, 1024);
			asdftex.unbind();

			a.front_back_fbo.bind();
			a.front_back_fbo.bindTexture(asdftex, 0, 0);
			a.front_back_fbo.unbind();

			a.front_back_fbo.bind();
			viewport.setSize(1024, 1024);
			glClearColor(1, 0.5, 1, 0.5);
			glClear(GL_COLOR_BUFFER_BIT);
			a.front_back_fbo.unbind();

			viewport.set(0, 0, width, height);
			cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, asdftex);
*/
#if 1
			int x = 0;
			viewport.set(x, 0, width, height);
			if (c)	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontBackFaces.front_depth_texture);
			else	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlVolumeRendererInterpolatedFrontBackFaces.front_depth_texture);
			viewport.set(x, (height+1), width, height);
			if (c)	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontBackFaces.front_normal_texture);
			else	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlVolumeRendererInterpolatedFrontBackFaces.front_normal_texture);
			viewport.set(x, (height+1)*2, width, height);
			if (c)	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontBackFaces.front_texture);
			else	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlVolumeRendererInterpolatedFrontBackFaces.front_texture);

			x += width+1;
			viewport.set(x, 0, width, height);
			if (c)	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontBackFaces.back_depth_texture);
			else	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlVolumeRendererInterpolatedFrontBackFaces.back_depth_texture);
			viewport.set(x, (height+1), width, height);
			if (c)	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontBackFaces.back_normal_texture);
			else	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlVolumeRendererInterpolatedFrontBackFaces.back_normal_texture);
			viewport.set(x, (height+1)*2, width, height);
			if (c)	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontBackFaces.back_texture);
			else	private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlVolumeRendererInterpolatedFrontBackFaces.back_texture);

			x += width+1;
			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontDiffuseFaces.front_depth_texture);
			viewport.set(x, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontDiffuseFaces.front_normal_texture);
			viewport.set(x, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.cGlPeelingFrontDiffuseFaces.front_texture);

			x += width+1;

			size_div = 8;
			width = private_class->cGlPhotonMappingCausticMapFrontBack.photon_position_texture.width / size_div;
			height = private_class->cGlPhotonMappingCausticMapFrontBack.photon_position_texture.height / size_div;

			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingCausticMapFrontBack.photon_position_texture);


			x += width+1;

			size_div = 4;
			width = private_class->cGlPhotonsCreateCausticMap.photon_caustic_map_texture.width / size_div;
			height = private_class->cGlPhotonsCreateCausticMap.photon_caustic_map_texture.height / size_div;

			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonsCreateCausticMap.photon_caustic_map_texture);
#endif
			viewport.restoreState();
		}
	}
}



/*
 * PHOTON MAPPING using only front and back faces
 */
template <typename T>
void CRenderPass<T>::render_photonmapping_front_back_faces(CGlExpandableViewBox &cGlExpandableViewBox)
{
	CGlProjectionMatrix p;
	p.setup(cMatrices.projection_matrix);
	p.unproject();

	if (cConfig.photon_mapping_front_back)
	{
		// area of light spaces near plane
		float light_space_near_plane_area =	(cGlExpandableViewBox.lsb_frustum_right - cGlExpandableViewBox.lsb_frustum_left)
											* (cGlExpandableViewBox.lsb_frustum_top - cGlExpandableViewBox.lsb_frustum_bottom);

		float splat_size = cConfig.photon_mapping_light0_splat_size *
				((float)cRenderWindow.window_width)/(p.frustum_right-p.frustum_left);

		private_class->cGlPhotonsRenderer.setupSplatEnergyAndSplatSize(
												cConfig.photon_mapping_light0_energy,
												light_space_near_plane_area,
												private_class->cGlPhotonMappingFrontBack.photon_position_texture.width,
												private_class->cGlPhotonMappingFrontBack.photon_position_texture.height,
												splat_size,
												4.0/(cConfig.photon_mapping_light0_splat_size*cConfig.photon_mapping_light0_splat_size)
											);

		private_class->cGlPhotonsRenderer.render(		cMatrices.projection_matrix,
										cMatrices.view_matrix,
										GLSL::inverseTranspose(GLSL::mat3(cMatrices.view_matrix)),
										GLSL::inverse(cGlExpandableViewBox.lsb_view_matrix),
										GLSL::mat3(GLSL::transpose(cGlExpandableViewBox.lsb_view_matrix)),
										private_class->cGlPhotonMappingFrontBack.photon_position_texture,
										private_class->cGlPhotonMappingFrontBack.photon_normal_attenuation_texture
									);


		if (cConfig.render_photon_mapping_peeling_textures)
		{
			CGlViewport viewport;
			viewport.saveState();

			int size_div = 8;
			int width = private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontBackFaces.width / size_div;
			int height = private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontBackFaces.height / size_div;

			int x = 0;
			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontBackFaces.front_depth_texture);
			viewport.set(x, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontBackFaces.front_normal_texture);
			viewport.set(x, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontBackFaces.front_texture);

			x += width+1;
			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontBackFaces.back_depth_texture);
			viewport.set(x, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontBackFaces.back_normal_texture);
			viewport.set(x, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontBackFaces.back_texture);

			x += width+1;
			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontDiffuseFaces.front_depth_texture);
			viewport.set(x, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontDiffuseFaces.front_normal_texture);
			viewport.set(x, (height+1)*2, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.cGlPeelingFrontDiffuseFaces.front_texture);

			x += width+1;

			size_div = 8;
			width = private_class->cGlPhotonMappingFrontBack.photon_position_texture.width / size_div;
			height = private_class->cGlPhotonMappingFrontBack.photon_position_texture.height / size_div;

			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.photon_position_texture);
			viewport.set(x, (height+1), width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonMappingFrontBack.photon_normal_attenuation_texture);


			x += width+1;

			size_div = 4;
			width = private_class->cGlPhotonsRenderer.photon_blend_texture.width / size_div;
			height = private_class->cGlPhotonsRenderer.photon_blend_texture.height / size_div;

			viewport.set(x, 0, width, height);
			private_class->cGlDrawTexturedQuad.render(cMatrices.ortho_matrix, private_class->cGlPhotonsRenderer.photon_blend_texture);

			viewport.restoreState();
		}
	}
}


/**
 * render refractive objects only (for photon mapping, caustics, refractions, etc.)
 */
template <typename T>
void CRenderPass<T>::callback_render_refractive_objects(
								const GLSL::mat4 &projection_matrix,	///< projection matrix
								const GLSL::mat4 &view_matrix,		///< view matrix
								typename CGlRenderCallbackTypes::setup_view_model_matrix *callback,	///< callback
								void *callback_param,				///< parameter for callback
								void *user_data						///< user data (pointer to cMainVisualization, because this is a static function)
						)
{
	class CRenderPass &c = *(class CRenderPass*)user_data;

	if (c.cConfig.render_mesh_bunny || c.cConfig.render_mesh_sphere)
	{
		GLSL::mat4 view_model_matrix = view_matrix*c.cMatrices.model_matrix;
		GLSL::mat3 view_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(view_model_matrix));
		GLSL::mat4 pvm_matrix = projection_matrix*view_model_matrix;

		callback(callback_param, pvm_matrix, view_model_matrix, view_model_normal_matrix3);

		if (c.cConfig.render_mesh_bunny)
			c.cObjectRenderers.cGlRenderObjFileBunny.renderWithoutProgram();
		else if (c.cConfig.render_mesh_sphere)
			c.cObjectRenderers.cGlRenderObjFileSphere.renderWithoutProgram();
	}
	else
	{
		GLSL::mat4 view_model_matrix = view_matrix*c.cMatrices.model_matrix*c.central_object_scale_matrix;
		GLSL::mat3 view_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(view_model_matrix));
		GLSL::mat4 pvm_matrix = projection_matrix*view_model_matrix;

		callback(callback_param, pvm_matrix, view_model_matrix, view_model_normal_matrix3);
		c.cMarchingCubesRenderers.cGlMarchingCubesVertexArrayRGBA.renderWithoutProgram();
	}
}



/**
 * render diffuse objects where photons are catched
 */
template <typename T>
void CRenderPass<T>::callback_render_diffuse_objects(
								const GLSL::mat4 &projection_matrix,	///< projection matrix
								const GLSL::mat4 &view_matrix,		///< view matrix
								typename CGlRenderCallbackTypes::setup_view_model_matrix *callback,	///< callback
								void *callback_param,				///< parameter for callback
								void *user_data						///< user data (pointer to cMainVisualization, because this is a static function)
						)
{
	class CRenderPass &c = *(class CRenderPass*)user_data;

	GLSL::mat4 view_model_matrix = view_matrix*c.diffuse_receivers_model_matrix;
	GLSL::mat3 view_model_normal_matrix3 = GLSL::inverseTranspose(GLSL::mat3(view_model_matrix));
	GLSL::mat4 pvm_matrix = projection_matrix*view_model_matrix;

	callback(callback_param, pvm_matrix, view_model_matrix, view_model_normal_matrix3);

	// render table without shaders
	c.cObjectRenderers.cGlRenderObjFileTable.renderWithoutProgram();
}


template <typename T>
void CRenderPass<T>::prepare_expandable_viewbox()
{
	// compute projection and view matrix from light source to center (0,0,0)

	GLSL::mat4 lsb_view_matrix;
	if (GLSL::length2(GLSL::abs(GLSL::normalize(GLSL::vec3(cGlLights.light0_world_pos4)))-GLSL::vec3(0,1,0)) > 0.01)
	{
		lsb_view_matrix = GLSL::lookAt(
									GLSL::vec3(cGlLights.light0_world_pos4),
									GLSL::vec3(0,0,0),
									GLSL::vec3(0,1,0)
		);
	}
	else
	{
		lsb_view_matrix = GLSL::lookAt(
									GLSL::vec3(cGlLights.light0_world_pos4),
									GLSL::vec3(0,0,0),
									GLSL::vec3(1,0,0)
		);
	}

	GLSL::mat4 lsb_projection_matrix = GLSL::frustum(-0.1,0.1,-0.1,0.1,0.5,20);

	cGlExpandableViewBox.setup(lsb_projection_matrix, lsb_view_matrix);

	GLSL::mat4 pm = cMatrices.model_matrix*central_object_scale_matrix;

	// expand obstacle
	cGlExpandableViewBox.expandWithPoint(pm*GLSL::vec3(-1,-1,-1));
	cGlExpandableViewBox.expandWithPoint(pm*GLSL::vec3(1,-1,-1));
	cGlExpandableViewBox.expandWithPoint(pm*GLSL::vec3(1,1,-1));
	cGlExpandableViewBox.expandWithPoint(pm*GLSL::vec3(-1,1,-1));
	cGlExpandableViewBox.expandWithPoint(pm*GLSL::vec3(-1,-1,1));
	cGlExpandableViewBox.expandWithPoint(pm*GLSL::vec3(1,-1,1));
	cGlExpandableViewBox.expandWithPoint(pm*GLSL::vec3(1,1,1));
	cGlExpandableViewBox.expandWithPoint(pm*GLSL::vec3(-1,1,1));

	// expand with diffuse receivers (table)

	cGlExpandableViewBox.expandWithPoint(diffuse_receivers_model_matrix*GLSL::vec3(-1,-1,-1));
	cGlExpandableViewBox.expandWithPoint(diffuse_receivers_model_matrix*GLSL::vec3(1,-1,-1));
	cGlExpandableViewBox.expandWithPoint(diffuse_receivers_model_matrix*GLSL::vec3(1,1,-1));
	cGlExpandableViewBox.expandWithPoint(diffuse_receivers_model_matrix*GLSL::vec3(-1,1,-1));
	cGlExpandableViewBox.expandWithPoint(diffuse_receivers_model_matrix*GLSL::vec3(-1,-1,1));
	cGlExpandableViewBox.expandWithPoint(diffuse_receivers_model_matrix*GLSL::vec3(1,-1,1));
	cGlExpandableViewBox.expandWithPoint(diffuse_receivers_model_matrix*GLSL::vec3(1,1,1));
	cGlExpandableViewBox.expandWithPoint(diffuse_receivers_model_matrix*GLSL::vec3(-1,1,1));


	cGlExpandableViewBox.computeMatrices();
}



/**
 * this function actually renders the objects and is also calling the simulation runtime
 */
template <typename T>
void CRenderPass<T>::render()
{
	/********************************************************
	 * COMMUNICATION (ONLY HERE AND NOWHERE ELSE!) PART WITH OPENCL
	 ********************************************************/

	volume_texture = &(private_class->cVolumeTexturesAndRenderers.test_volume_texture);

	if (!cConfig.lbm_simulation_disable_visualization)
	{
		if (	cConfig.volume_simple								||
				cConfig.volume_interpolated							||
				cConfig.volume_cube_steps							||
				cConfig.volume_marching_cubes						||

				cConfig.render_marching_cubes_geometry_shader		||
				cConfig.render_marching_cubes_vertex_array			||

				cConfig.volume_dummy
		)
		{
			if (cLbmOpenCl_ptr != NULL)
			{
				if (cConfig.lbm_simulation_copy_fluid_fraction_to_visualization)
				{
					cLbmOpenCl_ptr->storeFraction(private_class->fluid_fraction_buffer);
					fluid_fraction_volume_texture.bind();
					fluid_fraction_volume_texture.setData(private_class->fluid_fraction_buffer);
					fluid_fraction_volume_texture.unbind();
					CGlErrorCheck();
				}
				CGlErrorCheck();

				volume_texture = &fluid_fraction_volume_texture;
			}
		}
		else
		{
			if (cLbmOpenCl_ptr != NULL)
				volume_texture = &fluid_fraction_volume_texture;
		}

		if (	cConfig.render_marching_cubes_vertex_array_rgba									||
				cConfig.render_marching_cubes_vertex_array_front_back_refractions				||
				cConfig.render_marching_cubes_vertex_array_front_back_depth_voxels_refractions	||
				cConfig.render_marching_cubes_vertex_array_multi_layered_refractions			||
				cConfig.render_marching_cubes_vertex_array_front_back_total_reflections			||
				cConfig.photon_mapping_front_back												||
				(cConfig.photon_mapping_front_back_caustic_map && cConfig.photon_mapping_front_back_faces_from_polygons)								||
				cConfig.render_marching_cubes_dummy
		)
		{
			if (	cConfig.lbm_simulation_copy_fluid_fraction_to_visualization &&
					!cConfig.render_marching_cubes_vertex_array_disable_mc_generation)
			{
				// load raw fluid fraction
				if (cLbmOpenCl_ptr != NULL)
				{
#ifdef LBM_OPENCL_GL_INTEROP
					cLbmOpenCl_ptr->loadFluidFractionToRawFlatTexture();
#else
					cLbmOpenCl_ptr->storeFraction(private_class->fluid_fraction_buffer);
					private_class->fluid_fraction_raw_texture.bind();
					private_class->fluid_fraction_raw_texture.setData(private_class->fluid_fraction_buffer);
					private_class->fluid_fraction_raw_texture.unbind();
#endif
				}
			}
		}
	}


	/********************************************************
	 * setup matrices for refractive objects and diffuse receiver objects
	 ********************************************************/
	refraction_object_model_matrix = cMatrices.model_matrix;


	/********************************************************
	 * SETUP LIGHT SPACE BOX
	 ********************************************************/
	if (cConfig.photon_mapping_front_back || cConfig.photon_mapping_front_back_caustic_map || cConfig.render_light_space_box)
	{
		prepare_expandable_viewbox();
	}

	/**
	 * SPECIAL GL_INTEROPS VERSION FOR MARCHING CUBES
	 *
	 * the fluid fraction memory object is copied to a 2d texture
	 * (writing to a 3d texture is still not supported with current OpenCL drivers)
	 *
	 * because this is not an equivilant to the usual flat texture, we convert it
	 */
	if (	cConfig.render_marching_cubes_vertex_array_rgba								||
			cConfig.render_marching_cubes_vertex_array_front_back_refractions			||
			cConfig.render_marching_cubes_vertex_array_front_back_depth_voxels_refractions	||
			cConfig.render_marching_cubes_vertex_array_multi_layered_refractions		||
			cConfig.render_marching_cubes_vertex_array_front_back_total_reflections		||
			cConfig.photon_mapping_front_back											||
			(cConfig.photon_mapping_front_back_caustic_map && cConfig.photon_mapping_front_back_faces_from_polygons)		||
			cConfig.render_marching_cubes_dummy
	)
	{
		render_flat_volume_texture_to_flat_texture_and_extract(volume_texture);
	}


	/********************************************************
	 * prepare photonmap
	 ********************************************************/
	if (cConfig.photon_mapping_front_back || cConfig.photon_mapping_front_back_caustic_map)
	{
		render_photonmapping_front_back_faces_prepare(cGlExpandableViewBox);
	}


	/********************************************************
	 * render diffuse receiver (table)
	 ********************************************************/
	if (cConfig.render_table)
	{
		render_table(cMatrices.projection_matrix, cMatrices.view_matrix);
	}

	/********************************************************
	 * render photons
	 ********************************************************/
	if (cConfig.photon_mapping_front_back || cConfig.photon_mapping_front_back_caustic_map)
	{
		render_photonmapping_front_back_faces(cGlExpandableViewBox);
	}

	/********************************************************
	 * create cubemap
	 ********************************************************/
	if (cConfig.create_cube_map)
	{
		GLSL::vec3 translation(cMatrices.model_matrix[3][0], cMatrices.model_matrix[3][1], cMatrices.model_matrix[3][2]);
		private_class->cGlCubeMap.create(createCubeMapCallback, -translation, this, 1, 40);
	}

	/********************************************************
	 * render scene
	 ********************************************************/
	render_scene_objects(cMatrices.projection_matrix, cMatrices.view_matrix);



	/**
	 * RENDER VOLUME WITH VOLUME CASTING
	 */
	if (	cConfig.volume_simple			||
			cConfig.volume_interpolated		||
			cConfig.volume_cube_steps		||
			cConfig.volume_marching_cubes
		)
	{
		render_volume_casting(volume_texture);
	}

	/**
	 * MC renderers without refractions
	 */
	if (	cConfig.render_marching_cubes_geometry_shader	||
			cConfig.render_marching_cubes_vertex_array		||
			cConfig.render_marching_cubes_vertex_array_rgba
			)
	{
		render_marching_cubes_solid(volume_texture);
	}

	/**
	 * MC renderers with refractions
	 */
	if (	cConfig.render_marching_cubes_vertex_array_rgba								||
			cConfig.render_marching_cubes_vertex_array_front_back_refractions			||
			cConfig.render_marching_cubes_vertex_array_front_back_depth_voxels_refractions	||
			cConfig.render_marching_cubes_vertex_array_multi_layered_refractions		||
			cConfig.render_marching_cubes_vertex_array_front_back_total_reflections
	)
	{
		render_marching_cubes_with_refractions();
	}



	/**************************
	 * RENDER LIGHT SPACE BOX
	 **************************/
	if (cConfig.render_light_space_box)
	{
		cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.view_matrix;

		CGlProgramUse program(cShaders.cColor);
		cShaders.cColor.pvm_matrix.set(cMatrices.p_pvm_matrix);
		cShaders.cColor.frag_color.set(GLSL::vec4(0,1,0,0));

		cGlExpandableViewBox.emitBoxVertices();
	}



	/**************************
	 * draw bounding box
	 **************************/
	if (cConfig.render_bounding_box)
	{
		float max = CMath::max((float)volume_texture->width, (float)volume_texture->height);
		max = CMath::max(max, (float)volume_texture->depth);

		cMatrices.p_model_matrix = cMatrices.model_matrix*central_object_scale_matrix*GLSL::scale((float)volume_texture->width/max, (float)volume_texture->height/max, (float)volume_texture->depth/max);
		cMatrices.p_view_model_matrix = cMatrices.view_matrix*cMatrices.p_model_matrix;

		cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.p_view_model_matrix;

		cObjectRenderers.cGlRenderFluidBorder.render(	cGlLights,
														cMatrices.p_pvm_matrix,
														cMatrices.p_view_model_matrix,
														cMatrices.p_view_model_normal_matrix3
											);
	}


	/**************************
	 * RENDER STANDARD SCENE
	 **************************/
	if (cConfig.render_scene)
	{
		cMatrices.p_view_model_matrix = cMatrices.view_matrix;
		cMatrices.p_view_model_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_view_model_matrix));
		cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.p_view_model_matrix;

		cObjectRenderers.cGlRenderScene.render(	cGlLights,
												cMatrices.p_pvm_matrix,
												cMatrices.p_view_model_matrix,
												cMatrices.p_view_model_normal_matrix3
											);
	}

	if (cConfig.render_mesh_bunny || cConfig.render_mesh_sphere)
	{
		cMatrices.p_view_model_matrix = cMatrices.view_matrix * cMatrices.model_matrix;
		cMatrices.p_view_model_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_view_model_matrix));
		cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.p_view_model_matrix;

		CGlProgramUse program(cShaders.cBlinn);
		cShaders.cBlinn.setupUniforms(cObjectRenderers.cGlRenderObjFileBunny.cGlMaterial,
									cGlLights,
									cMatrices.view_matrix*cGlLights.light0_world_pos4);

		cShaders.cBlinn.setupUniformsMatrices(cMatrices.p_pvm_matrix, cMatrices.p_view_model_normal_matrix3, cMatrices.p_view_model_matrix);

		cTextures.texture1.bind();

		if (cConfig.render_mesh_bunny
			&& !(	cConfig.render_marching_cubes_vertex_array_front_back_refractions			||
					cConfig.render_marching_cubes_vertex_array_front_back_depth_voxels_refractions	||
					cConfig.render_marching_cubes_vertex_array_front_back_total_reflections			||
					cConfig.render_marching_cubes_vertex_array_multi_layered_refractions
			))
		{
			if (cObjectRenderers.cGlRenderObjFileBunny.cGlMaterial.texture0)
				cObjectRenderers.cGlRenderObjFileBunny.cGlMaterial.texture0->bind();

			cObjectRenderers.cGlRenderObjFileBunny.renderWithoutProgram();

			if (cObjectRenderers.cGlRenderObjFileBunny.cGlMaterial.texture0)
				cObjectRenderers.cGlRenderObjFileBunny.cGlMaterial.texture0->unbind();
		}


		if (cConfig.render_mesh_sphere
									&& !(	cConfig.render_marching_cubes_vertex_array_front_back_refractions			||
										cConfig.render_marching_cubes_vertex_array_front_back_depth_voxels_refractions	||
										cConfig.render_marching_cubes_vertex_array_front_back_total_reflections			||
										cConfig.render_marching_cubes_vertex_array_multi_layered_refractions
									))
		{
			if (cObjectRenderers.cGlRenderObjFileSphere.texture_valid)
				cShaders.cBlinn.texture0_enabled.set1b(true);
			else
				cShaders.cBlinn.texture0_enabled.set1b(false);

			cObjectRenderers.cGlRenderObjFileSphere.renderWithoutProgram();
		}
		cTextures.texture1.unbind();
	}


	/********************************************************************
	 * render obstacles with reflections using cube map
	 ********************************************************************/
	if (cConfig.render_mesh_bunny_reflected || cConfig.render_mesh_sphere_reflected)// || switches.sphere_reflected)
	{
		private_class->cGlCubeMap.texture_cube_map.bind();

		cMatrices.p_view_model_matrix = cMatrices.view_matrix*cMatrices.model_matrix;
		cMatrices.p_view_model_normal_matrix3 = GLSL::mat3(GLSL::inverseTranspose(cMatrices.p_view_model_matrix));
		cMatrices.p_pvm_matrix = cMatrices.projection_matrix * cMatrices.p_view_model_matrix;
		cMatrices.p_view_normal_matrix3 = GLSL::mat3(cMatrices.view_matrix);
		GLSL::mat3 p_transposed_view_matrix = GLSL::mat3(GLSL::transpose(cMatrices.view_matrix));

		CGlProgramUse program_use(cShaders.cCubeMapMirror);

		cShaders.cCubeMapMirror.pvm_matrix.set(cMatrices.p_pvm_matrix);
		cShaders.cCubeMapMirror.view_model_normal_matrix3.set(cMatrices.p_view_model_normal_matrix3);
		cShaders.cCubeMapMirror.view_model_matrix.set(cMatrices.p_view_model_matrix);

		cShaders.cCubeMapMirror.light0_view_pos3.set(GLSL::vec3(cMatrices.view_matrix*cGlLights.light0_world_pos4));
		// we use the transpose cz. we invert the normal which is invert(transp(invert(M))) = transp(M)
		cShaders.cCubeMapMirror.transposed_view_matrix3.set(p_transposed_view_matrix);

		if (cConfig.render_mesh_bunny_reflected)
			cObjectRenderers.cGlRenderObjFileBunny.renderWithoutProgram();

		if (cConfig.render_mesh_sphere_reflected)
			cObjectRenderers.cGlRenderObjFileSphere.renderWithoutProgram();

		cShaders.cCubeMapMirror.disable();
		private_class->cGlCubeMap.texture_cube_map.unbind();
	}


	/**********************************
	 * render flattened cube map
	 **********************************/
	if (cConfig.display_flat_cube_map)
	{
		private_class->cGlDrawFlatCubeMap.render(private_class->cGlCubeMap, 0.5f);
	}
}

template <typename T>
void CRenderPass<T>::cleanup()
{
	if (private_class->fluid_fraction_buffer != NULL)
	{
		delete [] private_class->fluid_fraction_buffer;
		private_class->fluid_fraction_buffer = NULL;
	}
}

template <typename T>
CRenderPass<T>::~CRenderPass()
{
	cleanup();
	delete private_class;
}


template class CRenderPass<float>;
