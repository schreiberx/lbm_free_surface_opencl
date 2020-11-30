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
 *  Created on: Feb 16, 2010
 *      Author: martin
 */

#ifndef CGLMARCHINGCUBES_VERTEX_ARRAY_RGBA_GLSL_HPP_
#define CGLMARCHINGCUBES_VERTEX_ARRAY_RGBA_GLSL_HPP_

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/core/CGlBuffer.hpp"
#include "libgl/core/CGlVertexArrayObject.hpp"
#include "lib/CFlatTextureLayout.hpp"
#include "libmath/CVector.hpp"
#include "shaders/shader_blinn/CShaderBlinnSkeleton.hpp"
#include "libgl/marching_cubes/CGlMarchingCubesSkeleton.hpp"
#include "libgl/histo_pyramid/CGlHistoPyramidRGBA.hpp"

#define MC_RGBA_BENCHMARK	0

// dont create buffer with vertex array
// read vertex positions from texture during rendering process
#define MC_RGBA_RENDER_VERTICES_VIA_TEXTURE	0

// create vertex array using transform feedback
#define MC_RGBA_TRANSFORM_FEEDBACK	1

// create vertex array by copying from texture to buffer
#define MC_RGBA_COPY_TO_BUFFER	0



/**
 * \brief create marching cubes data from OpenGL flat texture with OpenGL to render it with OpenGL vertex arrays.
 *
 * the normals are computed by the gradient field. the base flat texture has to set texture interpolation on.
 * this way, only the interpolation along the z coordinate has to be done manually.
 *
 * the triangles are rendered via a vertex array which is created using histo-pyramids
 */
class CGlMarchingCubesVertexArrayRGBA	:
		public CShaderBlinnSkeleton,
		public CGlMarchingCubesSkeleton
{
public:
	bool verbose;	///< verbose mode on/off

private:
	CGlTexture cTextureTriangleVertices;	///< shader array "parameter": texture with marching cubes triangle indices

	/*
	 * PREPROCESSING STEP #1: store mc indices and number of vertices
	 */
	CGlTexture cTextureMCIndices;					///< texture with marching cubes indices for each cell
	CGlTexture cTextureVertexCountHistoPyramid;		///< texture to create histoPyramid for vertex data
	CGlFbo cFboCreateMCIndicesAndCountVertices;
    CGlProgram cGlProgramCreateMCIndicesAndCountVertices;

    /*
     * PREPROCESSING STEP #2: create vertices and normals
     */
	CGlFbo cFboCreateVertexAndNormalArray;
	CGlProgram cGlProgramCreateVertexAndNormalArray;
 	CGlUniform vertex_fix_matrix4;
 	CGlUniform normal_fix_matrix3;


    /*
     * render marching cubes
     */
 	CGlUniform pvm_normal_matrix3;

	CGlVertexArrayObject vao_index;
	CGlBuffer index_buffer;

	CGlVertexArrayObject vao_quad_vertices;
	CGlBuffer quad_vertices_buffer;

#if MC_RGBA_TRANSFORM_FEEDBACK || MC_RGBA_COPY_TO_BUFFER
	size_t vertex_array_size;

	CGlVertexArrayObject vao_vertex_normal;
	CGlBuffer vertex_buffer;
	CGlBuffer normal_buffer;
#endif

#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE || MC_RGBA_COPY_TO_BUFFER
	size_t vertex_array_texture_width;
	size_t vertex_array_texture_height;
#endif


public:
    bool valid;		///< marching cubes are valid
	CError error;	///< error handler

	CGlHistoPyramidRGBA cGlHistoPyramidRGBA;	///< handler to histopyramid (specially the gathered_count variable)

    bool auto_expand_texture;			///< true, if vertex, normal and index texture are automatically resized

#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE || MC_RGBA_COPY_TO_BUFFER
    CGlTexture cTextureVertexArray;		///< texture with triangle vertex positions stored linearly
    CGlTexture cTextureNormalArray;		///< texture with gathered vertex normals
#endif

#if MC_RGBA_COPY_TO_BUFFER
	CGlFbo cFboVertexArray;		///< fbos to copy data from texture to
	CGlFbo cFboNormalArray;
#endif


	// layout for 2d flat texture
	CFlatTextureLayout	cFlatTextureLayout;	///< create and handle information about flat texture dimensions, etc.
	CGlProgram cGlProgramRender;			///< OpenGL program to render marching cubes with geometry shader

	CVector<3,int> domain_cells;			///< domain dimension of volumetric dataset

    /**
     * constructor for marching cubes
     */
	CGlMarchingCubesVertexArrayRGBA(bool p_verbose = false)	:
		verbose(p_verbose),

		cTextureTriangleVertices(			GL_TEXTURE_2D, GL_R32UI,	GL_RED_INTEGER,		GL_UNSIGNED_INT),

		cTextureMCIndices(					GL_TEXTURE_2D, GL_RGBA32UI,	GL_RGBA_INTEGER,	GL_UNSIGNED_INT),
		cTextureVertexCountHistoPyramid(	GL_TEXTURE_2D, GL_RGBA32UI,	GL_RGBA_INTEGER,	GL_UNSIGNED_INT)

#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE || MC_RGBA_COPY_TO_BUFFER
		,
		cTextureVertexArray(				GL_TEXTURE_2D, GL_RGBA32F,	GL_RGBA,			GL_FLOAT),
		cTextureNormalArray(				GL_TEXTURE_2D, GL_RGBA32F,	GL_RGBA,			GL_FLOAT)
#endif
	{
    	CGlErrorCheck();
    	valid = false;
	}

	/**
	 * reset the index buffer to the new size
	 *
	 * this is useful for adaptive MC vertices
	 */
	void resetIndexBuffer(GLuint new_size)
	{
		if (new_size == 0)
			new_size = 1;

		GLuint *indices = new GLuint[new_size];
		for (GLuint i = 0; i < new_size; i++)
			indices[i] = i;

		vao_index.bind();
			index_buffer.bind();
			index_buffer.data(sizeof(GLuint)*new_size, indices);
    		glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 0, 0);
    		glEnableVertexAttribArray(0);
		vao_index.unbind();

		delete [] indices;
	}

    /**
     * reset or initialize marching cubes creation by OpenGL programs
     *
     * \param p_domain_cells		domain size to run the MC algorithm on.
     *
     * \param vertex_array_texture_height_divisor	the height of the vertex array is divided by this value.
     * 												to make sure, that all vertices could be drawn, set this value to 1.
     * 												in most cases all vertices are drawn.
     *
     * 												set to -1 for automagic expansion
     */
    void reset(CVector<3,int> &p_domain_cells, int vertex_array_texture_height_divisor = -1)
    {
    	valid = false;

		domain_cells = p_domain_cells;

		// compute flat texture layout
		// this does not compute the real size. the real size has to be divided by 2 in each dimension!
		cFlatTextureLayout.init(domain_cells);

    	if (verbose)
    		std::cout << "flat texture: " << cFlatTextureLayout.ft_z_width/2 << "x" << cFlatTextureLayout.ft_z_height/2 << std::endl;


    	cGlHistoPyramidRGBA.resize(cFlatTextureLayout.ft_z_width/2, cFlatTextureLayout.ft_z_height/2);
		if (cGlHistoPyramidRGBA.error())
		{
			error << cGlHistoPyramidRGBA.error.getString();
			return;
		}
#if 0
    	if (domain_cells[0] > 128 || domain_cells[1] > 128 || domain_cells[2] > 128)
    	{
    		std::cout << "WARNING: domain sizes larger than 128 are disabled" << std::endl;
    		std::cout << "Remove this section from the class CGlMarchingCubesVertexArrayRGBA, if you still want to use it to create and render the MCs using this class." << std::endl;
			return;
    	}
#endif

    	/**
    	 * setup buffers
    	 */
		static const GLfloat quad_vertices[4][4] = {
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

		vao_quad_vertices.unbind();

    	/**
    	 * setup textures
    	 */

#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE || MC_RGBA_COPY_TO_BUFFER
		/*
		 * the vertex and normal array consist out of vertex positions with 3 vertex coordinates (stored in RGBA)
		 * and 3 vertices per triangle
		 *
		 * and a maximum of 15 triangles per cube
		 */

		vertex_array_texture_width = cFlatTextureLayout.ft_z_width*3;
		/*
		 * IMPORTANT! we divide the height of the vertex array texture by 16!
		 * we do this to save some memory
		 *
		 * it's possible, that this implementation is running out of
		 *
		 * TODO: adapt the texture height if more texels are needed
		 */

		if (vertex_array_texture_height_divisor == -1)
		{
			vertex_array_texture_height = 0;
			auto_expand_texture = true;
		}
		else
		{
			vertex_array_texture_height = cFlatTextureLayout.ft_z_height*15/vertex_array_texture_height_divisor;
			auto_expand_texture = false;
		}

    	GLint max_texture_size;
    	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

    	if (vertex_array_texture_width >= (size_t)max_texture_size)
    	{
    		std::cout << "WARNING: reducing vertex array texture width to " << max_texture_size << std::endl;
    		vertex_array_texture_width = (size_t)max_texture_size;
    	}

    	if (vertex_array_texture_height >= (size_t)max_texture_size)
    	{
    		std::cout << "WARNING: reducing vertex array texture height to " << max_texture_size << std::endl;
    		vertex_array_texture_height = (size_t)max_texture_size;
    	}

    	if (verbose)
    		std::cout << "vertex array texture: " << vertex_array_texture_width << "x" << vertex_array_texture_height << std::endl;

		cTextureVertexArray.bind();
		cTextureVertexArray.resize(vertex_array_texture_width, vertex_array_texture_height);
		cTextureVertexArray.unbind();
		CGlErrorCheck();

		cTextureNormalArray.bind();
		cTextureNormalArray.resize(vertex_array_texture_width, vertex_array_texture_height);
		cTextureNormalArray.unbind();
		CGlErrorCheck();

		resetIndexBuffer(vertex_array_texture_width*vertex_array_texture_height);
#endif

#if MC_RGBA_COPY_TO_BUFFER || MC_RGBA_TRANSFORM_FEEDBACK
		vertex_array_size = 0;
		resetIndexBuffer(0);
#endif

#if MC_RGBA_COPY_TO_BUFFER
		cFboVertexArray.bind();
		cFboVertexArray.bindTexture(cTextureVertexArray);
		cFboVertexArray.unbind();

		cFboNormalArray.bind();
		cFboNormalArray.bindTexture(cTextureNormalArray);
		cFboNormalArray.unbind();
#endif

		cTextureVertexCountHistoPyramid.bind();
		cTextureVertexCountHistoPyramid.resize(cFlatTextureLayout.ft_z_width/2, cFlatTextureLayout.ft_z_height/2);
		cTextureVertexCountHistoPyramid.setNearestNeighborInterpolation();
		cTextureVertexCountHistoPyramid.setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		cTextureVertexCountHistoPyramid.setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		const GLuint color[4]={0,0,0,0};
		cTextureVertexCountHistoPyramid.setParamIuiv(GL_TEXTURE_BORDER_COLOR, &color[0]);
		cTextureVertexCountHistoPyramid.unbind();

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
		program_defines << "#define TEXTURE_WIDTH	(" << (cFlatTextureLayout.ft_z_width/2) << ")" << std::endl;
		program_defines << "#define TEXTURE_HEIGHT	(" << (cFlatTextureLayout.ft_z_height/2) << ")" << std::endl;
		program_defines << "#define TEXTURE_PIXELS	(" << (cFlatTextureLayout.ft_z_elements/4) << ")" << std::endl;

#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE || MC_RGBA_COPY_TO_BUFFER
		program_defines << "#define VERTEX_ARRAY_TEXTURE_WIDTH	(" << vertex_array_texture_width << ")" << std::endl;
		program_defines << "#define VERTEX_ARRAY_TEXTURE_HEIGHT	(" << vertex_array_texture_height << ")" << std::endl;
#endif
		program_defines << "#define FT_Z_MOD		(" << cFlatTextureLayout.ft_z_mod << ")" << std::endl;
		program_defines << "#define ISO_VALUE		(0.5)" << std::endl;

		program_defines << "#define LAYERS	(" << cGlHistoPyramidRGBA.layers << ")" << std::endl;


		/*
		 * SHADER #1: create mc indices and number of triangle vertex indices for each cell
		 */
		cTextureMCIndices.bind();
		cTextureMCIndices.resize(cFlatTextureLayout.ft_z_width/2, cFlatTextureLayout.ft_z_height/2);
		cTextureMCIndices.setNearestNeighborInterpolation();
		cTextureMCIndices.unbind();

		cTextureTriangleVertices.bind();
		cTextureTriangleVertices.resize(5*3+1, 256);
		cTextureTriangleVertices.setNearestNeighborInterpolation();
		cTextureTriangleVertices.setData((void*)triangleVertices);
		cTextureTriangleVertices.unbind();

		cGlProgramCreateMCIndicesAndCountVertices.detachAndFreeShaders();
		cGlProgramCreateMCIndicesAndCountVertices.setSourcePrefix(program_defines.str());
		cGlProgramCreateMCIndicesAndCountVertices.initVertFragShadersFromDirectory("marching_cubes/vertex_array_rgba_create_mc_indices_and_vertex_counter");

		cGlProgramCreateMCIndicesAndCountVertices.link();

		{
			CGlProgramUse program_use(cGlProgramCreateMCIndicesAndCountVertices);
			cGlProgramCreateMCIndicesAndCountVertices.setUniform1i("fluid_fraction_texture", 0);
			cGlProgramCreateMCIndicesAndCountVertices.setUniform1i("triangle_vertices_texture", 1);
		}

		cFboCreateMCIndicesAndCountVertices.bind();
		cFboCreateMCIndicesAndCountVertices.bindTexture(cTextureMCIndices, 0);
		cFboCreateMCIndicesAndCountVertices.bindTexture(cTextureVertexCountHistoPyramid, 1);
		cFboCreateMCIndicesAndCountVertices.unbind();


		/*
		 * SHADER #2: create vertex and normal array
		 */

		/*
		 * create vertex array
		 *
		 * 3 vertices per triangle with 3 components
		 */
#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE || MC_RGBA_COPY_TO_BUFFER
		cFboCreateVertexAndNormalArray.bind();
		cFboCreateVertexAndNormalArray.bindTexture(cTextureVertexArray, 0);
		cFboCreateVertexAndNormalArray.bindTexture(cTextureNormalArray, 1);
		cFboCreateVertexAndNormalArray.unbind();
#endif

		cGlProgramCreateVertexAndNormalArray.detachAndFreeShaders();
		cGlProgramCreateVertexAndNormalArray.setSourcePrefix(program_defines.str());
#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE || MC_RGBA_COPY_TO_BUFFER
		cGlProgramCreateVertexAndNormalArray.initVertFragShadersFromDirectory("marching_cubes/vertex_array_rgba_create_vertex_and_normal_array");
#endif

#if MC_RGBA_TRANSFORM_FEEDBACK
		cGlProgramCreateVertexAndNormalArray.initVertFragShadersFromDirectory("marching_cubes/vertex_array_rgba_create_vertex_and_normal_array_transform_feedback");

		const GLchar *varyings[] = {"gl_Position", "out_normal"};
		glTransformFeedbackVaryings(	cGlProgramCreateVertexAndNormalArray.program,
										2,					// record 2 varyings
										varyings,			// varying strings
										GL_SEPARATE_ATTRIBS	// write each varying attribute to a separate buffer
								);
		CGlErrorCheck();
#endif
		cGlProgramCreateVertexAndNormalArray.link();

		{
			CGlProgramUse program_use(cGlProgramCreateVertexAndNormalArray);

			cGlProgramCreateVertexAndNormalArray.setUniform1i("fluid_fraction_texture", 0);
			cGlProgramCreateVertexAndNormalArray.setUniform1i("histo_pyramid_texture", 1);
			cGlProgramCreateVertexAndNormalArray.setUniform1i("mc_indices_texture", 2);
			cGlProgramCreateVertexAndNormalArray.setUniform1i("triangle_vertices_texture", 3);

			cGlProgramCreateVertexAndNormalArray.setupUniform(vertex_fix_matrix4, "vertex_fix_matrix4");
			cGlProgramCreateVertexAndNormalArray.setupUniform(normal_fix_matrix3, "normal_fix_matrix3");
		}


#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE
		if (verbose)
			std::cout << "vertex texture array: " << cTextureVertexArray.width << "x" << cTextureVertexArray.height << std::endl;

		/*
		 * RENDER marching cubes
		 */
		cGlProgramRender.detachAndFreeShaders();
		cGlProgramRender.setSourcePrefix(program_defines.str());
		cGlProgramRender.attachVertShader(SHADER_GLSL_DEFAULT_DIR"marching_cubes/vertex_array_rgba_render/vertex_shader.glsl");
		cGlProgramRender.attachFragShader(SHADER_GLSL_DEFAULT_DIR"marching_cubes/vertex_array_rgba_render/fragment_shader.glsl");
		cGlProgramRender.attachFragShader(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader_skeleton.glsl");

		cGlProgramRender.link();

		initBlinnSkeleton(cGlProgramRender);

		pvm_normal_matrix3.init(cGlProgramRender, "pvm_normal_matrix3");
		{
			CGlProgramUse program_use(cGlProgramRender);

			cGlProgramRender.setUniform1i("vertex_array_texture", 0);
			cGlProgramRender.setUniform1i("normal_array_texture", 1);
		}
#endif

#if MC_RGBA_COPY_TO_BUFFER || MC_RGBA_TRANSFORM_FEEDBACK
		cGlProgramRender.detachAndFreeShaders();
		cGlProgramRender.setSourcePrefix(program_defines.str());
		cGlProgramRender.attachVertShader(SHADER_GLSL_DEFAULT_DIR"marching_cubes/vertex_array_rgba_render_transform_feedback/vertex_shader.glsl");
		cGlProgramRender.attachFragShader(SHADER_GLSL_DEFAULT_DIR"marching_cubes/vertex_array_rgba_render_transform_feedback/fragment_shader.glsl");
		cGlProgramRender.attachFragShader(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader_skeleton.glsl");

		cGlProgramRender.link();

		initBlinnSkeleton(cGlProgramRender);

		cGlProgramRender.setupUniform(pvm_normal_matrix3, "pvm_normal_matrix3");
		{
			CGlProgramUse program_use(cGlProgramRender);

			cGlProgramRender.setUniform1i("vertex_position", 0);
			cGlProgramRender.setUniform1i("vertex_normal", 1);
		}
#endif
		CGlErrorCheck();
		valid = true;
    }


	/**
	 * prepare the marching cubes dataset for rendering
	 *
	 * \param flat_texture	2d flat texture with scalar values for marching cubes
	 */
    void prepare(CGlTexture &flat_texture)
    {
    	if (!valid)	return;
#if MC_RGBA_BENCHMARK
static CBenchmark b_("MC RGBA ( )");
//CBenchmark::deactivate();
b_.stop();
#endif

    	CGlViewport viewport;
		viewport.saveState();

		/*
		 * PREPROCESSING STEP #1: create 'MC indices' and 'number of vertices texture'
		 */
#if MC_RGBA_BENCHMARK
static CBenchmark b1("MC RGBA 1");
b1.start();
#endif
		vao_quad_vertices.bind();

		flat_texture.bind();
		glActiveTexture(GL_TEXTURE1);
		cTextureTriangleVertices.bind();

		cFboCreateMCIndicesAndCountVertices.bind();

    	{	CGlProgramUse program_use(cGlProgramCreateMCIndicesAndCountVertices);
    		viewport.setSize(cFlatTextureLayout.ft_z_width, cFlatTextureLayout.ft_z_height);


    		const GLenum draw_buffers_start[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    		const GLenum draw_buffers_finish[2] = {GL_COLOR_ATTACHMENT0, GL_NONE};

    		glDrawBuffers(2, draw_buffers_start);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glDrawBuffers(2, draw_buffers_finish);
    	}
		cFboCreateMCIndicesAndCountVertices.unbind();

		cTextureTriangleVertices.unbind();
		glActiveTexture(GL_TEXTURE0);
		flat_texture.unbind();

		vao_quad_vertices.unbind();

#if MC_RGBA_BENCHMARK
glFinish();
b1.stop();


static CBenchmark bhp("MC RGBA HP");
bhp.start();
#endif
    	/*
    	 * create histopyramid out of vertex count texture
    	 */
	cGlHistoPyramidRGBA.create(cTextureVertexCountHistoPyramid);

#if MC_RGBA_BENCHMARK
glFinish();
bhp.stop();
#endif
#if 0
		std::cout << "gathered_count: " << cGlHistoPyramidRGBA.gathered_count + 1 << std::endl;

		cGlHistoPyramidRGBA.cTexturePyramidUp.printR32UI(cGlHistoPyramidRGBA.layers-1);
		cGlHistoPyramidRGBA.cTexturePyramidDown.printR32UI(cGlHistoPyramidRGBA.layers-1);

		std::cout << std::endl;
		cGlHistoPyramidRGBA.cTexturePyramidUp.printR32UI(0);
		std::cout << std::endl;
		cGlHistoPyramidRGBA.cTexturePyramidDown.printR32UI(0);

		cGlHistoPyramidRGBA.cTexturePyramidUp.printR32UI(cGlHistoPyramidRGBA.layers-2);
		cGlHistoPyramidRGBA.cTexturePyramidDown.printR32UI(cGlHistoPyramidRGBA.layers-2);
#endif


#if LBM_OPENCL_FS_DEBUG
		if (cGlHistoPyramidRGBA.gathered_count < 0)
		{
			std::cerr << "FATAL ERROR (histopyramid for marching cubes): gathered_count = " << cGlHistoPyramidRGBA.gathered_count << std::endl;
			exit(-1);
		}

		if (cGlHistoPyramidRGBA.gathered_count >= (int)(cFlatTextureLayout.ft_z_width*15*cFlatTextureLayout.ft_z_height*3))
		{
			std::cerr << "FATAL ERROR (histopyramid for marching cubes): gathered_count " << cGlHistoPyramidRGBA.gathered_count << " > " << cFlatTextureLayout.ft_z_width*15*cFlatTextureLayout.ft_z_height*3 << std::endl;
			exit(-1);
		}
#endif

#if MC_RGBA_BENCHMARK
static CBenchmark b2("MC RGBA 2");
b2.start();
#endif
    	/*
    	 * PREPROCESSING STEP #2: create vertex and normal array
    	 */

#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE || MC_RGBA_COPY_TO_BUFFER
		/*
		 * check for necessary expansion (not all vertices fit into texture)
		 */

		if (cGlHistoPyramidRGBA.gathered_count / vertex_array_texture_width >= vertex_array_texture_height)
		{
			// TODO: check for maximum available texture height and output a warning
			// expand with more than 1 MB
			vertex_array_texture_height = cGlHistoPyramidRGBA.gathered_count / vertex_array_texture_width + (1024*1024/vertex_array_texture_width + 1);

			cTextureVertexArray.bind();
			cTextureVertexArray.resize(vertex_array_texture_width, vertex_array_texture_height);
			cTextureVertexArray.unbind();
			CGlErrorCheck();

			cTextureNormalArray.bind();
			cTextureNormalArray.resize(vertex_array_texture_width, vertex_array_texture_height);
			cTextureNormalArray.unbind();
			CGlErrorCheck();

			resetIndexBuffer(vertex_array_texture_width*vertex_array_texture_height);

#if MC_RGBA_COPY_TO_BUFFER
			vertex_array_size = vertex_array_texture_width * vertex_array_texture_height;

			vao_vertex_normal.bind();

				vertex_buffer.bind();
				vertex_buffer.resize(vertex_array_size*sizeof(GLfloat)*4, GL_DYNAMIC_DRAW);
				glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
				glEnableVertexAttribArray(0);

				normal_buffer.bind();
				normal_buffer.resize(vertex_array_size*sizeof(GLfloat)*4, GL_DYNAMIC_DRAW);
				glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL);
				glEnableVertexAttribArray(1);

			vao_vertex_normal.unbind();
#endif
		}
#endif


#if MC_RGBA_TRANSFORM_FEEDBACK
		if (cGlHistoPyramidRGBA.gathered_count > (int)vertex_array_size)
		{
			int m = 1024*32;	// default expansion size

			int extra_vertices = cGlHistoPyramidRGBA.gathered_count - vertex_array_size;

			vertex_array_size += (extra_vertices/m)*m;
			if (cGlHistoPyramidRGBA.gathered_count > (int)vertex_array_size)
				vertex_array_size += m;

			vao_vertex_normal.bind();
				vertex_buffer.bind();
				vertex_buffer.resize(vertex_array_size*sizeof(GLfloat)*4, GL_DYNAMIC_DRAW);
				glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
				glEnableVertexAttribArray(0);

				normal_buffer.bind();
				normal_buffer.resize(vertex_array_size*sizeof(GLfloat)*4, GL_DYNAMIC_DRAW);
				glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL);
				glEnableVertexAttribArray(1);
			vao_vertex_normal.unbind();

			resetIndexBuffer(vertex_array_size);
		}
#endif
		/*
		 * preprocessing step 2
		 */

#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE || MC_RGBA_COPY_TO_BUFFER
		vao_quad_vertices.bind();

		GLsizei va_height = cGlHistoPyramidRGBA.gathered_count / vertex_array_texture_width +
							int(cGlHistoPyramidRGBA.gathered_count % vertex_array_texture_width > 0);

    	{
    		CGlProgramUse program_use(cGlProgramCreateVertexAndNormalArray);

			float max = (float)cFlatTextureLayout.domain_cells.max();

			GLSL::mat4 vertex_fix = GLSL::scale(2.0f/max, 2.0f/max, 2.0f/max);

			vertex_fix *= GLSL::translate(	0.5f - (float)cFlatTextureLayout.domain_cells[0]*0.5f,
											0.5f - (float)cFlatTextureLayout.domain_cells[1]*0.5f,
											0.5f - (float)cFlatTextureLayout.domain_cells[2]*0.5f);

		 	vertex_fix_matrix4.set(vertex_fix);
		 	normal_fix_matrix3.set(GLSL::inverseTranspose(GLSL::mat3(vertex_fix)));

    		viewport.setSize(	vertex_array_texture_width,
								va_height
							);

    		const GLenum draw_buffers_start[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    		const GLenum draw_buffers_finish[2] = {GL_COLOR_ATTACHMENT0, GL_NONE};

			cFboCreateVertexAndNormalArray.bind();
			glDrawBuffers(2, draw_buffers_start);

			flat_texture.bind();

				glActiveTexture(GL_TEXTURE1);
				cGlHistoPyramidRGBA.cTexturePyramidDown.bind();

					glActiveTexture(GL_TEXTURE2);
					cTextureMCIndices.bind();

						glActiveTexture(GL_TEXTURE3);
						cTextureTriangleVertices.bind();

						glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

						cTextureTriangleVertices.unbind();
						glActiveTexture(GL_TEXTURE2);

					cTextureMCIndices.unbind();
					glActiveTexture(GL_TEXTURE1);

				cGlHistoPyramidRGBA.cTexturePyramidDown.unbind();
				glActiveTexture(GL_TEXTURE0);

			flat_texture.unbind();

			glDrawBuffers(2, draw_buffers_finish);
			cFboCreateVertexAndNormalArray.unbind();
    	}

#endif

#if MC_RGBA_TRANSFORM_FEEDBACK

		glEnable(GL_RASTERIZER_DISCARD);
/*
		GLuint query;
		glGenQueries(1, &query);
		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
*/
		{
    		CGlProgramUse program_use(cGlProgramCreateVertexAndNormalArray);

			float max = (float)cFlatTextureLayout.domain_cells.max();

			GLSL::mat4 vertex_fix = GLSL::scale(2.0f/max, 2.0f/max, 2.0f/max);

			vertex_fix *= GLSL::translate(	0.5f - (float)cFlatTextureLayout.domain_cells[0]*0.5f,
											0.5f - (float)cFlatTextureLayout.domain_cells[1]*0.5f,
											0.5f - (float)cFlatTextureLayout.domain_cells[2]*0.5f);

		 	vertex_fix_matrix4.set(vertex_fix);
		 	normal_fix_matrix3.set(GLSL::inverseTranspose(GLSL::mat3(vertex_fix)));

		 	vao_index.bind();

			CGlErrorCheck();
			vertex_buffer.bind(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

			CGlErrorCheck();
			normal_buffer.bind(GL_TRANSFORM_FEEDBACK_BUFFER, 1);
			CGlErrorCheck();

			flat_texture.bind();

				glActiveTexture(GL_TEXTURE1);
				cGlHistoPyramidRGBA.cTexturePyramidDown.bind();

					glActiveTexture(GL_TEXTURE2);
					cTextureMCIndices.bind();

						glActiveTexture(GL_TEXTURE3);
						cTextureTriangleVertices.bind();

			    		// begin with transform feedback after buffers are initialized and program is loaded
			    		// otherwise an error would be generated (according to GLspec)
						glBeginTransformFeedback(GL_TRIANGLES);
						glDrawArrays(GL_TRIANGLES, 0, cGlHistoPyramidRGBA.gathered_count);
						glEndTransformFeedback();

						cTextureTriangleVertices.unbind();
						glActiveTexture(GL_TEXTURE2);

					cTextureMCIndices.unbind();
					glActiveTexture(GL_TEXTURE1);

				cGlHistoPyramidRGBA.cTexturePyramidDown.unbind();
				glActiveTexture(GL_TEXTURE0);

			flat_texture.unbind();

			CGlErrorCheck();
			glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

//    		vertex_buffer.unbind(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
//    		normal_buffer.unbind(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
			CGlErrorCheck();
    	}
/*
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
		GLuint primitives_written;
		glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives_written);
		glDeleteQueries(1, &query);
		std::cout << primitives_written << " " << cGlHistoPyramidRGBA.gathered_count << std::endl;
*/
		glDisable(GL_RASTERIZER_DISCARD);
#endif


		// FINAL STEPS
		vao_quad_vertices.unbind();

#if MC_RGBA_COPY_TO_BUFFER
		/**
		 * copy the created vertex data from texture to the vertex array
		 *
		 * the viewport should be already correctly initialized
		 *
		 * BENCHMARK: reading directly from texture without framebuffer is much slower on current NVIDIA GPUs/Drivers
		 */
		cFboVertexArray.bind();
		glBindBuffer(GL_PIXEL_PACK_BUFFER, vertex_buffer.buffer);

			glReadPixels(	0,0,
							vertex_array_texture_width, va_height,
							GL_RGBA, GL_FLOAT, 0);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		cFboVertexArray.unbind();
		CGlErrorCheck();

		/**
		 * copy normal data
		 */
		cFboNormalArray.bind();
		glBindBuffer(GL_PIXEL_PACK_BUFFER, normal_buffer.buffer);

			glReadPixels(	0,0,
							vertex_array_texture_width, va_height,
							GL_RGBA, GL_FLOAT, 0);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		cFboNormalArray.unbind();
		CGlErrorCheck();

#endif

		// restore viewport
		viewport.restoreState();

#if MC_RGBA_BENCHMARK
glFinish();
b2.stop();

b_.start();
#endif

		CGlErrorCheck();
    }

    /**
     * render marching cubes polygons without program
     */
    void renderWithoutProgram()
    {
#if MC_RGBA_TRANSFORM_FEEDBACK || MC_RGBA_COPY_TO_BUFFER
    	vao_vertex_normal.bind();

		glDrawArrays(GL_TRIANGLES, 0, cGlHistoPyramidRGBA.gathered_count);

		vao_vertex_normal.unbind();
#else
		std::cout << "Rendering MC without program not supported using MC_RGBA_RENDER_VERTICES_VIA_TEXTURE" << std::endl;
#endif
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
#if MC_RGBA_BENCHMARK
static CBenchmark br("MC RGBA R");
br.start();
#endif

		CGlProgramUse program_use(cGlProgramRender);

    	setupUniformsMatrices(p_pvm_matrix, p_view_model_matrix, p_view_model_normal_matrix3);
		pvm_normal_matrix3.set(p_pvm_normal_matrix3);

#if MC_RGBA_RENDER_VERTICES_VIA_TEXTURE
		/*
		 * enable fake index buffer array because no vertices are emitted,
		 * if all attribute arrays are deactivated (this seems to be a bug in the NVIDIA driver for OpenGL 3.2)
		 */
		vao_index.bind();

			cTextureVertexArray.bind();

				glActiveTexture(GL_TEXTURE1);
				cTextureNormalArray.bind();

				CGlErrorCheck();

				glDrawArrays(GL_TRIANGLES, 0, cGlHistoPyramidRGBA.gathered_count);
				CGlErrorCheck();

				cTextureNormalArray.unbind();

			glActiveTexture(GL_TEXTURE0);
			cTextureVertexArray.unbind();

		vao_index.unbind();
#endif

#if MC_RGBA_TRANSFORM_FEEDBACK || MC_RGBA_COPY_TO_BUFFER
		vao_vertex_normal.bind();

		glDrawArrays(GL_TRIANGLES, 0, cGlHistoPyramidRGBA.gathered_count);

		vao_vertex_normal.unbind();
#endif


    	CGlErrorCheck();

#if MC_RGBA_BENCHMARK
if (br.outputReady())
	std::cout << "Triangles: " << cGlHistoPyramidRGBA.gathered_count/3 << std::endl << std::endl;

glFinish();
br.stop();
#endif
    }
};

#endif
