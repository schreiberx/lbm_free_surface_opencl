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


#ifndef CGL_VOLUME_RENDERER_INTERPOLATED_FRONT_BACK_FACES_HPP
#define CGL_VOLUME_RENDERER_INTERPOLATED_FRONT_BACK_FACES_HPP

#include "libgl/draw/CGlDrawVolumeBox.hpp"
#include "libgl/core/CGlProgram.hpp"
#include "libgl/core/CGlBuffer.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/core/CGlViewport.hpp"
#include "lib/CError.hpp"
#include "libmath/CGlSlMath.hpp"
#include "libmath/CGlSlMath.hpp"

/**
 * \brief OpenGL volume renderer using interpolation to extract front and back faces of volume
 */
class CGlVolumeRendererInterpolatedFrontBackFaces
{
private:
	CGlDrawVolumeBox volume_box;
	CGlViewport viewport;

	CGlUniform pvm_matrix_uniform;			///< uniform for pvm matrix
	CGlUniform model_matrix_uniform;			///< uniform for model matrix
	CGlUniform view_matrix_uniform;			///< uniform for view matrix
	CGlUniform view_model_matrix_uniform;				///< uniform for view*model
	CGlUniform view_model_normal_matrix3_uniform;		///< uniform for inv(transp(view*model)) matrix

	CGlUniform texture_size_uniform;			///< uniform for texture size
	CGlUniform inv_texture_size_uniform;		///< uniform for inverse of texture size
	CGlUniform texture_size_add_one_uniform;		///< uniform for texture_size+vec3(1)
	CGlUniform inv_texture_size_add_one_uniform;	///< uniform for inverse of texture_size+vec3(1)

	CGlUniform step_length_uniform;			///< uniform for step length
	CGlUniform gradient_delta_uniform;		///< uniform for displacements to compute gradient for normals

	GLsizei width;		///< width of peeling textures
	GLsizei height;		///< height of peeling textures

public:
	CGlProgram cGlProgram;		///< volume renderer OpenGL program

	CError error;				///< error handler

	CGlFbo front_back_fbo;				///< fbo for front and back faces

	CGlFbo front_fbo;					///< fbo for front faces
	CGlFbo front_depth_fbo;					///< fbo for front faces
	CGlTexture front_texture;			///< texture for front faces
	CGlTexture front_normal_texture;	///< normal texture for front faces
	CGlTexture front_depth_texture;		///< depth texture for front faces

	CGlFbo back_fbo;					///< fbo for back faces
	CGlFbo back_depth_fbo;					///< fbo for back faces
	CGlTexture back_texture;			///< texture stack
	CGlTexture back_normal_texture;		///< texture stack
	CGlTexture back_depth_texture;		///< depth textures


	/**
	 * resize textures
	 */
	void resize(
					GLsizei p_width,		///< width of viewport for first pass
					GLsizei p_height		///< height of viewport for first pass
	)
	{
		/**
		 * resize first pass textures
		 */
		width = p_width;
		height = p_height;

		front_texture.bind();
		front_texture.resize(width, height);
		front_texture.unbind();

		front_normal_texture.bind();
		front_normal_texture.resize(width, height);
		front_normal_texture.unbind();

		front_depth_texture.bind();
		front_depth_texture.resize(width, height);
		front_depth_texture.unbind();

		back_texture.bind();
		back_texture.resize(width, height);
		back_texture.unbind();

		back_normal_texture.bind();
		back_normal_texture.resize(width, height);
		back_normal_texture.unbind();

		back_depth_texture.bind();
		back_depth_texture.resize(width, height);
		back_depth_texture.unbind();

		CGlErrorCheck();
	}

	/**
	 * initialize volume rendering
	 */
	CGlVolumeRendererInterpolatedFrontBackFaces()	:
							front_texture(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
							front_normal_texture(	GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
							front_depth_texture(	GL_TEXTURE_2D, GL_R32F, GL_RED, GL_FLOAT),

							back_texture(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
							back_normal_texture(	GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
							back_depth_texture(		GL_TEXTURE_2D, GL_R32F, GL_RED, GL_FLOAT)
	{
		cGlProgram.initVertFragShadersFromDirectory("volume_renderer/interpolated_front_back_surface");

		cGlProgram.bindAttribLocation(0, "vertex_position");
		cGlProgram.bindAttribLocation(1, "vertex_texture_coord");

		cGlProgram.link();
		CGlErrorCheck();

		cGlProgram.setupUniform(pvm_matrix_uniform, "pvm_matrix");
		cGlProgram.setupUniform(view_matrix_uniform, "view_matrix");
		cGlProgram.setupUniform(view_model_matrix_uniform, "view_model_matrix");
		cGlProgram.setupUniform(model_matrix_uniform, "model_matrix");
		cGlProgram.setupUniform(view_model_normal_matrix3_uniform, "view_model_normal_matrix3");

		cGlProgram.setupUniform(gradient_delta_uniform, "gradient_delta");
		cGlProgram.setupUniform(step_length_uniform, "step_length");

		cGlProgram.setupUniform(texture_size_uniform, "texture_size");
		cGlProgram.setupUniform(inv_texture_size_uniform, "inv_texture_size");
		cGlProgram.setupUniform(texture_size_add_one_uniform, "texture_size_add_one");
		cGlProgram.setupUniform(inv_texture_size_add_one_uniform, "inv_texture_size_add_one");

		cGlProgram.use();
		cGlProgram.setUniform1i("volume_texture", 0);
		cGlProgram.disable();


    	GLint max_color_attachments;
    	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);

    	if (max_color_attachments < 6)
    	{
    		error << "not enough color attachments. " << max_color_attachments << " available, but 6 needed!";
    		return;
    	}

		resize(64, 64);

		front_fbo.bind();
		front_fbo.bindTexture(front_texture, 0);
		front_fbo.unbind();

		front_depth_fbo.bind();
		front_depth_fbo.bindTexture(front_depth_texture, 0);
		front_depth_fbo.unbind();

		back_fbo.bind();
		back_fbo.bindTexture(back_texture, 0);
		back_fbo.unbind();

		back_depth_fbo.bind();
		back_depth_fbo.bindTexture(back_depth_texture, 0);
		back_depth_fbo.unbind();

		front_back_fbo.bind();
		front_back_fbo.bindTexture(front_texture, 0);
		front_back_fbo.bindTexture(front_normal_texture, 1);
		front_back_fbo.bindTexture(front_depth_texture, 2);
		front_back_fbo.bindTexture(back_texture, 3);
		front_back_fbo.bindTexture(back_normal_texture, 4);
		front_back_fbo.bindTexture(back_depth_texture, 5);
		front_back_fbo.unbind();

		CGlErrorCheck();
	}


	/**
	 * start OpenGL volume rendering
	 */
	void render(
			const GLSL::mat4 &pvm_matrix,					///< pvm matrix
			const GLSL::mat4 &view_matrix,					///< view matrix
			const GLSL::mat4 &model_matrix,				///< model matrix
			const GLSL::mat4 &view_model_matrix,			///< view*model matrix
			const GLSL::mat3 &view_model_normal_matrix3,	///< inv(transp(view*model)) matrix

			CGlTexture &volume_texture,				///< 3d volume texture
			float gradientFactor = 0.5f,			///< factor to compute gradient
			float step_size = 0.5f					///< step size (unused)
		)
	{
		const GLenum draw_buffers_start[6] = {	GL_COLOR_ATTACHMENT0,
												GL_COLOR_ATTACHMENT1,
												GL_COLOR_ATTACHMENT2,
												GL_COLOR_ATTACHMENT3,
												GL_COLOR_ATTACHMENT4,
												GL_COLOR_ATTACHMENT5
											};

		const GLenum draw_buffers_finish[6] = {GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE};


		viewport.saveState();
		viewport.setSize(width, height);
		CGlErrorCheck();

		glClearColor(1, 0, 0, 0);

		front_depth_fbo.bind();
		glClear(GL_COLOR_BUFFER_BIT);

		back_depth_fbo.bind();
		glClear(GL_COLOR_BUFFER_BIT);

		glClearColor(0, 0, -CMath::numeric_inf<float>(), 0);

		front_fbo.bind();
		glClear(GL_COLOR_BUFFER_BIT);

		back_fbo.bind();
		glClear(GL_COLOR_BUFFER_BIT);

		float gradient_delta[3];
		gradient_delta[0] = gradientFactor/volume_texture.width;
		gradient_delta[1] = gradientFactor/volume_texture.height;
		gradient_delta[2] = gradientFactor/volume_texture.depth;

		CGlProgramUse program_use(cGlProgram);
		CGlStateDisable depth_test_disable(GL_DEPTH_TEST);


		front_back_fbo.bind();
		glDrawBuffers(6, draw_buffers_start);

			pvm_matrix_uniform.set(pvm_matrix);
			view_matrix_uniform.set(view_matrix);
			model_matrix_uniform.set(model_matrix);
			view_model_matrix_uniform.set(view_model_matrix);
			view_model_normal_matrix3_uniform.set(view_model_normal_matrix3);

			/**
			 * because the unit length in the simple renderer is one domain, we have to
			 * divide the step_length by the maximum number of cells in each dimension
			 */
			int max_cells = (volume_texture.width > volume_texture.height ? volume_texture.width : volume_texture.height);
			max_cells = (max_cells > volume_texture.depth ? max_cells : volume_texture.depth);

			step_length_uniform.set1f(step_size/(float)max_cells);
			gradient_delta_uniform.set3fv(gradient_delta);

			GLfloat texture_size[3];
			texture_size[0] = volume_texture.width;
			texture_size[1] = volume_texture.height;
			texture_size[2] = volume_texture.depth;
			texture_size_uniform.set3fv(texture_size);

			GLfloat inv_texture_size[3];
			inv_texture_size[0] = 1.0f/texture_size[0];
			inv_texture_size[1] = 1.0f/texture_size[1];
			inv_texture_size[2] = 1.0f/texture_size[2];
			inv_texture_size_uniform.set3fv(inv_texture_size);

			GLfloat texture_size_add_one[3];
			texture_size_add_one[0] = volume_texture.width+1.0f;
			texture_size_add_one[1] = volume_texture.height+1.0f;
			texture_size_add_one[2] = volume_texture.depth+1.0f;
			texture_size_add_one_uniform.set3fv(texture_size_add_one);

			GLfloat inv_texture_size_add_one[3];
			inv_texture_size_add_one[0] = 1.0f/texture_size_add_one[0];
			inv_texture_size_add_one[1] = 1.0f/texture_size_add_one[1];
			inv_texture_size_add_one[2] = 1.0f/texture_size_add_one[2];
			inv_texture_size_add_one_uniform.set3fv(inv_texture_size_add_one);

				volume_texture.bind();
glCullFace(GL_BACK);
glEnable(GL_CULL_FACE);
					volume_box.renderWithoutProgram();

				volume_texture.unbind();

		glDrawBuffers(1, draw_buffers_finish);

		front_back_fbo.unbind();

		viewport.restoreState();

		CGlErrorCheck();
	}
};

#endif
