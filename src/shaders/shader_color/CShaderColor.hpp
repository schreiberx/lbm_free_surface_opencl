#ifndef CGL_SHADER_COLOR_HPP
#define CGL_SHADER_COLOR_HPP

#include "libgl/core/CGlError.hpp"
#include "lib/CError.hpp"



/**
 * shader for a single color without anything else
 */
#include "libgl/core/CGlProgram.hpp"


class CShaderColor	:
	public CGlProgram
{
	static CGlShader vertShader;
	static CGlShader fragShader;

	static bool shaders_loaded;
	static int usage_counter;

public:
	CGlUniform frag_color;	///< uniform to vec4 specifying fragment color
	CGlUniform pvm_matrix;	///< uniform to proj-view-model matrix


	CShaderColor()
	{
		std::string infoLog;

		if (!shaders_loaded)
		{
			try
			{
				vertShader.init(GL_VERTEX_SHADER);
				vertShader.loadSource(SHADER_GLSL_DEFAULT_DIR"shader_color/vertex_shader.glsl", prefix_string);
				vertShader.compile();

				fragShader.init(GL_FRAGMENT_SHADER);
				fragShader.loadSource(SHADER_GLSL_DEFAULT_DIR"shader_color/fragment_shader.glsl", prefix_string);
				fragShader.compile();
			}
			catch (std::exception &e)
			{
				std::ostringstream ss;
				ss << "Error in CShaderColor()" << std::endl;
				ss << e.what();

				throw std::runtime_error(ss.str());
			}

			shaders_loaded = true;
		}

		this->attachShader(vertShader);
		this->attachShader(fragShader);

		// link programs
		link();

		bindAttribLocation(0, "vertex_position");
		this->setupUniform(frag_color, "frag_color");
		this->setupUniform(pvm_matrix, "pvm_matrix");

		usage_counter++;
	}

	~CShaderColor()
	{
		usage_counter--;

		if (usage_counter == 0)
		{
			shaders_loaded = false;
			vertShader.freeIfValid();
			fragShader.freeIfValid();
		}
	}
};
#endif
