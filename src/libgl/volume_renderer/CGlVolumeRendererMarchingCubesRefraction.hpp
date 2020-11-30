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


#ifndef CGL_VOLUME_RENDERER_MARCHING_CUBES_REFRACTION_HPP
#define CGL_VOLUME_RENDERER_MARCHING_CUBES_REFRACTION_HPP

#include "libgl/draw/CGlDrawVolumeBox.hpp"
#include "libgl/core/CGlProgram.hpp"
#include "libgl/core/CGlBuffer.hpp"
#include "libgl/core/CGlFbo.hpp"
#include "libgl/core/CGlViewport.hpp"
#include "lib/CError.hpp"
#include "libmath/CGlSlMath.hpp"
#include "libgl/marching_cubes/CGlMarchingCubesGeometryShader.hpp"
#include "libgl/marching_cubes/CGlMarchingCubesPreprocessing.hpp"

/**
 * \brief OpenGL volume renderer using marching cubes and refractions
 */
class CGlVolumeRendererMarchingCubesRefraction	:
	public CShaderBlinnSkeleton
{
private:
	CGlDrawVolumeBox volume_box;
	CGlViewport viewport;

	CGlUniform pvm_matrix_uniform;			///< uniform for pvm matrix
	CGlUniform model_matrix_uniform;			///< uniform for model matrix
	CGlUniform model_normal_matrix3_uniform;	///< uniform for inv(transp(model)) matrix
	CGlUniform view_matrix_uniform;			///< uniform for view matrix
	CGlUniform view_model_matrix_uniform;				///< uniform for view*model
	CGlUniform view_model_normal_matrix3_uniform;		///< uniform for inv(transp(view*model)) matrix

	CGlUniform texture_size_uniform;			///< uniform for texture size
	CGlUniform inv_texture_size_uniform;		///< uniform for inverse of texture size
	CGlUniform texture_size_add_one_uniform;		///< uniform for texture_size+vec3(1)
	CGlUniform inv_texture_size_add_one_uniform;	///< uniform for inverse of texture_size+vec3(1)

	CGlUniform step_length_uniform;			///< uniform for step length
	CGlUniform gradient_delta_uniform;		///< uniform for displacements to compute gradient for normals

	CGlUniform water_reflectance_at_normal_incidence_uniform;		///< spectral distribution function of the fresnel factor at normal incidence

public:
	CGlProgram cGlProgram;		///< volume rendering program

	CGlUniform refraction_index_uniform;		///< uniform for refraction index

	CGlUniform domain_cells_uniform;			///< uniform to number of domain cells
	CGlUniform domain_cell_elements_uniform;	///< uniform to number of domain elements
	CGlUniform texture_width_uniform;			///< uniform to flat texture width
	CGlUniform texture_height_uniform;		///< uniform to flat texture height
	CGlUniform ft_z_mod_uniform;				///< uniform to flat texture z mod

public:
	CError error;		///< error handler


	/**
	 * initialize volume rendering
	 */
	CGlVolumeRendererMarchingCubesRefraction()
	{
		cGlProgram.initVertFragShadersFromDirectory("volume_renderer/marching_cubes_refraction");
		cGlProgram.attachFragShader(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader_skeleton_volume.glsl");

		cGlProgram.bindAttribLocation(0, "vertex_position");
		cGlProgram.bindAttribLocation(1, "vertex_texture_coord");

		cGlProgram.link();

		initBlinnSkeleton(cGlProgram);

		cGlProgram.setupUniform(pvm_matrix_uniform, "pvm_matrix");
		cGlProgram.setupUniform(view_matrix_uniform, "view_matrix");
		cGlProgram.setupUniform(view_model_matrix_uniform, "view_model_matrix");
		cGlProgram.setupUniform(model_matrix_uniform, "model_matrix");
		cGlProgram.setupUniform(view_model_normal_matrix3_uniform, "view_model_normal_matrix3");
		cGlProgram.setupUniform(model_normal_matrix3_uniform, "model_normal_matrix3");

		cGlProgram.setupUniform(gradient_delta_uniform, "gradient_delta");
		cGlProgram.setupUniform(step_length_uniform, "step_length");

		cGlProgram.setupUniform(refraction_index_uniform, "refraction_index");

		cGlProgram.setupUniform(texture_size_uniform, "texture_size");
		cGlProgram.setupUniform(inv_texture_size_uniform, "inv_texture_size");
		cGlProgram.setupUniform(texture_size_add_one_uniform, "texture_size_add_one");
		cGlProgram.setupUniform(inv_texture_size_add_one_uniform, "inv_texture_size_add_one");

		cGlProgram.setupUniform(domain_cells_uniform, "domain_cells");
		cGlProgram.setupUniform(domain_cell_elements_uniform, "domain_cell_elements");

		cGlProgram.setupUniform(texture_width_uniform, "texture_width");
		cGlProgram.setupUniform(texture_height_uniform, "texture_height");

		cGlProgram.setupUniform(ft_z_mod_uniform, "ft_z_mod");

		cGlProgram.setupUniform(water_reflectance_at_normal_incidence_uniform, "water_reflectance_at_normal_incidence");

		cGlProgram.use();
		cGlProgram.setUniform1i("mc_indices_texture", 0);
		cGlProgram.setUniform1i("vertices_texture", 1);
		cGlProgram.setUniform1i("normals_texture", 2);
		cGlProgram.setUniform1i("triangle_vertices_texture", 3);
		cGlProgram.setUniform1i("cube_map", 4);
		refraction_index_uniform.set1f(1.33333);
		cGlProgram.disable();

		CGlErrorCheck();
	}


	/**
	 * start OpenGL volume rendering
	 */
	void render(
			GLSL::mat4 &pvm_matrix,					///< pvm matrix
			GLSL::mat4 &view_matrix,					///< view matrix
			GLSL::mat4 &model_matrix,				///< model matrix
			GLSL::mat3 &model_normal_matrix3,		///< inv(transp(model)) matrix
			GLSL::mat4 &view_model_matrix,			///< view*model matrix
			GLSL::mat3 &view_model_normal_matrix3,	///< inv(transp(view*model)) matrix

			CGlMarchingCubesPreprocessing &mc,		///< class for marching cubes
			CGlTexture &cube_map,					///< cube map texture
			float gradientFactor = 0.5f,			///< factor to compute gradient
			float refraction_index = 1.33333,		///< refraction index
			float step_size = 0.5f,					///< step size (unused)
			float water_reflectance_at_normal_incidence = 0.0	///< reflected light from view dir parallel to surface normal
		)
	{
		float gradient_delta[3];
		gradient_delta[0] = gradientFactor/(float)mc.domain_cells[0];
		gradient_delta[1] = gradientFactor/(float)mc.domain_cells[1];
		gradient_delta[2] = gradientFactor/(float)mc.domain_cells[2];

		CGlProgramUse program_use(cGlProgram);
		CGlStateEnable depth_test_enable(GL_DEPTH_TEST);

		pvm_matrix_uniform.set(pvm_matrix);
		view_matrix_uniform.set(view_matrix);
		model_matrix_uniform.set(model_matrix);
		view_model_matrix_uniform.set(view_model_matrix);
		view_model_normal_matrix3_uniform.set(view_model_normal_matrix3);
		model_normal_matrix3_uniform.set(model_normal_matrix3);
		water_reflectance_at_normal_incidence_uniform.set1f(water_reflectance_at_normal_incidence);

		step_length_uniform.set1f(step_size);
		gradient_delta_uniform.set3fv(gradient_delta);

		refraction_index_uniform.set1f(refraction_index);

		GLfloat texture_size[3];
		texture_size[0] = mc.domain_cells[0];
		texture_size[1] = mc.domain_cells[1];
		texture_size[2] = mc.domain_cells[2];
		texture_size_uniform.set3fv(texture_size);

		GLfloat inv_texture_size[3];
		inv_texture_size[0] = 1.0f/texture_size[0];
		inv_texture_size[1] = 1.0f/texture_size[1];
		inv_texture_size[2] = 1.0f/texture_size[2];
		inv_texture_size_uniform.set3fv(inv_texture_size);

		GLfloat texture_size_add_one[3];
		texture_size_add_one[0] = (float)mc.domain_cells[0]+1.0f;
		texture_size_add_one[1] = (float)mc.domain_cells[1]+1.0f;
		texture_size_add_one[2] = (float)mc.domain_cells[2]+1.0f;
		texture_size_add_one_uniform.set3fv(texture_size_add_one);

		GLfloat inv_texture_size_add_one[3];
		inv_texture_size_add_one[0] = 1.0f/texture_size_add_one[0];
		inv_texture_size_add_one[1] = 1.0f/texture_size_add_one[1];
		inv_texture_size_add_one[2] = 1.0f/texture_size_add_one[2];
		inv_texture_size_add_one_uniform.set3fv(inv_texture_size_add_one);

		GLint domain_cells[3];
		domain_cells[0] = mc.domain_cells.data[0];
		domain_cells[1] = mc.domain_cells.data[1];
		domain_cells[2] = mc.domain_cells.data[2];
		domain_cells_uniform.set3iv(domain_cells);

		texture_width_uniform.set1i(mc.cFlatTextureLayout.ft_z_width);
		texture_height_uniform.set1i(mc.cFlatTextureLayout.ft_z_height);

		ft_z_mod_uniform.set1i(mc.cFlatTextureLayout.ft_z_mod);

		mc.cTextureMCIndices.bind();

		glActiveTexture(GL_TEXTURE1);
		mc.cTextureVertices.bind();

    	glActiveTexture(GL_TEXTURE2);
    	mc.cTextureNormals.bind();

        glActiveTexture(GL_TEXTURE3);
        mc.cTextureTriangleVertices.bind();

		glActiveTexture(GL_TEXTURE4);
		cube_map.bind();

		volume_box.renderWithoutProgram();

		cube_map.unbind();
		glActiveTexture(GL_TEXTURE3);

		mc.cTextureTriangleVertices.bind();
        glActiveTexture(GL_TEXTURE2);

        mc.cTextureNormals.unbind();
  	    glActiveTexture(GL_TEXTURE1);

  	    mc.cTextureVertices.unbind();
    	glActiveTexture(GL_TEXTURE0);

    	mc.cTextureMCIndices.unbind();

		CGlErrorCheck();
	}
};

#endif
