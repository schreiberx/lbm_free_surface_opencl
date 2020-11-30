#ifndef CGL_SHADER_BLINN_SINGLE_HPP
#define CGL_SHADER_BLINN_SINGLE_HPP

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlError.hpp"
#include "lib/CError.hpp"


/**
 * general blinn shader to use for rendering vertices
 *
 * the shader is initialized only once
 */
#include "libgl/core/CGlProgram.hpp"
#include "shaders/shader_blinn/CShaderBlinnSkeleton.hpp"


class CShaderBlinnSingle	:
	public CGlProgram,
	public CShaderBlinnSingleSkeleton
{

	// shaders to reuse
	static CGlShader vertShader;
	static CGlShader fragShaderSkel;
	static CGlShader fragShader;

	static bool shaders_loaded;

	static int usage_counter;

public:
	CGlUniform texture0_enabled;	///< uniform to enable and disable texturing
	CGlUniform vertex_color;		///< uniform to basic vertex color of fragment

	CShaderBlinnSingle()
	{
		std::string infoLog;

		if (!shaders_loaded)
		{
			vertShader.init(GL_VERTEX_SHADER);
			vertShader.loadSource(SHADER_GLSL_DEFAULT_DIR"shader_blinn/vertex_shader.glsl");
			vertShader.compile();

			fragShader.init(GL_FRAGMENT_SHADER);
			fragShader.loadSource(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader.glsl");
			fragShader.compile();

			fragShaderSkel.init(GL_FRAGMENT_SHADER);
			fragShaderSkel.loadSource(SHADER_GLSL_DEFAULT_DIR"shader_blinn/fragment_shader_skeleton.glsl");
			fragShaderSkel.compile();

			shaders_loaded = true;
		}

		this->attachShader(vertShader);
		this->attachShader(fragShader);
		this->attachShader(fragShaderSkel);

		std::ostringstream program_defines;
		program_defines << "#version 120" << std::endl;
		setSourcePrefix(program_defines.str());

		// link programs
		link();
		if (error())
		{
			std::string infoLog;
			getInfoLog(infoLog);
			error << "info Log: during linking: " << infoLog << std::endl;
			return;
		}

		this->setupUniform(texture0_enabled, "texture0_enabled");
		this->setupUniform(vertex_color, "vertex_color");

		initBlinnSkeleton(*this);
		usage_counter++;
	}

	~CShaderBlinnSingle()
	{
		usage_counter--;

		if (usage_counter == 0)
		{
			shaders_loaded = false;
			vertShader.freeIfValid();
			fragShader.freeIfValid();
			fragShaderSkel.freeIfValid();
		}
	}


	/**
	 * setup the light for rendering
	 */
	void setupLight(	CGlMaterial	&material,
						CGlLights &lights,
						const GLSL::vec3 &light_view_pos3
	)
	{
		CShaderBlinnSingleSkeleton::setupLight(material, lights, light_view_pos3);

		texture0_enabled.set1b(material.texture0 != NULL);
	}
};

// no shaders are initialized so far
CGlProgram::CGlShader CShaderBlinnSingle::vertShader;
CGlProgram::CGlShader CShaderBlinnSingle::fragShaderSkel;
CGlProgram::CGlShader CShaderBlinnSingle::fragShader;
bool CShaderBlinnSingle::shaders_loaded = false;
int CShaderBlinnSingle::usage_counter = 0;

#endif
