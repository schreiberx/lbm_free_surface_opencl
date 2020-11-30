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
 * CGlHistoPyramidRGBA.hpp
 *
 *  Created on: Jan 19, 2010
 *      Author: Martin Schreiber
 */

#ifndef CGLHISTOPYRAMIDRGBA_HPP_
#define CGLHISTOPYRAMIDRGBA_HPP_

#include "libgl/core/CGlTexture.hpp"
#include "libmath/CMath.hpp"
#include "libgl/core/CGlProgram.hpp"
#include "libgl/draw/CGlDrawFboQuad.hpp"
#include "lib/CBenchmark.hpp"


/**
 * \brief create histopyramid data out of a 2d flat texture.
 * the data is stored only in all (RGBA) color channel of the texture!
 */
class CGlHistoPyramidRGBA
{
public:
	CError error;			///< error handler
	GLint layers;			///< layers/height of histo pyramid

	CGlTexture cTexturePyramidUp;		///< mipmap texture acting as layers for pyramid (bottom-up)
	CGlTexture cTexturePyramidDown;		///< mipmap texture acting as layers for pyramid (top-down)

private:
	GLint width, height;	///< width and height of "basement"
	GLint max_size;			///< max(width,height)

	CGlDrawFboQuad fbo_quad;

	CGlProgram cGlProgramUp;
	CGlProgram cGlProgramDown;


	// TODO: maybe we can use glViewport with only one target texture to slide within this texture
	// if fbo switches are time consuming, this could be a faster solution
	CGlFbo *cFboPyramidUp[64];			///< fbo writing for layers of histo pyramid

	CGlFbo *cFboPyramidDown[64];		///< fbo writing for layers of histo pyramid (top-down)

	CGlFbo cFboParameterBuffer;			///< fbo writing to base_texture (parameter of create(...))

	CGlUniform up_current_level_uniform;

	CGlUniform down_current_down_level_uniform;
	CGlUniform down_current_up_level_uniform;

	int stop_layer_number;
	GLint top_layer_width;
	GLint top_layer_height;

	GLint *top_layer_cpu_storage;
	int top_layer_cpu_storage_size;

public:
	int gathered_count;					///< peak value of pyramid

	CGlHistoPyramidRGBA()	:
			cTexturePyramidUp(GL_TEXTURE_2D, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT),
			cTexturePyramidDown(GL_TEXTURE_2D, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT)
	{
		for (int i = 0; i < 64; i++)
		{
			cFboPyramidUp[i] = NULL;
			cFboPyramidDown[i] = NULL;
		}

		top_layer_cpu_storage = NULL;
	}

	/**
	 * resize base RGBA texture for preprocessing to run histopyramid algorithm
	 */
	void resize(	GLint p_width,					///< width of base texture
					GLint p_height,					///< height of base texture
					GLint p_cpu_gather_layers = 0	///< number of layers to stop gpu gathering and use the cpu for rest of histopyramid
	)
	{
		clear();

		// TODO: maybe ceiling to power of 2 is not necessary
		width = CMath::ceil2<GLint>(p_width);
		height = CMath::ceil2<GLint>(p_height);

		if (p_cpu_gather_layers != 0)
		{
			if (width < height)
			{
				error << "width is smaller than height! this is not safe for p_cpu_gather_layers > 0!!!";
				return;
			}
		}

		max_size = CMath::max<GLint>(width, height);
		layers = CMath::digits2<GLint>(max_size);

		stop_layer_number = layers - p_cpu_gather_layers;
		if (stop_layer_number < 1)
		{
//			std::error << "Invalid value for p_cpu_gather_layers!!!" << CError::endl;
			stop_layer_number = 2;
			p_cpu_gather_layers = layers - 2;
		}
#if 0
		std::cout << p_width << " x " << p_height << std::endl;
		std::cout << width << " x " << height << std::endl;
		std::cout << "max size: " << max_size << std::endl;
		std::cout << "layers: " << layers << std::endl;
		std::cout << "stop layer number: " << stop_layer_number << std::endl;
#endif
		/**
		 * create texture pyramid to save data needed for the top-down process
		 */
		GLsizei current_width;
		GLsizei current_height;

		cTexturePyramidDown.bind();
		current_width = width;
		current_height = height;
		for(int d = 0; d < layers; d++)
		{
			cTexturePyramidDown.resizeMipMap(current_width, current_height, d);
//			std::cout << "HP DOWN size (" << d << ") " << current_width << " x " << current_height << std::endl;
			current_width >>= 1;
			current_height >>= 1;
			if (current_width == 0)		current_width = 1;
			if (current_height == 0)	current_height = 1;
		}
		cTexturePyramidDown.unbind();

		// resize mipmap textures
		current_width = width;
		current_height = height;
		cTexturePyramidUp.bind();
		for(int d = 0; d < stop_layer_number; d++)
		{
			current_width >>= 1;
			current_height >>= 1;
			if (current_width == 0)		current_width = 1;
			if (current_height == 0)	current_height = 1;

			cTexturePyramidUp.resizeMipMap(current_width, current_height, d);
//			std::cout << "HP UP size (" << d << ") " << current_width << " x " << current_height << std::endl;


			CGlErrorCheck();
		}
		cTexturePyramidUp.unbind();

		// always save latest up layer size
		top_layer_width = current_width;
		top_layer_height = current_height;

#if 0
		std::cout << top_layer_width << " " << top_layer_height <<  std::endl;
#endif

		top_layer_cpu_storage_size = top_layer_width * top_layer_height * 4;
		top_layer_cpu_storage = new GLint[top_layer_cpu_storage_size];

		// we start with the 2nd layer and store it to array position 0!!!
		for(int d = 0; d < stop_layer_number; d++)
		{
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
		}


		for(int d = 0; d < stop_layer_number; d++)
		{
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
		cGlProgramUp.initVertFragShadersFromDirectory("histo_pyramid/rgba_up");
		cGlProgramUp.link();

		cGlProgramUp.use();
		cGlProgramUp.setUniform1i("src_texture", 0);
		cGlProgramUp.disable();
		cGlProgramUp.setupUniform(up_current_level_uniform, "current_level");
		CGlErrorCheck();

		/*
		 * GLSL program to go down to the basement of the histopyramid
		 */
		cGlProgramDown.initVertFragShadersFromDirectory("histo_pyramid/rgba_down");

		cGlProgramDown.link();

		cGlProgramDown.use();
		cGlProgramDown.setUniform1i("down_texture", 0);
		cGlProgramDown.setUniform1i("up_texture", 1);
		cGlProgramDown.disable();
		cGlProgramDown.setupUniform(down_current_up_level_uniform, "current_up_level");
		cGlProgramDown.setupUniform(down_current_down_level_uniform, "current_down_level");
		CGlErrorCheck();
	}


	inline int SFC_getPosX_from_index(int sfc_index)
	{
		int ret_val = 0;
		for (int i = 0; sfc_index; i++, sfc_index>>=1)
			ret_val |= sfc_index & (1<<i);

		return ret_val;
/*
		return	((sfc_index>>0) & (1<<0)) |
				((sfc_index>>1) & (1<<1)) |
				((sfc_index>>2) & (1<<2)) |
				((sfc_index>>3) & (1<<3)) |
				((sfc_index>>4) & (1<<4)) |
				((sfc_index>>5) & (1<<5)) |
				((sfc_index>>6) & (1<<6)) |
				((sfc_index>>7) & (1<<7)) |
				((sfc_index>>8) & (1<<8)) |
				((sfc_index>>9) & (1<<9)) |
				((sfc_index>>10) & (1<<10)) |
				((sfc_index>>11) & (1<<11)) |
				((sfc_index>>12) & (1<<12)) |
				((sfc_index>>13) & (1<<13)) |
				((sfc_index>>14) & (1<<14)) |
				((sfc_index>>15) & (1<<15)) |
				((sfc_index>>16) & (1<<16)) |
				((sfc_index>>17) & (1<<17)) |
				((sfc_index>>18) & (1<<18)) |
				((sfc_index>>19) & (1<<19)) |
				((sfc_index>>20) & (1<<20)) |
				((sfc_index>>21) & (1<<21)) |
				((sfc_index>>22) & (1<<22)) |
				((sfc_index>>23) & (1<<23)) |
				((sfc_index>>24) & (1<<24)) |
				((sfc_index>>25) & (1<<25)) |
				((sfc_index>>26) & (1<<26)) |
				((sfc_index>>27) & (1<<27)) |
				((sfc_index>>28) & (1<<28)) |
				((sfc_index>>29) & (1<<29)) |
				((sfc_index>>30) & (1<<30)) |
				((sfc_index>>31) & (1<<31));
*/
	}

	inline int SFC_getPosY_from_index(int sfc_index)
	{
		int ret_val = 0;
		sfc_index >>= 1;
		for (int i = 0; sfc_index; i++, sfc_index>>=1)
			ret_val |= sfc_index & (1<<i);

		return ret_val;
/*
		return	((sfc_index>>1) & (1<<0)) |
				((sfc_index>>2) & (1<<1)) |
				((sfc_index>>3) & (1<<2)) |
				((sfc_index>>4) & (1<<3)) |
				((sfc_index>>5) & (1<<4)) |
				((sfc_index>>6) & (1<<5)) |
				((sfc_index>>7) & (1<<6)) |
				((sfc_index>>8) & (1<<7)) |
				((sfc_index>>9) & (1<<8)) |
				((sfc_index>>10) & (1<<9)) |
				((sfc_index>>11) & (1<<10)) |
				((sfc_index>>12) & (1<<11)) |
				((sfc_index>>13) & (1<<12)) |
				((sfc_index>>14) & (1<<13)) |
				((sfc_index>>15) & (1<<14)) |
				((sfc_index>>16) & (1<<15)) |
				((sfc_index>>17) & (1<<16)) |
				((sfc_index>>18) & (1<<17)) |
				((sfc_index>>19) & (1<<18)) |
				((sfc_index>>20) & (1<<19)) |
				((sfc_index>>21) & (1<<20)) |
				((sfc_index>>22) & (1<<21)) |
				((sfc_index>>23) & (1<<22)) |
				((sfc_index>>24) & (1<<23)) |
				((sfc_index>>25) & (1<<24)) |
				((sfc_index>>26) & (1<<25)) |
				((sfc_index>>27) & (1<<26)) |
				((sfc_index>>28) & (1<<27)) |
				((sfc_index>>29) & (1<<28)) |
				((sfc_index>>30) & (1<<29)) |
				((sfc_index>>31) & (1<<30));
*/
	}

	/**
	 * return the array index for a given position (x,y) for a domain with width p_width
	 */
	inline int SFC_getArrayIndex_from_pos(	int x, int y, int p_width)
	{
		return ((x&(~1))<<1) + (x&1) + ((y&1)<<1) + (y>>1)*p_width*2;
	}

	/**
	 * create the histopyramid data copying the base texture file to the basement of the histopyramids
	 *
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
	 *
	 *	base_texture.setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	 *
	 *	GLfloat color[4]={0,0,0,0};
	 *
	 *	base_texture.setParamfv(GL_TEXTURE_BORDER_COLOR, color);
	 */

	void create(CGlTexture &base_texture)
	{
		GLint width, height;

		CGlErrorCheck();

		fbo_quad.draw_save_state();
		fbo_quad.draw_init_array();

		CGlErrorCheck();

		/**
		 * UP
		 */
		cGlProgramUp.use();

			/**
			 * bottom-up
			 *
			 * bind mipmap texture.
			 * this can create render loops (read fbo <-> write fbo)!!!!
			 * maybe we have to use glFlush() to avoid those on different platforms!!!
			 */

			width = cTexturePyramidUp.width;
			height = cTexturePyramidUp.height;

//			std::cout << "create UP layer: " << 0 << "          " << width << "x" << height << std::endl;

			base_texture.bind();
			cFboPyramidUp[0]->bind();
			fbo_quad.draw_viewport_resize(width, height);
			up_current_level_uniform.set1i(0);
				fbo_quad.draw();

			CGlErrorCheck();

			cTexturePyramidUp.bind();
			CGlErrorCheck();
			for(int d = 1; d < stop_layer_number-1; d++)
			{
				up_current_level_uniform.set1i(d-1);
				cFboPyramidUp[d]->bind();

				width = cTexturePyramidUp.width >> d;
				height = cTexturePyramidUp.height >> d;
				if (width == 0)		width = 1;
				if (height == 0)	height = 1;

//				std::cout << "create UP layer: " << d << "          " << width << "x" << height << std::endl;

				fbo_quad.draw_viewport_resize(width, height);
				fbo_quad.draw();

				CGlErrorCheck();
			}
//			cTexturePyramidUp.unbind();

//		cGlProgramUp.disable();
		/**
		 * read gathered value
		 */

		CGlErrorCheck();
#if 0
		glReadPixels(	0,0,
						top_layer_width, top_layer_height,
						GL_RGBA_INTEGER, GL_UNSIGNED_INT, top_layer_cpu_storage);
		CGlErrorCheck();
#else
		cTexturePyramidUp.getData(top_layer_cpu_storage, stop_layer_number-2);
#endif
//		CGlErrorCheck();
//		glFinish();

		gathered_count = 0;
		for (int i = 0; i < top_layer_cpu_storage_size; i++)
		{
			gathered_count += top_layer_cpu_storage[i];
//			std::cout << ">+ " << top_layer_cpu_storage[i] << std::endl;
		}
//		std::cout << "HP G: " << gathered_count << std::endl;
#if 1
//		std::cout << "top layer storage size: " << top_layer_cpu_storage_size << std::endl;
		// traverse backwards using space filling morton curve (Z-ordered)

		// sfc_index: first index in according RGBA 2x2 cell
		int sfc_index = top_layer_cpu_storage_size-4;

		// compute array index
		int x =	SFC_getPosX_from_index(sfc_index);
		int y =	SFC_getPosY_from_index(sfc_index);

//		std::cout << "ARRAY INDEX: " << x << " " << y << "        SFC: " << sfc_index << std::endl;

		int id = SFC_getArrayIndex_from_pos(x, y, top_layer_width*2);

//		std::cout << "ID: " << id << std::endl;
//		std::cout << "top_layer_width: " << top_layer_width << std::endl;
//		std::cout << "top_layer_height: " << top_layer_height << std::endl;

		GLint *prev_sfc_ptr = &top_layer_cpu_storage[id];
		prev_sfc_ptr[3] = gathered_count - prev_sfc_ptr[3];
		prev_sfc_ptr[2] = prev_sfc_ptr[3] - prev_sfc_ptr[2];
		prev_sfc_ptr[1] = prev_sfc_ptr[2] - prev_sfc_ptr[1];
		prev_sfc_ptr[0] = prev_sfc_ptr[1] - prev_sfc_ptr[0];

//		std::cout << "   + " << prev_sfc_ptr[0] << " " << prev_sfc_ptr[1] << " " << prev_sfc_ptr[2] << " " << prev_sfc_ptr[3] << std::endl;

		for (int sfc_index = top_layer_cpu_storage_size-8; sfc_index >= 0; sfc_index-=4)
		{
			int x =	SFC_getPosX_from_index(sfc_index);
			int y =	SFC_getPosY_from_index(sfc_index);

			int id = SFC_getArrayIndex_from_pos(x, y, top_layer_width*2);

			GLint *this_rgba = &top_layer_cpu_storage[id];

//			std::cout << "ARRAY INDEX: " << x << " " << y << "     SFC index: " << sfc_index << "   ID: " << id << "        Value: " << top_layer_cpu_storage[id] << std::endl;

			this_rgba[3] = prev_sfc_ptr[0] - this_rgba[3];
			this_rgba[2] = this_rgba[3] - this_rgba[2];
			this_rgba[1] = this_rgba[2] - this_rgba[1];
			this_rgba[0] = this_rgba[1] - this_rgba[0];

			prev_sfc_ptr = this_rgba;
		}

		cTexturePyramidDown.setDataMipMap(top_layer_cpu_storage, stop_layer_number-1);

		/*
		 * setup the remaining data for down pyramid.
		 *
		 * this is necessary because that data is needed by the calling algorithms
		 * to traverse the pyramid up-down to find the corresponding index
		 */
		int gather_width = top_layer_width*2;
		int gather_height = top_layer_height*2;

		for (int l = stop_layer_number; l < layers; l++)
		{
			gather_width >>= 1;
			if (gather_width == 0)	gather_width = 1;

			gather_height >>= 1;
			if (gather_height == 0)	gather_height = 1;

			for (int y = 0; y < gather_height; y++)
			{
				int read_id = SFC_getArrayIndex_from_pos(0, y*2, gather_width*2);
				for (int x = 0; x < gather_width; x+=2)
				{
					int write_id = SFC_getArrayIndex_from_pos(x, y, gather_width);

					top_layer_cpu_storage[write_id] = top_layer_cpu_storage[read_id];
					read_id += 4;
					top_layer_cpu_storage[write_id+1] = top_layer_cpu_storage[read_id];
					read_id += 4;
				}
			}

			cTexturePyramidDown.setDataMipMap(top_layer_cpu_storage, l);
		}

#else
//		exit(-1);

		top_layer_cpu_storage[top_layer_cpu_storage_size-1] = gathered_count - top_layer_cpu_storage[top_layer_cpu_storage_size-1];
		for (int i = top_layer_cpu_storage_size-2; i >= 1; i--)
		{
			top_layer_cpu_storage[i] = top_layer_cpu_storage[i+1] - top_layer_cpu_storage[i];
		}
		top_layer_cpu_storage[0] = 0;

		cTexturePyramidDown.setDataMipMap(top_layer_cpu_storage, stop_layer_number-1);
#endif

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

			for(int d = stop_layer_number-2; d >= 1; d--)
			{
				GLint width = cTexturePyramidDown.width >> d;
				GLint height = cTexturePyramidDown.height >> d;
				if (width == 0)		width = 1;
				if (height == 0)	height = 1;

				cFboPyramidDown[d]->bind();
				fbo_quad.draw_viewport_resize(width, height);
				down_current_up_level_uniform.set1i(d-1);
				down_current_down_level_uniform.set1i(d+1);
					fbo_quad.draw();

				CGlErrorCheck();
			}

			base_texture.bind();

			cFboPyramidDown[0]->bind();
			fbo_quad.draw_viewport_resize(cTexturePyramidDown.width, cTexturePyramidDown.height);
			down_current_up_level_uniform.set1i(0);
			down_current_down_level_uniform.set1i(1);
				fbo_quad.draw();

			CGlErrorCheck();
			cFboPyramidDown[0]->unbind();
			base_texture.unbind();
			glActiveTexture(GL_TEXTURE0);
			cTexturePyramidDown.unbind();

		cGlProgramDown.disable();

		fbo_quad.draw_finish_array();
		fbo_quad.draw_restore_state();

#if 0
		unsigned int *data = new unsigned int[base_texture.width*base_texture.height*4];
		base_texture.getData(data);
		unsigned int count = 0;
		for (int i = 0; i < base_texture.width*base_texture.height*4; i++)
			count += data[i];

		std::cout << "DEBUG GATHERED COUNT: " << gathered_count << " " << count << std::endl;
		delete [] data;
#endif
#if 0
		unsigned int *data = new unsigned int[base_texture.width*base_texture.height*4];
		base_texture.getData(data);
		unsigned int mc_cells_count = 0;
		for (int i = 0; i < base_texture.width*base_texture.height*4; i++)
			mc_cells_count += (data[i] != 0 && data[i] != 255);

		std::cout << "MC CELLS: " << mc_cells_count << std::endl;
		delete [] data;
#endif
//		outputPyramids();
//		exit(-1);

	}

	~CGlHistoPyramidRGBA()
	{
		clear();
	}

private:

	void debugOutput()
	{

	}
	void clear()
	{
		for (int i = 0; i < 64; i++)
		{
			if (cFboPyramidUp[i] == NULL)
				break;

			delete cFboPyramidUp[i];
		}

		for (int i = 0; i < 64; i++)
		{
			if (cFboPyramidDown[i] == NULL)
				break;

			delete cFboPyramidDown[i];
		}

		if (top_layer_cpu_storage != NULL)
		{
			delete [] top_layer_cpu_storage;
			top_layer_cpu_storage = NULL;
		}

		cFboPyramidUp[0] = NULL;
		cFboPyramidDown[0] = NULL;
	}

#if 1
	GLint *test_data;
public:

	void outputPyramids()
	{

		for (int i = 0; i < stop_layer_number-1; i++)
		{
			std::cout << std::endl;
			std::cout << "up level " << i << ":" << std::endl;
			cTexturePyramidUp.printRGBA32UI(i);
		}

		std::cout << std::endl;

		for (int i = layers-1; i >= 0; i--)
		{
			std::cout << std::endl;
			std::cout << "down level " << i << ":" << std::endl;
			cTexturePyramidDown.printRGBA32UI(i);
		}

	}



	void test()
	{
		// texture size
		int SIZE_X = 23;
		int SIZE_Y = 16;
		resize(SIZE_X,SIZE_Y);
		if (error())
			return;

		CGlTexture a(GL_TEXTURE_2D, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT);

		a.bind();
		a.resize(SIZE_X,SIZE_Y);
		a.setNearestNeighborInterpolation();
		a.setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		a.setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		const GLuint color[4]={0,0,0,0};
		a.setParamIuiv(GL_TEXTURE_BORDER_COLOR, color);
		a.unbind();

		test_data = new GLint[SIZE_Y*SIZE_X*4];
		int sum = 0;
		for(int y = 0; y < SIZE_Y; y++)
			for(int x = 0; x < SIZE_X; x++)
				for (int i = 0; i < 4; i++)
				{
					int val = ((x+1)*(y+1)*(i+66))%10;
					test_data[(x+SIZE_X*y)*4+i] = val;
					sum += val;
				}

		std::cout << "------------------------ ORIGINAL DATA ------------------------" << std::endl;
		a.setData(test_data);
		a.printRGBA32UI(0);

		create(a);

		std::cout << "---------------------------------------------------------------" << std::endl;

		outputPyramids();

		std::cout << "gathered count: " << gathered_count << std::endl;

		delete [] test_data;
		exit(1);
	}

#endif
};

#endif /* CGLHISTOPYRAMIDRGBA_HPP_ */
