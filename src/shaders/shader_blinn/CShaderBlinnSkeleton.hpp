/*
 * CShaderBlinnSkeleton.hpp
 *
 *  Created on: Jan 6, 2010
 *      Author: martin
 */
/**
 * skeleton for classes including a blinn shader
 */
#ifndef CSHADERBLINNSKELETON_HPP_
#define CSHADERBLINNSKELETON_HPP_

#include "libgl/CGlLights.hpp"
#include "libgl/CGlMaterial.hpp"


/**
 * blinn shader skeleton to handle uniforms and their initialization
 */
class CShaderBlinnSkeleton
{
public:
	CGlUniform pvm_matrix_uniform;	///< uniform to projection-view-model matrix
	CGlUniform view_model_normal_matrix3_uniform;	///< uniform to view-model normal matrix
	CGlUniform view_model_matrix_uniform;			///< uniform to view-model matrix

	CGlUniform light0_enabled_uniform;			///< uniform to enable or disable uniforms
	CGlUniform light0_view_pos3_uniform;			///< uniform to position of light after applying view matrix

	CGlUniform light0_ambient_color3_uniform;		///< uniform to ambient color
	CGlUniform light0_diffuse_color3_uniform;		///< uniform to diffuse color
	CGlUniform light0_specular_color3_uniform;	///< uniform to specular color

	CGlUniform material_ambient_color3_uniform;	///< uniform to ambient color
	CGlUniform material_diffuse_color3_uniform;	///< uniform to diffuse color
	CGlUniform material_specular_color3_uniform;	///< uniform to specular color
	CGlUniform material_specular_exponent_uniform;	///< uniform to specular exponent

	/**
	 * initialize the blinn skeleton for a program implementing the necessary uniforms
	 */
	void initBlinnSkeleton(CGlProgram &program)
	{
		program.bindAttribLocation(0, "vertex_position");
		program.bindAttribLocation(1, "vertex_normal3");
		program.bindAttribLocation(2, "vertex_texture_coord2");

		program.setupUniform(light0_view_pos3_uniform, "light0_view_pos3");

		program.setupUniform(pvm_matrix_uniform, "pvm_matrix");
		program.setupUniform(view_model_matrix_uniform, "view_model_matrix");
		program.setupUniform(view_model_normal_matrix3_uniform, "view_model_normal_matrix3");

		program.setupUniform(light0_enabled_uniform, "light0_enabled");
		program.setupUniform(light0_view_pos3_uniform, "light0_view_pos3");

		program.setupUniform(light0_ambient_color3_uniform, "light0_ambient_color3");
		program.setupUniform(light0_diffuse_color3_uniform, "light0_diffuse_color3");
		program.setupUniform(light0_specular_color3_uniform, "light0_specular_color3");

		program.setupUniform(material_ambient_color3_uniform, "material_ambient_color3");
		program.setupUniform(material_diffuse_color3_uniform, "material_diffuse_color3");
		program.setupUniform(material_specular_color3_uniform, "material_specular_color3");
		program.setupUniform(material_specular_exponent_uniform, "material_specular_exponent");
	}


	void setupUniformsMatrices(	const GLSL::mat4 &pvm_matrix,
								const GLSL::mat4 &view_model_matrix,
								const GLSL::mat3 &view_model_normal_matrix3
	)
	{
		pvm_matrix_uniform.set(pvm_matrix);
		view_model_matrix_uniform.set(view_model_matrix);
		view_model_normal_matrix3_uniform.set(view_model_normal_matrix3);
	}


	/**
	 * setup the material uniforms
	 */
	void setupUniformsMaterial(CGlMaterial	&material)
	{
		material_ambient_color3_uniform.set(material.ambient_color3);
		material_diffuse_color3_uniform.set(material.diffuse_color3);
		material_specular_color3_uniform.set(material.specular_color3);
		material_specular_exponent_uniform.set1f(material.specular_exponent);
	}

	/**
	 * setup the light uniforms
	 */
	void setupUniformsLights(	const CGlLights &lights,
								const GLSL::vec3 &light_view_pos3
	)
	{
		light0_enabled_uniform.set1b(lights.light0_enabled);
		light0_view_pos3_uniform.set(light_view_pos3);

		light0_ambient_color3_uniform.set(lights.light0_ambient_color3);
		light0_diffuse_color3_uniform.set(lights.light0_diffuse_color3);
		light0_specular_color3_uniform.set(lights.light0_specular_color3);
	}

	/**
	 * setup the light and material uniforms
	 */
	void setupUniforms(	CGlMaterial	&material,
						CGlLights &lights,
						const GLSL::vec3 &light_view_pos3
	)
	{
		setupUniformsLights(lights, light_view_pos3);
		setupUniformsMaterial(material);
	}
};


#endif /* CSHADERBLINNSKELETON_HPP_ */
