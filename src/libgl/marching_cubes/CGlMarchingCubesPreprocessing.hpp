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
 * CMarchingCubesPreprocessing.hpp
 *
 *  Created on: Mar 28, 2010
 *      Author: martin
 */

#ifndef CGLMARCHINGCUBES_PREPROCESSING_GLSL_HPP_
#define CGLMARCHINGCUBES_PREPROCESSING_GLSL_HPP_


#include "libgl/core/CGlTexture.hpp"
#include "lib/CFlatTextureLayout.hpp"
#include "libmath/CVector.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/marching_cubes/CGlMarchingCubesSkeleton.hpp"

/**
 * \brief create marching cubes data from OpenGL volumetric texture with OpenGL for OpenGL rendering.
 *
 * the normals are created from the gradient field.
 *
 * the triangles are rendered using the geometry shaders.
 */
class CGlMarchingCubesPreprocessing	:
		public CGlMarchingCubesSkeleton
{
    CGlFbo cFboCreateVertices;
    CGlProgram cGlProgramCreateVertices;

	CGlFbo cFboCreateMCIndices;
    CGlProgram cGlProgramCreateMCIndices;

    CGlVertexArrayObject vao_quad_vertices;
	CGlBuffer quad_vertices_buffer;

public:
    bool verbose;	///< verbose mode on/off
    bool valid;		///< marching cubes are valid
	CError error;	///< error handler

	CGlTexture cTextureMCIndices;			///< texture with marching cubes indices
	CGlTexture cTextureTriangleVertices;	///< texture with marching cubes triangle indices
	CGlTexture cTextureVertices;			///< texture with triangle vertex positions
	CGlTexture cTextureNormals;				///< texture with gathered vertex normals

	CFlatTextureLayout	cFlatTextureLayout;	///< create and handle information about flat texture dimensions, etc.

	CVector<3,int> domain_cells;			///< domain dimension of volumetric dataset

    /**
     * constructor for marching cubes
     */
	CGlMarchingCubesPreprocessing(bool p_verbose = false)	:
		verbose(p_verbose),
		cTextureMCIndices(			GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT),
		cTextureTriangleVertices(	GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT),
		cTextureVertices(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
		cTextureNormals(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT)
	{
    	CGlErrorCheck();
    	valid = false;
	}

    /**
     * reset or initialize marching cubes creation by OpenGL programs
     */
    void reset(CVector<3,int> &p_domain_cells)
    {
    	valid = false;

		domain_cells = p_domain_cells;

    	cFlatTextureLayout.init(domain_cells);

    	if (verbose)
    		std::cout << "flat texture: " << cFlatTextureLayout.ft_z_width << "x" << cFlatTextureLayout.ft_z_height << std::endl;

    	GLint max_texture_size;
    	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

    	if (cFlatTextureLayout.ft_z_width*3 >= max_texture_size || cFlatTextureLayout.ft_z_height >= max_texture_size)
    	{
			std::cout << "WARNING: FLAT TEXTURE LAYOUT TOO LARGE (" << cFlatTextureLayout.ft_z_width*3 << "x" << cFlatTextureLayout.ft_z_height << ") => disabling CGlMarchingCubesGeometryShaderVertexArray" << std::endl;
			return;
    	}

    	if (domain_cells[0] > 128 || domain_cells[1] > 128 || domain_cells[2] > 128)
    	{
    		std::cout << "WARNING: domain sizes larger than 128 are disabled because cGlMarchingCubesVertexArrayRGBA is the preferred class." << std::endl;
    		std::cout << "Remove this section from the class CGlMarchingCubesGeometryShader, if you still want to use it to create and render the MCs using this class." << std::endl;
			return;
    	}
		/*******************************************
		 * OpenGL stuff
		 *******************************************/

		const GLfloat quad_vertices[4][4] = {
					{-1.0, -1.0, 0.0, 1.0},
					{ 1.0, -1.0, 0.0, 1.0},
					{-1.0,  1.0, 0.0, 1.0},
					{ 1.0,  1.0, 0.0, 1.0},
				};

		vao_quad_vertices.bind();
			quad_vertices_buffer.bind();
			quad_vertices_buffer.data(sizeof(quad_vertices), quad_vertices);

			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);
//		quad_vertices_buffer.unbind();

		vao_quad_vertices.unbind();


    	/**
    	 * setup textures
    	 */
		// allocate textures
		cTextureMCIndices.bind();
		cTextureMCIndices.resize(cFlatTextureLayout.ft_z_width, cFlatTextureLayout.ft_z_height);
		cTextureMCIndices.setNearestNeighborInterpolation();
		cTextureMCIndices.unbind();
		CGlErrorCheck();

		cTextureVertices.bind();
		cTextureVertices.resize(cFlatTextureLayout.ft_z_width*3, cFlatTextureLayout.ft_z_height);
		cTextureVertices.setNearestNeighborInterpolation();
		cTextureVertices.unbind();
		CGlErrorCheck();

		cTextureNormals.bind();
		cTextureNormals.resize(cFlatTextureLayout.ft_z_width*3, cFlatTextureLayout.ft_z_height);
		cTextureNormals.setNearestNeighborInterpolation();
		cTextureNormals.unbind();
		CGlErrorCheck();

		cTextureTriangleVertices.bind();
		cTextureTriangleVertices.resize(5*3+1, 256);
		cTextureTriangleVertices.setNearestNeighborInterpolation();
		cTextureTriangleVertices.setData((void*)triangleVertices);
		cTextureTriangleVertices.unbind();
		CGlErrorCheck();


		/**
		 * setup shader programs & fbos
		 */
		std::ostringstream program_defines;
		program_defines << "#version 150" << std::endl;
		program_defines << "#define DOMAIN_CELLS_X	(" << domain_cells[0] << ")" << std::endl;
		program_defines << "#define DOMAIN_CELLS_Y	(" << domain_cells[1] << ")" << std::endl;
		program_defines << "#define DOMAIN_CELLS_Z	(" << domain_cells[2] << ")" << std::endl;

		program_defines << "#define INV_DOMAIN_CELLS_X	(" << (1.0f/(float)domain_cells[0]) << ")" << std::endl;
		program_defines << "#define INV_DOMAIN_CELLS_Y	(" << (1.0f/(float)domain_cells[1]) << ")" << std::endl;
		program_defines << "#define INV_DOMAIN_CELLS_Z	(" << (1.0f/(float)domain_cells[2]) << ")" << std::endl;
		program_defines << "#define INV_05_DOMAIN_CELLS_X	(" << (0.5f/(float)domain_cells[0]) << ")" << std::endl;
		program_defines << "#define INV_05_DOMAIN_CELLS_Y	(" << (0.5f/(float)domain_cells[1]) << ")" << std::endl;
		program_defines << "#define INV_05_DOMAIN_CELLS_Z	(" << (0.5f/(float)domain_cells[2]) << ")" << std::endl;
		program_defines << "#define INV_DOMAIN_CELLS_X_ADD_ONE	(" << (1.0f/((float)domain_cells[0]+1.0f)) << ")" << std::endl;
		program_defines << "#define INV_DOMAIN_CELLS_Y_ADD_ONE	(" << (1.0f/((float)domain_cells[1]+1.0f)) << ")" << std::endl;
		program_defines << "#define INV_DOMAIN_CELLS_Z_ADD_ONE	(" << (1.0f/((float)domain_cells[2]+1.0f)) << ")" << std::endl;

		program_defines << "#define DOMAIN_CELLS	(" << domain_cells.elements() << ")" << std::endl;
		program_defines << "#define TEXTURE_WIDTH	(" << (cFlatTextureLayout.ft_z_width) << ")" << std::endl;
		program_defines << "#define TEXTURE_HEIGHT	(" << (cFlatTextureLayout.ft_z_height) << ")" << std::endl;
		program_defines << "#define TEXTURE_PIXELS	(" << (cFlatTextureLayout.ft_z_elements) << ")" << std::endl;
		program_defines << "#define FT_Z_MOD		(" << cFlatTextureLayout.ft_z_mod << ")" << std::endl;
		program_defines << "#define ISO_VALUE		(0.5)" << std::endl;


		cGlProgramCreateMCIndices.detachAndFreeShaders();
		cGlProgramCreateMCIndices.setSourcePrefix(program_defines.str());
		cGlProgramCreateMCIndices.initVertFragShadersFromDirectory("marching_cubes/preprocessing_create_mc_indices");

		cGlProgramCreateMCIndices.link();

		{
			CGlProgramUse program_use(cGlProgramCreateMCIndices);

			cGlProgramCreateMCIndices.setUniform1i("fluid_fraction_texture", 0);
		}

		cFboCreateMCIndices.bind();
		cFboCreateMCIndices.bindTexture(cTextureMCIndices, 0);
		cFboCreateMCIndices.unbind();

		cGlProgramCreateVertices.detachAndFreeShaders();
		cGlProgramCreateVertices.setSourcePrefix(program_defines.str());
		cGlProgramCreateVertices.initVertFragShadersFromDirectory("marching_cubes/preprocessing_create_vertices");

		cGlProgramCreateVertices.link();

		{
			CGlProgramUse program_use(cGlProgramCreateVertices);

			cGlProgramCreateVertices.setUniform1i("fluid_fraction_texture", 0);
			cGlProgramCreateVertices.setUniform1i("mc_indices_texture", 1);
		}

		cFboCreateVertices.bind();
		cFboCreateVertices.bindTexture(cTextureVertices, 0);
		cFboCreateVertices.bindTexture(cTextureNormals, 1);
		cFboCreateVertices.unbind();

		CGlErrorCheck();

		valid = true;
    }

    /**
     * prepare the marching cubes dataset for rendering
     */
    void prepare(CGlTexture &volume_texture)
    {
    	if (!valid)	return;

    	CGlViewport viewport;
		viewport.saveState();

		vao_quad_vertices.bind();

		volume_texture.bind();

    	{
    		CGlProgramUse program_use(cGlProgramCreateMCIndices);
    		viewport.setSize(cFlatTextureLayout.ft_z_width, cFlatTextureLayout.ft_z_height);

    		cFboCreateMCIndices.bind();

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			cFboCreateMCIndices.unbind();
    	}

    	{
    		CGlProgramUse program_use(cGlProgramCreateVertices);
    		viewport.setSize(cFlatTextureLayout.ft_z_width*3, cFlatTextureLayout.ft_z_height);

			cFboCreateVertices.bind();

			glActiveTexture(GL_TEXTURE1);
			cTextureMCIndices.bind();

    		const GLenum draw_buffers_start[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    		const GLenum draw_buffers_finish[2] = {GL_COLOR_ATTACHMENT0, GL_NONE};

			glDrawBuffers(2, draw_buffers_start);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glDrawBuffers(2, draw_buffers_finish);

			cTextureMCIndices.unbind();
			glActiveTexture(GL_TEXTURE0);

			cFboCreateVertices.unbind();
    	}

		volume_texture.unbind();

		vao_quad_vertices.unbind();

		viewport.restoreState();
    }
};

#endif
