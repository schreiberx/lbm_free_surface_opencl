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
 * CGlVolumeTextureToFlat.hpp
 *
 *  Created on: Mar 24, 2010
 *      Author: martin
 */

#ifndef CGLVOLUMETEXTURETOFLAT_HPP_
#define CGLVOLUMETEXTURETOFLAT_HPP_

#include "libgl/core/CGlProgram.hpp"
#include "libgl/core/CGlBuffer.hpp"
#include "libgl/core/CGlVertexArrayObject.hpp"
#include "lib/CFlatTextureLayout.hpp"

/**
 * after a 3d dataset is copied from opencl to an opengl 2d texture, convert the 2d texture to
 * flat texture layout
 */
class CGlVolumeTextureToFlat
{
public:
	CError error;

private:
	CGlProgram cGlProgram;
	CGlViewport cGlViewport;
	CGlBuffer quad_vertices_buffer;
	CGlVertexArrayObject vao;
	CGlFbo cGlFbo;

public:

	CGlVolumeTextureToFlat()
	{

	}


	void setup	(	CGlTexture &fluid_fraction_flat_texture,
					CFlatTextureLayout &cFlatTextureLayout
				)
	{
		// setup viewport and clear flat texture with 0 values
		cGlViewport.saveState();
		cGlViewport.setSize(fluid_fraction_flat_texture.width, fluid_fraction_flat_texture.height);

			cGlFbo.bind();
			cGlFbo.bindTexture(fluid_fraction_flat_texture);

				glClearColor(0,0,0,0);
				glClear(GL_COLOR_BUFFER_BIT);
			cGlFbo.unbind();

		cGlViewport.restoreState();
		CGlErrorCheck();

		std::ostringstream program_defines;
		program_defines << "#version 150" << std::endl;
		program_defines << "#define TEXTURE_WIDTH	(" << (cFlatTextureLayout.ft_z_width >> 1) << ")" << std::endl;
		program_defines << "#define TEXTURE_HEIGHT	(" << (cFlatTextureLayout.ft_z_height >> 1) << ")" << std::endl;

		program_defines << "#define DOMAIN_CELLS_X	(" << cFlatTextureLayout.domain_cells[0] << ")" << std::endl;
		program_defines << "#define DOMAIN_CELLS_Y	(" << cFlatTextureLayout.domain_cells[1] << ")" << std::endl;
		program_defines << "#define DOMAIN_CELLS_Z	(" << cFlatTextureLayout.domain_cells[2] << ")" << std::endl;

		program_defines << "#define FT_Z_MOD		(" << cFlatTextureLayout.ft_z_mod << ")" << std::endl;


		/*
		 * GL PROGRAM: RAW TO FLAT
		 */
		cGlProgram.setSourcePrefix(program_defines.str());
		cGlProgram.initVertFragShadersFromDirectory("texture_volume_to_flat");

		cGlProgram.link();

		cGlProgram.bindAttribLocation(0, "vertex_position");
		cGlProgram.use();
		cGlProgram.setUniform1i("fluid_fraction_volume_texture", 0);
		cGlProgram.disable();

		vao.bind();

			/*
			 * prepare quad buffer
			 */
			const GLfloat quad_vertices[4][4] = {
						{-1.0, -1.0, 0.0, 1.0},
						{ 1.0, -1.0, 0.0, 1.0},
						{-1.0,  1.0, 0.0, 1.0},
						{ 1.0,  1.0, 0.0, 1.0},
					};

			quad_vertices_buffer.bind();
			quad_vertices_buffer.data(sizeof(quad_vertices), quad_vertices);

			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);

			quad_vertices_buffer.unbind();

		vao.unbind();
	}


	void convert(	CGlTexture &volume_texture,
					CGlTexture &fluid_fraction_flat_texture
			)
	{
		/*
		 * convert raw fluid fraction texture to flat texture layout
		 */

		vao.bind();
/*
		quad_vertices_buffer.bind();
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);
*/

		cGlFbo.bind();

			cGlViewport.saveState();
			cGlViewport.setSize(fluid_fraction_flat_texture.width, fluid_fraction_flat_texture.height);

			{
				CGlProgramUse program_use(cGlProgram);

				// convert to flat fluid fraction texture
				volume_texture.bind();
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				volume_texture.unbind();
			}

			cGlViewport.restoreState();

		cGlFbo.unbind();
/*
		quad_vertices_buffer.unbind();
		glDisableVertexAttribArray(0);
*/
		vao.unbind();
	}
};

#endif /* CGLRAWVOLUMETEXTURETOFLAT_HPP_ */
