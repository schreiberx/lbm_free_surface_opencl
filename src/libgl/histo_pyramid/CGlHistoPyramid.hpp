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
 * CGlHistoPyramid.hpp
 *
 *  Created on: Jan 19, 2010
 *      Author: Martin Schreiber
 */

#ifndef CGLHISTOPYRAMID_HPP_
#define CGLHISTOPYRAMID_HPP_

#include "libmath/CMath.hpp"
#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlProgram.hpp"
#include "libgl/draw/CGlDrawFboQuad.hpp"


/**
 * \brief create histopyramid data out of a 2d flat texture.
 * the data is stored only in the red color channel of the texture!
 */
class CGlHistoPyramid
{
public:
	CError error;			///< error handler
	GLint layers;			///< layers/height of histo pyramid

private:
	GLint width, height;	///< width and height of "basement"
	GLint max_size;			///< max(width,height)

	CGlDrawFboQuad fbo_quad;

	CGlProgram cGlProgramUp;
	CGlProgram cGlProgramDown;

public:
	CGlTexture cTexturePyramidUp;		///< mipmap texture acting as layers for pyramid (bottom-up)
	CGlTexture cTexturePyramidDown;		///< mipmap texture acting as layers for pyramid (top-down)

private:
	// TODO: maybe we can use glViewport with only one target texture to slide within this texture
	// if fbo switches are time consuming, this could be a faster solution
	CGlFbo *cFboPyramidUp[64];			///< fbo writing for layers of histo pyramid

	CGlFbo *cFboPyramidDown[64];		///< fbo writing for layers of histo pyramid (top-down)

	CGlFbo cFboParameterBuffer;		///< fbo writing to base_texture (parameter of create(...))

	CGlUniform current_level_up_uniform;
	CGlUniform current_level_down_uniform;

public:
	int gathered_count;					///< peak value of pyramid

	CGlHistoPyramid()	:
			cTexturePyramidUp(GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT),
			cTexturePyramidDown(GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT)
	{
		for (int i = 0; i < 64; i++)
		{
			cFboPyramidUp[i] = NULL;
			cFboPyramidDown[i] = NULL;
		}
	}

	/**
	 * resize base of histopyramid
	 */
	void resize(	GLint p_width,
					GLint p_height
	)
	{

		for (int i = 0; i < 64; i++)
		{
			if (cFboPyramidUp[i] == NULL)
				break;

			delete cFboPyramidUp[i];
			delete cFboPyramidDown[i];

			cFboPyramidUp[i] = NULL;
		}

		width = CMath::ceil2<GLint>(p_width);
		height = CMath::ceil2<GLint>(p_height);

		max_size = CMath::max<GLint>(width, height);
		layers = CMath::digits2<GLint>(max_size);
#if 0
		std::cout << p_width << " x " << p_height << std::endl;
		std::cout << width << " x " << height << std::endl;
		std::cout << "max size: " << max_size << std::endl;
		std::cout << "layers: " << layers << std::endl;
#endif
		/**
		 * create texture pyramid to save data needed for the top-down process
		 */
		GLsizei current_width = width;
		GLsizei current_height = height;


		// resize mipmap textures
		for(int d = 0; d < layers; d++)
		{
//			std::cout << "init textures: level " << d << "     " << current_width << "x" << current_height << std::endl;

			cTexturePyramidUp.bind();
			cTexturePyramidUp.resizeMipMap(current_width, current_height, d);
			cTexturePyramidUp.unbind();

			cTexturePyramidDown.bind();
			cTexturePyramidDown.resizeMipMap(current_width, current_height, d);
			cTexturePyramidDown.unbind();

			current_width >>= 1;
			current_height >>= 1;
			if (current_width == 0)		current_width = 1;
			if (current_height == 0)	current_height = 1;
		}

		const GLuint color_up[4]={0,0,0,0};
		cTexturePyramidUp.bind();
		cTexturePyramidUp.setParam(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		cTexturePyramidUp.setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		cTexturePyramidUp.setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		cTexturePyramidUp.setParamIuiv(GL_TEXTURE_BORDER_COLOR, &color_up[0]);
		cTexturePyramidUp.unbind();
		CGlErrorCheck();

//		const GLuint color_down[4]={CMath<int>::max(),CMath<int>::max(),CMath<int>::max(),CMath<int>::max()};
		const GLuint color_down[4]={1111, 1111, 1111, 1111};
//		const GLuint color_down[4]={0,0,0,0};
		cTexturePyramidDown.bind();
		cTexturePyramidDown.setParam(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		cTexturePyramidDown.setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		cTexturePyramidDown.setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		cTexturePyramidDown.setParamIuiv(GL_TEXTURE_BORDER_COLOR, color_down);
		cTexturePyramidDown.unbind();
		CGlErrorCheck();

		// initialize the topmost layer value with '0'
		GLint tmp_data = 0;
		cTexturePyramidDown.setDataMipMap(&tmp_data, layers-1);
		CGlErrorCheck();

		// we start with the 2nd layer and store it to array position 0!!!
		for(int d = 0; d < layers; d++)
		{
//			std::cout << "init FBO layer: " << d << std::endl;

			cFboPyramidUp[d] = new CGlFbo;
			cFboPyramidUp[d]->bind();
			cFboPyramidUp[d]->bindTexture(cTexturePyramidUp, 0, d);
			cFboPyramidUp[d]->unbind();
			CGlErrorCheck();

			if (!cFboPyramidUp[d]->checkStatus())
			{
				error << "invalid fbo" << std::endl;
				return;
			}

			cFboPyramidDown[d] = new CGlFbo;
			cFboPyramidDown[d]->bind();
			cFboPyramidDown[d]->bindTexture(cTexturePyramidDown, 0, d);
			cFboPyramidDown[d]->unbind();
			CGlErrorCheck();

			if (!cFboPyramidDown[d]->checkStatus())
			{
				error << "invalid fbo" << std::endl;
				return;
			}
		}

		/*
		 * GLSL program to go bottom-up
		 */
		cGlProgramUp.initVertFragShadersFromDirectory("histo_pyramid/red_up");

		cGlProgramUp.link();

		cGlProgramUp.use();
		cGlProgramUp.setUniform1i("src_texture", 0);
		cGlProgramUp.disable();
		cGlProgramUp.setupUniform(current_level_up_uniform, "current_level");
		CGlErrorCheck();

		/*
		 * GLSL program to go down to the basement of the histopyramid
		 */
		cGlProgramDown.initVertFragShadersFromDirectory("histo_pyramid/red_down");
		cGlProgramDown.link();

		cGlProgramDown.use();
		cGlProgramDown.setUniform1i("down_texture", 0);
		cGlProgramDown.setUniform1i("up_texture", 1);
		cGlProgramDown.disable();
		cGlProgramDown.setupUniform(current_level_down_uniform, "current_level");
		CGlErrorCheck();


		/**
		 * initialize color of basement texture of up pyramid with (0,0,0,0)
		 */
		cFboPyramidUp[0]->bind();
			CGlViewport viewport;
			viewport.saveState();
			viewport.setSize(cTexturePyramidUp.width, cTexturePyramidUp.height);
				glClearColor(0,0,0,0);
				glClear(GL_COLOR_BUFFER_BIT);
			viewport.restoreState();
		cFboPyramidUp[0]->unbind();
	}

	/**
	 * create the histopyramid data
	 * \param base_texture	texture with the following properties:
	 *
	 * 						type has to be a "GL_TEXTURE_2D"
	 *
	 * 						the number of elements has to be stored in the red channel as a float value
	 *
	 * 						the textured has to be clamped to the border and the
	 * 						value of the red border color has to be 0.0:
	 *
	 *	base_texture.setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	 *	base_texture.setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	 *	GLfloat color[4]={0,0,0,0};
	 *	base_texture.setParamfv(GL_TEXTURE_BORDER_COLOR, color);
	 */
	void create(CGlTexture &base_texture)
	{
		fbo_quad.draw_save_state();
		fbo_quad.draw_init_array();

		/**
		 * UP
		 */
		cGlProgramUp.use();

			/**
			 * copy first slice to "basement of pyramid"
			 */
#if 0
			fbo_quad.draw_viewport_resize(width, height);
			cFboPyramidUp[0]->bind();
			glClearColor(0,0,0,0);
			glClear(GL_COLOR_BUFFER_BIT);
			cFboPyramidUp[0]->unbind();
#endif
			cFboParameterBuffer.bind();
			cFboParameterBuffer.bindTexture(base_texture);
			fbo_quad.draw_viewport_resize(base_texture.width, base_texture.height);

			glReadBuffer(GL_COLOR_ATTACHMENT0);
				cTexturePyramidUp.bind();

				glCopyTexSubImage2D(	GL_TEXTURE_2D, 0,
										0, 0, 0, 0,
										base_texture.width, base_texture.height
										);
				cTexturePyramidUp.unbind();

			cFboParameterBuffer.unbind();


			/**
			 * bottom-up
			 *
			 * bind mipmap texture.
			 * this can create render loops!!!!
			 * maybe we have to use glFlush() to avoid those on different platforms!!!
			 */
			cTexturePyramidUp.bind();

			for(int d = 1; d < layers; d++)
			{
				GLint width = cTexturePyramidUp.width >> d;
				GLint height = cTexturePyramidUp.height >> d;
				if (width == 0)		width = 1;
				if (height == 0)	height = 1;

//				std::cout << "create UP layer: " << d << "          " << width << "x" << height << std::endl;

				cFboPyramidUp[d]->bind();
				fbo_quad.draw_viewport_resize(width, height);
				current_level_up_uniform.set1i(d-1);
					fbo_quad.draw();

//				glFinish();

				CGlErrorCheck();
			}
			cTexturePyramidUp.unbind();

		cGlProgramUp.disable();

		/**
		 * read gathered value
		 */
		cTexturePyramidUp.getData(&gathered_count, layers-1);
		CGlErrorCheck();

		/**
		 * DOWN
		 */
		cGlProgramDown.use();
			/**
			 * top-bottom
			 */
			glActiveTexture(GL_TEXTURE0);
			cTexturePyramidDown.bind();
			glActiveTexture(GL_TEXTURE1);
			cTexturePyramidUp.bind();

			for(int d = layers-2; d >= 0; d--)
			{
				GLint width = cTexturePyramidDown.width >> d;
				GLint height = cTexturePyramidDown.height >> d;
				if (width == 0)		width = 1;
				if (height == 0)	height = 1;

//				std::cout << "create DOWN layer: " << d << "          " << width << "x" << height << std::endl;

				cFboPyramidDown[d]->bind();
				fbo_quad.draw_viewport_resize(width, height);
				current_level_down_uniform.set1i(d);
					fbo_quad.draw();

//				glFinish();

				CGlErrorCheck();
			}

			cTexturePyramidUp.unbind();
			glActiveTexture(GL_TEXTURE0);
			cTexturePyramidDown.unbind();

		cGlProgramDown.disable();

		fbo_quad.draw_finish_array();
		fbo_quad.draw_restore_state();
	}

	/**
	 * free unused classes
	 */
	void clear()
	{
		for (int i = 0; i < 64; i++)
		{
			if (cFboPyramidUp[i] == NULL)
				break;

			delete cFboPyramidUp[i];
			delete cFboPyramidDown[i];
		}
	}

	~CGlHistoPyramid()
	{
		clear();
	}

#if 0
	GLint *test_data;

	void test()
	{
		int SIZE_X = 24;
		int SIZE_Y = 48;
		resize(SIZE_X,SIZE_Y);
		if (error())
		{
			std::cout << error.getString();
			return;
		}

		CGlTexture a(GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
		a.resize(SIZE_X,SIZE_Y);

		test_data = new GLint[SIZE_Y*SIZE_X];

		int sum = 0;
		for(int y = 0; y < SIZE_Y; y++)
			for(int x = 0; x < SIZE_X; x++)
			{
				test_data[x+SIZE_X*y] = ((x+1)*(y+1))%13;
				test_data[x+SIZE_X*y] = 1;
				sum += test_data[x+SIZE_X*y];
			}

		a.setData(test_data);
		create(a);

		std::cout << "---------------------------------------------------------------" << std::endl;

		for (int i = 0; i < layers; i++)
		{
			std::cout << std::endl;
			std::cout << "up level " << i << ":" << std::endl;
			cTexturePyramidUp.printR32UI(i);
		}

		std::cout << std::endl;
		std::cout << "SUM / gathered value: " << sum << " / " << gathered_count << std::endl;
		std::cout << std::endl;

		for (int i = layers-2; i >= 0; i--)
		{
			std::cout << std::endl;
			std::cout << "down level " << i << ":" << std::endl;
			cTexturePyramidDown.printR32UI(i);
		}

		std::cout << "gathered count: " << gathered_count << std::endl;

		delete [] test_data;
		exit(1);
	}

#endif
};

#endif /* CGLHISTOPYRAMID_HPP_ */
