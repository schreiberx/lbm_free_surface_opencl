#ifndef CGL_SHADER_BLINN_HPP
#define CGL_SHADER_BLINN_HPP

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlError.hpp"
#include "lib/CError.hpp"


/**
 * general blinn shader to use for rendering vertices
 */
#include "libgl/core/CGlProgram.hpp"
#include "shaders/shader_blinn/CShaderBlinnSkeleton.hpp"


class CShaderBlinn	:
	public CGlProgram,
	public CShaderBlinnSkeleton
{
public:
	CGlUniform texture0_enabled;	///< uniform to enable and disable texturing
	CGlUniform vertex_color;		///< uniform to basic vertex color of fragment

	CShaderBlinn()
	{
		std::string infoLog;

		try
		{
			initVertFragShadersFromDirectory("shader_blinn");

			attachFragShader(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader_skeleton.glsl");
		}
		catch (std::exception &e)
		{
			std::ostringstream ss;
			ss << "Error in CShaderBlinn()" << std::endl;
			ss << e.what();
			throw std::runtime_error(ss.str());
		}

		std::ostringstream program_defines;
		program_defines << "#version 150" << std::endl;
		setSourcePrefix(program_defines.str());

		// link programs
		link();

		this->setupUniform(texture0_enabled, "texture0_enabled");
		this->setupUniform(vertex_color, "vertex_color");

		initBlinnSkeleton(*this);

	}

	~CShaderBlinn()
	{
	}


	/**
	 * setup the uniforms for rendering
	 */
	void setupUniforms(	CGlMaterial	&material,
						CGlLights &lights,
						const GLSL::vec3 &light_view_pos3
	)
	{
		CShaderBlinnSkeleton::setupUniforms(material, lights, light_view_pos3);

		texture0_enabled.set1b(material.texture0 != NULL);
	}


	/**
	 * setup the uniforms for rendering
	 */
	void setupUniformsMaterial(	CGlMaterial	&material	)
	{
		CShaderBlinnSkeleton::setupUniformsMaterial(material);

		texture0_enabled.set1b(material.texture0 != NULL);
	}
};


#endif
