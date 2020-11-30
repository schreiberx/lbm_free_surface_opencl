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


#ifndef CGL_VIEWSPACE_REFRACTIONS_MULTI_LAYERED_HPP
#define CGL_VIEWSPACE_REFRACTIONS_MULTI_LAYERED_HPP

#include "libgl/peeling/CGlDepthPeelingVoxels.hpp"
#include "libgl/draw/CGlDrawFboQuad.hpp"
#include "lib/CError.hpp"

/**
 * render refractions in viewspace
 *
 * this implementation depends on depth peeling textures
 */
class CGlViewspaceRefractionsMultiLayered	:
	public CShaderBlinnSkeleton
{
	CGlUniform inv_viewport_uniform;
	CGlUniform inv_view_matrix3_uniform;
	CGlUniform projection_matrix_uniform;
	CGlUniform refraction_index_uniform;	///< uniform to refraction index

	CGlDrawFboQuad cGlDrawFboQuad;

public:
	CError error;		///< error message

	CGlProgram cGlProgram;
	CGlDepthPeelingVoxels cGlDepthPeelingVoxels;	///< depth peeling for refractions

	GLsizei render_viewport_width;	///< width of rendering viewport
	GLsizei render_viewport_height;	///< height of rendering viewport

	int max_peelings;	///< maximum peeling layers

	/**
	 * initialize viewspace refractions
	 */
	CGlViewspaceRefractionsMultiLayered(	int p_max_peelings = 4	///< maximum number of peeling layers
			)
	{
		max_peelings = p_max_peelings;

		/*
		 * GLSL program for refractions in viewspace
		 */
		std::ostringstream program_defines;
		program_defines << "#version 150" << std::endl;
		program_defines << "#define MAX_PEELINGS " << max_peelings << std::endl;

		cGlProgram.setSourcePrefix(program_defines.str());
		cGlProgram.initVertFragShadersFromDirectory("viewspace_refractions/multi_layered");
		cGlProgram.attachFragShader(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader_skeleton_volume.glsl");

		cGlProgram.link();

		initBlinnSkeleton(cGlProgram);

		cGlProgram.use();
		cGlProgram.setupUniform(inv_viewport_uniform, "inv_viewport");
		cGlProgram.setupUniform(inv_view_matrix3_uniform, "inv_view_matrix3");
		cGlProgram.setupUniform(projection_matrix_uniform, "projection_matrix");
		cGlProgram.setupUniform(refraction_index_uniform, "refraction_index");

		cGlProgram.setUniform1i("vertex_position", 0);

		cGlProgram.setUniform1i("first_voxel_texture", 0);
		cGlProgram.setUniform1i("first_normal_texture", 1);

		cGlProgram.setUniform1i("voxel_texture", 2);
		cGlProgram.setUniform1i("normal_texture", 3);

		cGlProgram.setUniform1i("cube_map_texture", 4);
		cGlProgram.disable();

		CGlErrorCheck();
	}


	/**
	 * setup rendering of viewspace refractions
	 */
	void setup(
				GLsizei p_render_viewport_width,	///< width of rendering viewport and first depth peeling texture
				GLsizei p_render_viewport_height,	///< height of rendering viewport and first depth peeling texture
				GLsizei p_peeling_width,			///< width of peeling textures
				GLsizei p_peeling_height,			///< height of peeling textures
				int p_max_peelings = 4				///< maximum depth peeling textures
			)
	{
		render_viewport_width = p_render_viewport_width;
		render_viewport_height = p_render_viewport_height;

		cGlDepthPeelingVoxels.setup(p_render_viewport_width, p_render_viewport_height, p_peeling_width, p_peeling_height, p_max_peelings);
	}

	/**
	 * setup rendering of viewspace refractions only by specifying the maximum peeling layers
	 *
	 * the resolution of the depth peeling textures can be specified later using setup()
	 */
	void setup(
				int p_max_peelings = 4				///< maximum depth peeling textures
			)
	{
		render_viewport_width = 64;
		render_viewport_height = 64;

		cGlDepthPeelingVoxels.setup(render_viewport_width, render_viewport_height, render_viewport_width, render_viewport_height, p_max_peelings);
	}

	/**
	 * this function has to be called, when the viewport is resized (e.g. when the user resized the window)
	 */
	void resizeViewport(GLsizei p_render_viewport_width, GLsizei p_render_viewport_height)
	{
		render_viewport_width = p_render_viewport_width;
		render_viewport_height = p_render_viewport_height;

		cGlDepthPeelingVoxels.resizeViewport(p_render_viewport_width, p_render_viewport_height);
	}


	/**
	 * prepare rendering the viewspace refractions
	 */
	void create(	const GLSL::mat4 &projection_matrix,		///< projection matrix
					const GLSL::mat4 &view_matrix,			///< view matrix
					CGlRenderCallbackTypes::call_render_pass *callback,
					void *user_data							///< user specific data
		)
	{
		cGlDepthPeelingVoxels.create(projection_matrix, view_matrix, callback, user_data);
	}


	/**
	 * render the viewspace refractions
	 */
	/*
	void render(	float depth_range_near,	///< near plane
					float depth_range_far	///< far plane
			)
	*/

	void render(	CGlTexture &cube_map_texture,		///< cube map refractions for outgoing rays
					const GLSL::mat3 &inv_view_matrix3,	///< inverse of view matrix
					const GLSL::mat4 &projection_matrix,		///< inverse transpose of pvm matrix
														///<
														///< we just need the first 2 rows, otherwise we are not
														///< allowed to convert to mat3 before inversing
					float refraction_index				///< refraction index
			)
	{
		cGlProgram.use();

		inv_viewport_uniform.set(CVector<2,float>(1.0/render_viewport_width, 1.0/render_viewport_height));
		inv_view_matrix3_uniform.set(inv_view_matrix3);
		refraction_index_uniform.set1f(refraction_index);
		projection_matrix_uniform.set(projection_matrix);

			cGlDepthPeelingVoxels.first_texture.bind();

			glActiveTexture(GL_TEXTURE1);
			cGlDepthPeelingVoxels.first_normal_texture.bind();

			glActiveTexture(GL_TEXTURE2);
			cGlDepthPeelingVoxels.peeling_voxel_texture.bind();

			glActiveTexture(GL_TEXTURE3);
			cGlDepthPeelingVoxels.peeling_normal_texture.bind();

			glActiveTexture(GL_TEXTURE4);
			cube_map_texture.bind();

				cGlDrawFboQuad.draw_init_array();
					cGlDrawFboQuad.draw();
				cGlDrawFboQuad.draw_finish_array();

			cube_map_texture.unbind();
			glActiveTexture(GL_TEXTURE3);

			cGlDepthPeelingVoxels.peeling_normal_texture.unbind();
			glActiveTexture(GL_TEXTURE2);

			cGlDepthPeelingVoxels.peeling_voxel_texture.unbind();
			glActiveTexture(GL_TEXTURE1);

			cGlDepthPeelingVoxels.first_normal_texture.unbind();
			glActiveTexture(GL_TEXTURE0);

			cGlDepthPeelingVoxels.first_texture.unbind();

		cGlProgram.disable();
		CGlErrorCheck();
	}

};

#endif
