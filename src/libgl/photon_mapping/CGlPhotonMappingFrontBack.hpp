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
 * CGlPhotonMapping.hpp
 *
 *  Created on: Mar 5, 2010
 *      Author: martin
 */

#ifndef CGLPHOTONMAPPING_FRONT_BACK_HPP_
#define CGLPHOTONMAPPING_FRONT_BACK_HPP_

#include "libgl/peeling/CGlPeelingFrontBackFaces.hpp"
#include "libgl/peeling/CGlPeelingFrontFaces.hpp"
#include "libgl/core/CGlViewport.hpp"
#include "lib/CError.hpp"

/**
 * compute photon mapping based on front/back faces of an object in light space
 */
class CGlPhotonMappingFrontBack
{
	CGlProgram cGlProgram;
	CGlUniform peeling_texture_size_uniform;
	CGlUniform inv_peeling_texture_size_uniform;
	CGlUniform inv_photon_texture_size_uniform;
	CGlUniform projection_matrix_uniform;
	CGlUniform refraction_index_uniform;						///< uniform to refraction index
	CGlUniform scale_photon_to_peeling_texture_coord_uniform;

	CGlDrawFboQuad cGlDrawFboQuad;

	CGlFbo cGlFbo;

	CGlViewport viewport;

	float light_energy;

public:
	CError error;		///< error message

	CGlPeelingFrontBackFaces cGlPeelingFrontBackFaces;	///< depth peeling for refractions
	CGlPeelingFrontFaces cGlPeelingFrontDiffuseFaces;	///< diffuse receivers

	CGlTexture photon_position_texture;
	CGlTexture photon_normal_attenuation_texture;

	GLsizei peeling_texture_width;	///< width of rendering viewport
	GLsizei peeling_texture_height;	///< height of rendering viewport

	/**
	 * initialize viewspace refractions
	 */
	CGlPhotonMappingFrontBack()	:
			photon_position_texture(GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
			photon_normal_attenuation_texture(GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT)
	{
	}

	void setup()
	{
		/*
		 * GLSL program for refractions in viewspace
		 */

		cGlProgram.initVertFragShadersFromDirectory("photon_mapping/viewspace_splats_front_back");

		cGlProgram.link();

		cGlProgram.use();
			cGlProgram.setupUniform(peeling_texture_size_uniform, "peeling_texture_size");
			cGlProgram.setupUniform(inv_peeling_texture_size_uniform, "inv_peeling_texture_size");
			cGlProgram.setupUniform(inv_photon_texture_size_uniform, "inv_photon_texture_size");
			cGlProgram.setupUniform(projection_matrix_uniform, "projection_matrix");
			cGlProgram.setupUniform(refraction_index_uniform, "refraction_index");
			cGlProgram.setupUniform(scale_photon_to_peeling_texture_coord_uniform, "scale_photon_to_peeling_texture_coord");

			cGlProgram.setUniform1i("front_voxel_texture", 0);
			cGlProgram.setUniform1i("front_normal_texture", 1);

			cGlProgram.setUniform1i("back_voxel_texture", 2);
			cGlProgram.setUniform1i("back_normal_texture", 3);

			cGlProgram.setUniform1i("diffuse_voxel_texture", 4);
			cGlProgram.setUniform1i("diffuse_normal_texture", 5);
		cGlProgram.disable();

		cGlFbo.bind();
		cGlFbo.bindTexture(photon_position_texture, 0);
		cGlFbo.bindTexture(photon_normal_attenuation_texture, 1);
		cGlFbo.unbind();

		resizePeelingTexture(32, 32);
		resizePhotonTexture(32, 32);

		CGlErrorCheck();
	}



	/**
	 * resolution of the texture storing the peeled refraction data
	 */
	void resizePeelingTexture(
				GLsizei p_width,	///< width of rendering viewport and peeling texture
				GLsizei p_height	///< height of rendering viewport and peeling texture
			)
	{
		peeling_texture_width = p_width;
		peeling_texture_height = p_height;

		cGlPeelingFrontBackFaces.resize(peeling_texture_width, peeling_texture_height);
		cGlPeelingFrontDiffuseFaces.resize(peeling_texture_width, peeling_texture_height);
	}



	/**
	 * resize the texture used to emit and store photons
	 */
	void resizePhotonTexture(
				GLsizei p_width,	///< width of rendering viewport and peeling texture
				GLsizei p_height	///< height of rendering viewport and peeling texture
			)
	{
		photon_position_texture.bind();
		photon_position_texture.resize(p_width, p_height);
		photon_position_texture.unbind();

		photon_normal_attenuation_texture.bind();
		photon_normal_attenuation_texture.resize(p_width, p_height);
		photon_normal_attenuation_texture.unbind();
	}


	/**
	 * create the photon mapping data (photon) storage
	 */
	void create(	const GLSL::mat4 &projection_matrix,		///< projection matrix
					const GLSL::mat4 &view_matrix,				///< view matrix
					float refraction_index,						///< refraction index

					CGlRenderCallbackTypes::call_render_pass *callback_refractive_objects,	///< callback to render refractive objects
					void *user_data_refractive_objects,			///< user specific data

					CGlRenderCallbackTypes::call_render_pass *callback_diffuse_objects,		///< callback to render diffuse objects
					void *user_data_diffuse_objects				///< user specific data
		)
	{
		/**
		 * get peeling textures
		 */
		cGlPeelingFrontBackFaces.create(projection_matrix, view_matrix, callback_refractive_objects, user_data_refractive_objects);

		/**
		 * create texture with diffuse photon receivers
		 */
		cGlPeelingFrontDiffuseFaces.create(projection_matrix, view_matrix, callback_diffuse_objects, user_data_diffuse_objects);

		/**
		 * shoot out photons - HELL YEAH!
		 */
		const GLenum draw_buffers_start[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
		const GLenum draw_buffers_finish[2] = {GL_COLOR_ATTACHMENT0, GL_NONE};

		CGlErrorCheck();

		cGlFbo.bind();
		viewport.saveState();
		viewport.setSize(photon_position_texture.width, photon_position_texture.height);

		glDrawBuffers(2, draw_buffers_start);

			cGlProgram.use();

			peeling_texture_size_uniform.set(GLSL::vec2((float)peeling_texture_width, (float)peeling_texture_height));
			inv_peeling_texture_size_uniform.set(GLSL::vec2(1.0/(float)peeling_texture_width, 1.0/(float)peeling_texture_height));
			inv_photon_texture_size_uniform.set(GLSL::vec2(1.0/(float)photon_position_texture.width, 1.0/(float)photon_position_texture.height));
			scale_photon_to_peeling_texture_coord_uniform.set(GLSL::vec2((float)peeling_texture_width/(float)photon_position_texture.width, (float)peeling_texture_height/(float)photon_position_texture.height));

			refraction_index_uniform.set1f(refraction_index);
			projection_matrix_uniform.set(projection_matrix);

				cGlPeelingFrontBackFaces.front_texture.bind();

				glActiveTexture(GL_TEXTURE1);
				cGlPeelingFrontBackFaces.front_normal_texture.bind();

				glActiveTexture(GL_TEXTURE2);
				cGlPeelingFrontBackFaces.back_texture.bind();

				glActiveTexture(GL_TEXTURE3);
				cGlPeelingFrontBackFaces.back_normal_texture.bind();

				glActiveTexture(GL_TEXTURE4);
				cGlPeelingFrontDiffuseFaces.front_texture.bind();

				glActiveTexture(GL_TEXTURE5);
				cGlPeelingFrontDiffuseFaces.front_normal_texture.bind();

					cGlDrawFboQuad.draw_init_array();
					cGlDrawFboQuad.draw();
					cGlDrawFboQuad.draw_finish_array();

				cGlPeelingFrontDiffuseFaces.front_texture.bind();

				glActiveTexture(GL_TEXTURE4);
				cGlPeelingFrontDiffuseFaces.front_normal_texture.bind();

				glActiveTexture(GL_TEXTURE3);
				cGlPeelingFrontBackFaces.back_normal_texture.unbind();
				glActiveTexture(GL_TEXTURE2);

				cGlPeelingFrontBackFaces.back_texture.bind();
				glActiveTexture(GL_TEXTURE1);

				cGlPeelingFrontBackFaces.front_normal_texture.unbind();
				glActiveTexture(GL_TEXTURE0);

				cGlPeelingFrontBackFaces.front_texture.bind();

			cGlProgram.disable();

		glDrawBuffers(2, draw_buffers_finish);
		cGlFbo.unbind();
		viewport.restoreState();
	}
};

#endif /* CGLPHOTONMAPPING_HPP_ */
