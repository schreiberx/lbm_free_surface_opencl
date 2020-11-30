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
 * CMarchingCubesGeomShader.hpp
 *
 * VERSION USING GEOMETRY SHADER
 *
 *  Created on: Jan 3, 2010
 *      Author: martin
 */

#ifndef CGLMARCHINGCUBES_GEOM_SHADER_GLSL_HPP_
#define CGLMARCHINGCUBES_GEOM_SHADER_GLSL_HPP_


#include "libgl/core/CGlTexture.hpp"
#include "lib/CFlatTextureLayout.hpp"
#include "libmath/CVector.hpp"
#include "shaders/shader_blinn/CShaderBlinnSkeleton.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/marching_cubes/CGlMarchingCubesSkeleton.hpp"
#include "libgl/marching_cubes/CGlMarchingCubesPreprocessing.hpp"

/**
 * \brief create marching cubes data from OpenGL volumetric texture with OpenGL for OpenGL rendering.
 *
 * the normals are created from the gradient field.
 *
 * the triangles are rendered using the geometry shaders.
 */
class CGlMarchingCubesGeometryShader	:
		public CShaderBlinnSkeleton,
		public CGlMarchingCubesPreprocessing
{
	CGlUniform pvm_normal_matrix3;
	CGlBuffer index_buffer;
	CGlVertexArrayObject vao;

public:
    CGlProgram cGlProgramRender;			///< OpenGL program to render marching cubes with geometry shader

    /**
     * constructor for marching cubes
     */
	CGlMarchingCubesGeometryShader(bool p_verbose = false)	:
		CGlMarchingCubesPreprocessing(p_verbose)
	{
    	CGlErrorCheck();
    	valid = false;
	}

    /**
     * reset or initialize marching cubes creation by OpenGL programs
     */
    void reset(CVector<3,int> &p_domain_cells)
    {
    	CGlMarchingCubesPreprocessing::reset(p_domain_cells);

    	valid = CGlMarchingCubesPreprocessing::valid;
    	if (!valid)	return;

		GLint *indices = new GLint[cFlatTextureLayout.ft_z_elements];
		for (int i = 0; i < cFlatTextureLayout.ft_z_elements; i++)
			indices[i] = i;

		vao.bind();
			index_buffer.bind();
			index_buffer.data(sizeof(GLint)*cFlatTextureLayout.ft_z_elements, indices);

			glVertexAttribIPointer(0, 1, GL_INT, 0, NULL);
			glEnableVertexAttribArray(0);
		vao.unbind();

		CGlErrorCheck();
		glFinish();
		delete [] indices;

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

		cGlProgramRender.detachAndFreeShaders();
		cGlProgramRender.setSourcePrefix(program_defines.str());
		cGlProgramRender.attachVertShader(SHADER_GLSL_DEFAULT_DIR"marching_cubes/geometry_shader_render/vertex_shader.glsl");
		cGlProgramRender.attachGeomShader(SHADER_GLSL_DEFAULT_DIR"marching_cubes/geometry_shader_render/geometry_shader.glsl");
		cGlProgramRender.attachFragShader(SHADER_GLSL_DEFAULT_DIR"marching_cubes/geometry_shader_render/fragment_shader.glsl");
		cGlProgramRender.attachFragShader(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader_skeleton.glsl");

		cGlProgramRender.link();

		initBlinnSkeleton(cGlProgramRender);

		cGlProgramRender.setupUniform(pvm_normal_matrix3, "pvm_normal_matrix3");
		{
			CGlProgramUse program_use(cGlProgramRender);

			cGlProgramRender.setUniform1i("mc_indices_texture", 0);
			cGlProgramRender.setUniform1i("vertices_texture", 1);
			cGlProgramRender.setUniform1i("normals_texture", 2);
			cGlProgramRender.setUniform1i("triangle_vertices_texture", 3);	// texture with triangle table
		}

		CGlErrorCheck();

		valid = true;
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
		 * enable fake index buffer array because no vertices are emitted, if all attribarrays are deactivated
		 */

		vao.bind();

			cTextureMCIndices.bind();

			glActiveTexture(GL_TEXTURE1);
			cTextureVertices.bind();

			glActiveTexture(GL_TEXTURE2);
			cTextureNormals.bind();

			glActiveTexture(GL_TEXTURE3);
			cTextureTriangleVertices.bind();
			CGlErrorCheck();

			glDrawArrays(GL_POINTS, 0, cFlatTextureLayout.ft_z_elements);
			CGlErrorCheck();

			cTextureTriangleVertices.bind();

			glActiveTexture(GL_TEXTURE2);
			cTextureNormals.unbind();

			glActiveTexture(GL_TEXTURE1);
			cTextureVertices.unbind();

			glActiveTexture(GL_TEXTURE0);
			cTextureMCIndices.unbind();


		vao.unbind();

    	CGlErrorCheck();
    }
};

#endif
