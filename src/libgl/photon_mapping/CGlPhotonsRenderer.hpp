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
 * CGlPhotonsRenderer
 *
 *  Created on: Feb 25, 2010
 *      Author: martin
 */

#ifndef CGL_PHOTONS_RENDERER_HPP_
#define CGL_PHOTONS_RENDERER_HPP_

#include "libgl/core/CGlTexture.hpp"
#include "lib/CFlatTextureLayout.hpp"
#include "shaders/shader_blinn/CShaderBlinnSkeleton.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/draw/CGlDrawFboQuad.hpp"



/**
 * \brief render photons directly to screen using splats
 */
class CGlPhotonsRenderer
{
public:
	bool verbose;	///< verbose mode on/off

    /*
     * render photons
     */
	CGlProgram cGlProgramRenderSplats;			///< OpenGL program to render photons

	CGlUniform texture_width_uniform;
 	CGlUniform texture_height_uniform;

 	CGlUniform projection_matrix_uniform;

 	CGlUniform view_MUL_inv_light_view_matrix_uniform;
 	CGlUniform view_normal_MUL_transpose_light_view_matrix3_uniform;

 	CGlUniform splat_energy_uniform;
 	CGlUniform splat_size_uniform;

 	GLuint buffer_indices;			///< indices in buffer

 	CGlVertexArrayObject vao_index;
	CGlBuffer index_buffer;

	CGlVertexArrayObject vao_quad_vertices;
	CGlBuffer quad_vertices_buffer;

	CGlDrawFboQuad cGlDrawFboQuad;


	CGlTexture photon_blend_texture;
	CGlFbo cGlBlendFbo;

	CGlProgram cGlProgramBlendPhotons;			///< blend rendered photons to current framebuffer

public:
    bool valid;		///< marching cubes are valid
	CError error;	///< error handler


    /**
     * constructor for marching cubes
     */
	CGlPhotonsRenderer(bool p_verbose = false)	:
		verbose(p_verbose),
		photon_blend_texture(GL_TEXTURE_2D, GL_R16F, GL_RED, GL_FLOAT)
	{
    	CGlErrorCheck();
    	valid = false;
	}


	/**
	 * reset the index buffer to the new size
	 *
	 * this is useful for adaptive texture sizes
	 */
	void resizeIndexBuffer(GLuint new_size)
	{
		GLuint *indices = new GLuint[new_size];
		for (GLuint i = 0; i < new_size; i++)
			indices[i] = i;


		vao_index.bind();
			index_buffer.bind();
			index_buffer.data(sizeof(GLuint)*new_size, indices);
			glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 0, NULL);
			glEnableVertexAttribArray(0);
		vao_index.unbind();

		delete [] indices;

		buffer_indices = new_size;
	}


    /**
     * reset rendering
     */
    void setup(	CGlTexture &photon_view_pos_texture	)
    {
    	valid = false;

    	/**
    	 * setup buffers
    	 */
		const GLfloat quad_vertices[4][4] = {
					{-1.0, -1.0, 0.0, 1.0},
					{ 1.0, -1.0, 0.0, 1.0},
					{-1.0,  1.0, 0.0, 1.0},
					{ 1.0,  1.0, 0.0, 1.0},
				};

		vao_quad_vertices.bind();
		quad_vertices_buffer.bind();
		quad_vertices_buffer.data(sizeof(quad_vertices), quad_vertices);
		vao_quad_vertices.unbind();

    	/**
    	 * setup textures
    	 */
		resizeIndexBuffer(photon_view_pos_texture.width*photon_view_pos_texture.height);

		/*
		 * GLSL program
		 */
		cGlProgramRenderSplats.initVertFragShadersFromDirectory("photon_mapping/viewspace_splats_render");

		cGlProgramRenderSplats.link();

		cGlProgramRenderSplats.setupUniform(projection_matrix_uniform, "projection_matrix");

		cGlProgramRenderSplats.setupUniform(view_MUL_inv_light_view_matrix_uniform, "view_MUL_inv_light_view_matrix");
		cGlProgramRenderSplats.setupUniform(view_normal_MUL_transpose_light_view_matrix3_uniform, "view_normal_MUL_transpose_light_view_matrix3");

		cGlProgramRenderSplats.setupUniform(texture_width_uniform, "texture_width");
		cGlProgramRenderSplats.setupUniform(texture_height_uniform, "texture_height");

		cGlProgramRenderSplats.setupUniform(splat_energy_uniform, "splat_energy");
		cGlProgramRenderSplats.setupUniform(splat_size_uniform, "splat_size");
		{
			CGlProgramUse program_use(cGlProgramRenderSplats);

			cGlProgramRenderSplats.setUniform1i("photon_light_view_pos_texture", 0);
			cGlProgramRenderSplats.setUniform1i("photon_light_view_dir_texture", 1);
		}


		/*
		 * float framebuffer to gather photons in viewspace
		 */
		photon_blend_texture.bind();
		photon_blend_texture.resize(16, 16);
		photon_blend_texture.unbind();

		cGlBlendFbo.bind();
		cGlBlendFbo.bindTexture(photon_blend_texture, 0);
		cGlBlendFbo.unbind();


		/*
		 * blending photons to framebuffer
		 */
		cGlProgramBlendPhotons.detachAndFreeShaders();
		cGlProgramBlendPhotons.initVertFragShadersFromDirectory("photon_mapping/viewspace_splats_render_postprocess");

		cGlProgramBlendPhotons.link();

		{
			CGlProgramUse program_use(cGlProgramBlendPhotons);
			cGlProgramBlendPhotons.setUniform1i("photon_blend_texture", 0);
			cGlProgramBlendPhotons.setUniform1i("vertices", 0);
		}

		buffer_indices = 0;

		CGlErrorCheck();
		valid = true;
    }



    /**
     * setup the attenuation and splat size of every photon
     *
     * e.g. when the fov is increased, each photons energy has to be increased.
     *
     * we only use an approximation for the photon energy using the assumption, that the fov angle is very small.
     *
     * then each pixels covers an almost equal area on the viewport (in lightspace).
     */
    void setupSplatEnergyAndSplatSize(
							float light_energy,			///< energy of light

							float area,					///< area (length units) of photon viewport in worldspace

							int photon_texture_width,	///< width of photon texture
							int photon_texture_height,	///< height of photon texture

							float p_splat_size,			///< splat size rendered with glFrustum(-1,1,-1,1,1,..) at position (0,0,-1)
							float splat_energy = 1000.0	///< base of splat energy
					)
    {
		// attenuation decreases with the light energy
		splat_energy *= light_energy;

		// splat energy increases with the covered worldspace area
		// (one image pixel covers a larger area passing photons)
		splat_energy *= area;

		// splat energy decreases with increasement of the photon texture size
		splat_energy /= (float)(photon_texture_width*photon_texture_height);

		CGlProgramUse program_use(cGlProgramRenderSplats);
	 	splat_size_uniform.set1f(p_splat_size);
		splat_energy_uniform.set1f(splat_energy);

//		std::cout << "splat size: " << p_splat_size << "     energy: " << splat_energy << std::endl;
    }


    void resizeViewport(int width, int height)
    {
    	photon_blend_texture.bind();
    	photon_blend_texture.resize(width, height);
    	photon_blend_texture.unbind();
    }


    /**
     * render photons
     */
    void render(	const GLSL::mat4 &p_projection_matrix,				///< projection matrix to render photons
					const GLSL::mat4 &p_view_matrix,					///< view matrix to render photons
					const GLSL::mat3 &p_view_normal_matrix3,			///< transpose inverse view matrix to render photons
    				const GLSL::mat4 &p_inverse_light_view_matrix,		///< lightspace: inverse of view matrix for light space (photons to world space)
					const GLSL::mat3 &p_transpose_light_view_matrix3,	///< lightspace: transpose of view matrix for light space (photons directions to world space)
					CGlTexture &photon_view_pos_texture,
					CGlTexture &photon_normal_attenuation_texture
    		)
    {
    	if (!valid)	return;

#define PHOTONS_RENDER_FBO	1

#if 1
		// enable blending
		CGlStateEnable blend(GL_BLEND);

		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#endif
    	{
			CGlProgramUse program_use(cGlProgramRenderSplats);

			/**
			 * we use a framebuffer using float R and A components
			 *
			 * this is necessary because using the standard buffer, only a granulation of 1.0/256.0 is available
			 */
#if PHOTONS_RENDER_FBO
			cGlBlendFbo.bind();
			glClearColor(0,0,0,0);
			glClear(GL_COLOR_BUFFER_BIT);
#endif

				texture_width_uniform.set1i(photon_view_pos_texture.width);
				texture_height_uniform.set1i(photon_view_pos_texture.height);

				projection_matrix_uniform.set(p_projection_matrix);
				view_MUL_inv_light_view_matrix_uniform.set(p_view_matrix*p_inverse_light_view_matrix);
				view_normal_MUL_transpose_light_view_matrix3_uniform.set(p_view_normal_matrix3*p_transpose_light_view_matrix3);

				CGlStateDisable depth_test(GL_DEPTH_TEST);
				CGlStateEnable point_size(GL_PROGRAM_POINT_SIZE);

				/*
				 * enable fake index buffer array because no vertices are emitted,
				 * if all attribute arrays are deactivated (this seems to be a bug in the NVIDIA driver for OpenGL 3.2)
				 */
				vao_index.bind();

					photon_view_pos_texture.bind();

					glActiveTexture(GL_TEXTURE1);
					photon_normal_attenuation_texture.bind();

						glDrawArrays(GL_POINTS, 0, buffer_indices);

					photon_normal_attenuation_texture.unbind();

					glActiveTexture(GL_TEXTURE0);
					photon_view_pos_texture.unbind();

				vao_index.unbind();

#if PHOTONS_RENDER_FBO
			cGlBlendFbo.unbind();
#endif
    	}

#if PHOTONS_RENDER_FBO
		/**
		 * postprocessing: draw
		 */
    	{
    		CGlProgramUse program_use(cGlProgramBlendPhotons);
			CGlStateDisable depth_test(GL_DEPTH_TEST);

			photon_blend_texture.bind();

				cGlDrawFboQuad.draw_init_array();
				cGlDrawFboQuad.draw();
				cGlDrawFboQuad.draw_finish_array();

			photon_blend_texture.unbind();
    	}
#endif
    	CGlErrorCheck();
    }
};


#endif
