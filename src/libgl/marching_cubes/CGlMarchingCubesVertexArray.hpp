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
 * CGlMarchingCubesVertexArray.hpp
 *
 *  Created on: Jan 19, 2010
 *      Author: martin
 */

#ifndef CGLMARCHINGCUBES_VERTEX_ARRAY_GLSL_HPP_
#define CGLMARCHINGCUBES_VERTEX_ARRAY_GLSL_HPP_

#include "libgl/core/CGlTexture.hpp"
#include "lib/CFlatTextureLayout.hpp"
#include "libmath/CVector.hpp"
#include "shaders/shader_blinn/CShaderBlinnSkeleton.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/marching_cubes/CGlMarchingCubesSkeleton.hpp"
#include "libgl/histo_pyramid/CGlHistoPyramid.hpp"

/**
 * \brief create marching cubes data from OpenGL volumetric texture with OpenGL for OpenGL rendering.
 *
 * the normals are created from the gradient field.
 *
 * the triangles are rendered via a vertex array which is created using histo-pyramids
 */
class CGlMarchingCubesVertexArray	:
		public CShaderBlinnSkeleton,
		public CGlMarchingCubesSkeleton
{

    // create mc index data and vertex count
	CGlFbo cFboCreateMCIndices;
    CGlProgram cGlProgramCreateMCIndices;

	// create vertex coordinates
    CGlFbo cFboCreateVertices;
    CGlProgram cGlProgramCreateVertices;

    // create vertex array
	CGlFbo cFboCreateVertexArray;
    CGlProgram cGlProgramCreateVertexArray;
    GLsizei vertex_array_texture_width;
    GLsizei vertex_array_texture_height;

	CGlVertexArrayObject vao_index;
	CGlBuffer index_buffer;

	CGlVertexArrayObject vao_quad_vertices;
	CGlBuffer quad_vertices_buffer;

public:
    // render marching cubes
 	CGlUniform pvm_normal_matrix3;	///< the normal matrix for rendering

	CGlHistoPyramid cGlHistoPyramid;		///< histo pyramid using only the red color channel.
											///< information like the gathered count can be read from this class

    bool verbose;	///< verbose mode on/off
    bool valid;		///< marching cubes are valid
	CError error;	///< error handler

    CGlTexture cTextureVertexArray;			///< array with texture coordinates to the texture cTextureVertices
	CGlTexture cTextureMCIndices;			///< texture with marching cubes indices
	CGlTexture cTextureTriangleVertices;	///< texture with marching cubes triangle indices

	CGlTexture cTextureVertices;			///< texture with triangle vertex positions
	CGlTexture cTextureNormals;				///< texture with gathered vertex normals
	CGlTexture cTextureVertexHistoPyramidBase;	///< texture to create histoPyramid for vertex data

	CFlatTextureLayout	cFlatTextureLayout;	///< create and handle information about flat texture dimensions, etc.

    CGlProgram cGlProgramRender;			///< OpenGL program to render marching cubes with geometry shader
	CVector<3,int> domain_cells;			///< domain dimension of volumetric dataset

	/**
	 * constructor for marching cubes
	 */
	CGlMarchingCubesVertexArray(bool p_verbose = false)	:
		verbose(p_verbose),
		cTextureVertexArray(		GL_TEXTURE_2D, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT),
		cTextureMCIndices(			GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT),
		cTextureTriangleVertices(	GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT),
		cTextureVertices(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
		cTextureNormals(			GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, GL_FLOAT),
		cTextureVertexHistoPyramidBase(	GL_TEXTURE_2D, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT)
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
			std::cout << "WARNING: FLAT TEXTURE LAYOUT TOO LARGE (" << cFlatTextureLayout.ft_z_width*3 << "x" << cFlatTextureLayout.ft_z_height << ") => disabling CGlMarchingCubesVertexArray" << std::endl;
			return;
    	}
#if 1
    	if (domain_cells[0] > 128 || domain_cells[1] > 256 || domain_cells[2] > 128)
    	{
    		std::cout << "WARNING: domain sizes larger than 128 are disabled (and also not supported due to oversized vertex texture) because cGlMarchingCubesVertexArrayRGBA is the preferred class." << std::endl;
    		std::cout << "Remove this section from the class CGlMarchingCubesVertexArray, if you still want to use it to create and render the MCs using this class." << std::endl;
			return;
    	}
#endif
    	cGlHistoPyramid.resize(cFlatTextureLayout.ft_z_width, cFlatTextureLayout.ft_z_height);
		if (cGlHistoPyramid.error())
		{
			error << cGlHistoPyramid.error.getString();
			return;
		}

    	/**
    	 * setup buffers
    	 */
		GLint *indices = new GLint[domain_cells.elements()*15];
		for (int i = 0; i < domain_cells.elements()*15; i++)
			indices[i] = i;

		vao_index.bind();
			index_buffer.bind();
			index_buffer.data(sizeof(GLint)*domain_cells.elements()*15, indices);
			glVertexAttribIPointer(0, 1, GL_INT, 0, 0);
			glEnableVertexAttribArray(0);
		vao_index.unbind();

		delete [] indices;

		const GLfloat quad_vertices[4][4] = {
					{-1.0, -1.0, 0.0, 1.0},
					{ 1.0, -1.0, 0.0, 1.0},
					{-1.0,  1.0, 0.0, 1.0},
					{ 1.0,  1.0, 0.0, 1.0},
				};

		vao_quad_vertices.bind();
			quad_vertices_buffer.bind();
			quad_vertices_buffer.data(sizeof(quad_vertices), quad_vertices);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(0);
		vao_quad_vertices.unbind();

    	/**
    	 * setup textures
    	 */
		cTextureMCIndices.bind();
		cTextureMCIndices.resize(cFlatTextureLayout.ft_z_width, cFlatTextureLayout.ft_z_height);
		cTextureMCIndices.setNearestNeighborInterpolation();
		cTextureMCIndices.unbind();

		cTextureVertices.bind();
		cTextureVertices.resize(cFlatTextureLayout.ft_z_width*3, cFlatTextureLayout.ft_z_height);
		cTextureVertices.setNearestNeighborInterpolation();
		cTextureVertices.unbind();

		cTextureNormals.bind();
		cTextureNormals.resize(cFlatTextureLayout.ft_z_width*3, cFlatTextureLayout.ft_z_height);
		cTextureNormals.setNearestNeighborInterpolation();
		cTextureNormals.unbind();

		cTextureTriangleVertices.bind();
		cTextureTriangleVertices.resize(5*3+1, 256);
		cTextureTriangleVertices.setNearestNeighborInterpolation();
		cTextureTriangleVertices.setData((void*)triangleVertices);
		cTextureTriangleVertices.unbind();

		cTextureVertexHistoPyramidBase.bind();
		cTextureVertexHistoPyramidBase.resize(cFlatTextureLayout.ft_z_width, cFlatTextureLayout.ft_z_height);
		cTextureVertexHistoPyramidBase.setNearestNeighborInterpolation();
		cTextureVertexHistoPyramidBase.setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		cTextureVertexHistoPyramidBase.setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		const GLuint color[4]={0,0,0,0};
		cTextureVertexHistoPyramidBase.setParamIuiv(GL_TEXTURE_BORDER_COLOR, &color[0]);
		cTextureVertexHistoPyramidBase.unbind();

		// we have to store a maximum of 15 vertices for every cube
		vertex_array_texture_width = cFlatTextureLayout.ft_z_width*4;
		vertex_array_texture_height = cFlatTextureLayout.ft_z_height*4;

		if (verbose)
			std::cout << "vertex texture array: " << vertex_array_texture_width << "x" << vertex_array_texture_height << std::endl;

		/**
		 * setup shader programs & frame buffer objects
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
		program_defines << "#define VERTEX_ARRAY_TEXTURE_WIDTH	(" << vertex_array_texture_width << ")" << std::endl;
		program_defines << "#define ISO_VALUE		(0.5)" << std::endl;


		/*
		 * create mc indices and number of triangle vertex indices for each cell
		 */
		cGlProgramCreateMCIndices.detachAndFreeShaders();
		cGlProgramCreateMCIndices.setSourcePrefix(program_defines.str());
		cGlProgramCreateMCIndices.initVertFragShadersFromDirectory("marching_cubes/vertex_array_create_mc_indices_and_vertex_counter");

		cGlProgramCreateMCIndices.link();

		{
			CGlProgramUse program_use(cGlProgramCreateMCIndices);
			cGlProgramCreateMCIndices.setUniform1i("fluid_fraction_texture", 0);
			cGlProgramCreateMCIndices.setUniform1i("triangle_vertices_texture", 1);
		}

		cFboCreateMCIndices.bind();
		cFboCreateMCIndices.bindTexture(cTextureMCIndices, 0);
		cFboCreateMCIndices.bindTexture(cTextureVertexHistoPyramidBase, 1);
		cFboCreateMCIndices.unbind();


		/*
		 * create vertex coordinates
		 */
		cGlProgramCreateVertices.detachAndFreeShaders();
		cGlProgramCreateVertices.setSourcePrefix(program_defines.str());
		cGlProgramCreateVertices.initVertFragShadersFromDirectory("marching_cubes/vertex_array_create_vertices");

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

		/*
		 * create vertex array
		 */

		cGlProgramCreateVertexArray.detachAndFreeShaders();
		cGlProgramCreateVertexArray.setSourcePrefix(program_defines.str());
		cGlProgramCreateVertexArray.initVertFragShadersFromDirectory("marching_cubes/vertex_array_create_vertex_array");

		cGlProgramCreateVertexArray.link();

		{	CGlProgramUse program_use(cGlProgramCreateVertexArray);

			cGlProgramCreateVertexArray.setUniform1i("histoPyramid", 0);
			cGlProgramCreateVertexArray.setUniform1i("mc_indices_texture", 1);
			cGlProgramCreateVertexArray.setUniform1i("triangle_vertices_texture", 2);
			cGlProgramCreateVertexArray.setUniform1i("layers", cGlHistoPyramid.layers);
		}

		cTextureVertexArray.bind();
		cTextureVertexArray.resize(vertex_array_texture_width, vertex_array_texture_height);
		cTextureVertexArray.unbind();
		CGlErrorCheck();

		cFboCreateVertexArray.bind();
		cFboCreateVertexArray.bindTexture(cTextureVertexArray);
		cFboCreateVertexArray.unbind();
		CGlErrorCheck();


		/*
		 * render marching cubes
		 */
		cGlProgramRender.detachAndFreeShaders();
		cGlProgramRender.setSourcePrefix(program_defines.str());
		cGlProgramRender.attachVertShader(SHADER_GLSL_DEFAULT_DIR"marching_cubes/vertex_array_render/vertex_shader.glsl");
		cGlProgramRender.attachFragShader(SHADER_GLSL_DEFAULT_DIR"marching_cubes/vertex_array_render/fragment_shader.glsl");
		cGlProgramRender.attachFragShader(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader_skeleton.glsl");

		cGlProgramRender.link();

		initBlinnSkeleton(cGlProgramRender);

		cGlProgramRender.setupUniform(pvm_normal_matrix3, "pvm_normal_matrix3");
		{
			CGlProgramUse program_use(cGlProgramRender);

			cGlProgramRender.setUniform1i("vertex_array_texture", 0);
			cGlProgramRender.setUniform1i("vertices_texture", 1);
			cGlProgramRender.setUniform1i("normals_texture", 2);
		}

		CGlErrorCheck();

		valid = true;
    }


    /**
     * create vertex array for rendering out of a volume texture
     *
     * \param volume_texture	3d volume texture
     */
    void prepare(CGlTexture &volume_texture)
    {
    	if (!valid)	return;

    	CGlViewport viewport;
		viewport.saveState();

		/*
		 * create 'MC indices' and 'number of vertices texture'
		 */

		vao_quad_vertices.bind();

		volume_texture.bind();
		glActiveTexture(GL_TEXTURE1);
		cTextureTriangleVertices.bind();

		cFboCreateMCIndices.bind();

    	{	CGlProgramUse program_use(cGlProgramCreateMCIndices);
    		viewport.setSize(cFlatTextureLayout.ft_z_width, cFlatTextureLayout.ft_z_height);

    		const GLenum draw_buffers_start[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    		const GLenum draw_buffers_finish[2] = {GL_COLOR_ATTACHMENT0, GL_NONE};

    		glDrawBuffers(2, draw_buffers_start);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glDrawBuffers(2, draw_buffers_finish);
    	}
		cFboCreateMCIndices.unbind();

		cTextureTriangleVertices.unbind();
		glActiveTexture(GL_TEXTURE0);
		volume_texture.unbind();

		vao_quad_vertices.unbind();

    	/*
    	 * create histopyramid out of vertex count texture
    	 */
		cGlHistoPyramid.create(cTextureVertexHistoPyramidBase);
#if 0
		std::cout << "gathered_count: " << cGlHistoPyramid.gathered_count + 1 << std::endl;

		cGlHistoPyramid.cTexturePyramidUp.printR32UI(cGlHistoPyramid.layers-1);
		cGlHistoPyramid.cTexturePyramidDown.printR32UI(cGlHistoPyramid.layers-1);

		std::cout << std::endl;
		cGlHistoPyramid.cTexturePyramidUp.printR32UI(0);
		std::cout << std::endl;
		cGlHistoPyramid.cTexturePyramidDown.printR32UI(0);

		cGlHistoPyramid.cTexturePyramidUp.printR32UI(cGlHistoPyramid.layers-2);
		cGlHistoPyramid.cTexturePyramidDown.printR32UI(cGlHistoPyramid.layers-2);
#endif

		if (cGlHistoPyramid.gathered_count < 0 ||
			cGlHistoPyramid.gathered_count >= vertex_array_texture_width*vertex_array_texture_height)
		{
			std::cerr << "FATAL ERROR (histopyramid for marching cubes)" << std::endl;
			exit(-1);
		}


		/*
		 * create vertex list
		 */
		// only for debugging purposes: clear vertex array list
#if 0
		cFboCreateVertexArray.bind();
		viewport.setSize(vertex_array_texture_width, vertex_array_texture_height);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);
		cFboCreateVertexArray.unbind();
#endif

		vao_quad_vertices.bind();

		cGlHistoPyramid.cTexturePyramidDown.bind();
		glActiveTexture(GL_TEXTURE1);
		cTextureMCIndices.bind();
		glActiveTexture(GL_TEXTURE2);
		cTextureTriangleVertices.bind();

    	{	CGlProgramUse program_use(cGlProgramCreateVertexArray);
			cFboCreateVertexArray.bind();

			int viewport_width = vertex_array_texture_width;
			int viewport_height = cGlHistoPyramid.gathered_count/vertex_array_texture_width + (cGlHistoPyramid.gathered_count % vertex_array_texture_width != 0);

			glClearColor(0,0,1,0);
			glClear(GL_COLOR_BUFFER_BIT);
//			std::cout << "viewport: " << viewport_width << "x" << viewport_height << std::endl;

			viewport.setSize(viewport_width, viewport_height);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    		cFboCreateVertexArray.unbind();
    	}
    	CGlErrorCheck();
		cTextureTriangleVertices.unbind();
		glActiveTexture(GL_TEXTURE1);
		cTextureMCIndices.unbind();
		glActiveTexture(GL_TEXTURE0);
		cGlHistoPyramid.cTexturePyramidDown.unbind();

		vao_quad_vertices.unbind();

#if 0
		std::cout << std::endl;
		cTextureVertexArray.printRG32UI();
		std::cout << std::endl;
		cGlHistoPyramid.cTexturePyramidDown.printR32UI(0);
#endif

    	/*
    	 * create vertex positions and normals
    	 */

		vao_quad_vertices.bind();

		volume_texture.bind();

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

    /**
     * render marching cubes polygons
     */
    void render(	const GLSL::mat4 &p_pvm_matrix,				///< pvm
					const GLSL::mat3 &p_pvm_normal_matrix3,		///< invTransp(pvm)
					const GLSL::mat4 &p_view_model_matrix,		///< view*model
					const GLSL::mat3 &p_view_model_normal_matrix3	///< invTransp(view*model)
    		)
    {
    	if (!valid)	return;

    	CGlProgramUse program_use(cGlProgramRender);

    	setupUniformsMatrices(p_pvm_matrix, p_view_model_matrix, p_view_model_normal_matrix3);
		pvm_normal_matrix3.set(p_pvm_normal_matrix3);

		/*
		 * enable fake index buffer array because no vertices are emitted,
		 * if all attribute arrays are deactivated (this is a bug in the NVIDIA driver)
		 */

		vao_index.bind();

			cTextureVertexArray.bind();

				glActiveTexture(GL_TEXTURE1);
				cTextureVertices.bind();

					glActiveTexture(GL_TEXTURE2);
					cTextureNormals.bind();

						CGlErrorCheck();

						glDrawArrays(GL_TRIANGLES, 0, cGlHistoPyramid.gathered_count);
						CGlErrorCheck();

					cTextureNormals.unbind();

				glActiveTexture(GL_TEXTURE1);
				cTextureVertices.unbind();

			glActiveTexture(GL_TEXTURE0);
			cTextureVertexArray.unbind();

		vao_index.unbind();

    	CGlErrorCheck();
    }
};

#endif
