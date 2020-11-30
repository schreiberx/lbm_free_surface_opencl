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


#ifndef CGL_VIEWSPACE_REFRACTIONS_FRONT_BACK_DEPTH_VOXELS_HPP
#define CGL_VIEWSPACE_REFRACTIONS_FRONT_BACK_DEPTH_VOXELS_HPP

#include "libgl/peeling/CGlPeelingFrontBackFaces.hpp"
#include "libgl/draw/CGlDrawFboQuad.hpp"
#include "lib/CError.hpp"

/**
 * render refractions in viewspace
 *
 * this implementation depends on depth peeling textures
 */
class CGlViewspaceRefractionsFrontBackDepthVoxels	:
	public CShaderBlinnSkeleton
{
	CGlUniform inv_viewport_size_uniform;
	CGlUniform viewport_size_uniform;
	CGlUniform inv_view_matrix3_uniform;
	CGlUniform projection_matrix_uniform;
	CGlUniform inv_projection_matrix_uniform;
	CGlUniform refraction_index_uniform;	///< uniform to refraction index

	CGlDrawFboQuad cGlDrawFboQuad;

public:
	CError error;		///< error message

	CGlProgram cGlProgram;
	CGlPeelingFrontBackFaces cGlPeelingFrontBackFaces;	///< depth peeling for refractions

	GLsizei width;	///< width of rendering viewport
	GLsizei height;	///< height of rendering viewport

	/**
	 * initialize viewspace refractions
	 */
	CGlViewspaceRefractionsFrontBackDepthVoxels()
	{
		/*
		 * GLSL program for refractions in viewspace
		 */
		cGlProgram.initVertFragShadersFromDirectory("viewspace_refractions/front_back_depth_voxels");
		cGlProgram.attachFragShader(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader_skeleton_volume.glsl");

		cGlProgram.link();

		initBlinnSkeleton(cGlProgram);

		cGlProgram.use();
		cGlProgram.setupUniform(inv_viewport_size_uniform, "inv_viewport_size");
		cGlProgram.setupUniform(viewport_size_uniform, "viewport_size");
		cGlProgram.setupUniform(inv_view_matrix3_uniform, "inv_view_matrix3");
		cGlProgram.setupUniform(projection_matrix_uniform, "projection_matrix");
		cGlProgram.setupUniform(inv_projection_matrix_uniform, "inv_projection_matrix");
		cGlProgram.setupUniform(refraction_index_uniform, "refraction_index");

		cGlProgram.setUniform1i("vertex_position", 0);

		cGlProgram.setUniform1i("front_voxel_texture", 0);
		cGlProgram.setUniform1i("front_normal_texture", 1);

		cGlProgram.setUniform1i("back_depth_texture", 2);
		cGlProgram.setUniform1i("back_normal_texture", 3);

		cGlProgram.setUniform1i("cube_map_texture", 4);
		cGlProgram.disable();

		CGlErrorCheck();
	}


	/**
	 * setup rendering of viewspace refractions
	 */
	void setup(
				GLsizei p_width,	///< width of rendering viewport and peeling texture
				GLsizei p_height	///< height of rendering viewport and peeling texture
			)
	{
		width = p_width;
		height = p_height;

		cGlPeelingFrontBackFaces.setup(width, height);
	}



	/**
	 * setup rendering of viewspace refractions
	 */
	void resize(
				GLsizei p_width,	///< width of rendering viewport and peeling texture
				GLsizei p_height	///< height of rendering viewport and peeling texture
			)
	{
		width = p_width;
		height = p_height;

		cGlPeelingFrontBackFaces.resize(width, height);
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
		cGlPeelingFrontBackFaces.create(projection_matrix, view_matrix, callback, user_data);

	}

	/**
	 * render the viewspace refractions
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

		viewport_size_uniform.set(CVector<2,float>(width, height));
		inv_viewport_size_uniform.set(CVector<2,float>(1.0/width, 1.0/height));
		inv_view_matrix3_uniform.set(inv_view_matrix3);
		refraction_index_uniform.set1f(refraction_index);
		projection_matrix_uniform.set(projection_matrix);
		inv_projection_matrix_uniform.set(GLSL::inverse(projection_matrix));

			cGlPeelingFrontBackFaces.front_texture.bind();

			glActiveTexture(GL_TEXTURE1);
			cGlPeelingFrontBackFaces.front_normal_texture.bind();

			glActiveTexture(GL_TEXTURE2);
			cGlPeelingFrontBackFaces.back_depth_texture.bind();

			glActiveTexture(GL_TEXTURE3);
			cGlPeelingFrontBackFaces.back_normal_texture.bind();

			glActiveTexture(GL_TEXTURE4);
			cube_map_texture.bind();

				cGlDrawFboQuad.draw_init_array();
					cGlDrawFboQuad.draw();
				cGlDrawFboQuad.draw_finish_array();

			cube_map_texture.unbind();
			glActiveTexture(GL_TEXTURE3);

			cGlPeelingFrontBackFaces.back_normal_texture.unbind();
			glActiveTexture(GL_TEXTURE2);

			cGlPeelingFrontBackFaces.back_depth_texture.unbind();
			glActiveTexture(GL_TEXTURE1);

			cGlPeelingFrontBackFaces.front_normal_texture.unbind();
			glActiveTexture(GL_TEXTURE0);

			cGlPeelingFrontBackFaces.front_texture.unbind();

		cGlProgram.disable();
		CGlErrorCheck();
	}

};

#endif
