/*
 * CGlShader.hpp
 *
 *  Created on: Sep 6, 2011
 *      Author: schreibm
 */

#include <stdexcept>

#ifndef CGLSHADER_HPP_
#define CGLSHADER_HPP_

/**
 * \brief	load & compile GLSL vertex, geometry and fragment shaders
 */
class CGlShader
{
	GLuint shader;

public:
	std::string shader_source;
	std::string shader_prefix_string;
	std::string shader_filename;

	/**
	 * initialize with given type
	 */
	void init(GLenum type)
	{
		freeIfValid();

		shader = glCreateShader(type);
		CGlErrorCheck();
	}

	/**
	 * constructor with type
	 */
	CGlShader(GLenum type)
	{
		init(type);
	}

	/**
	 * initialize with existing shader
	 */
	CGlShader(const CGlShader &p_shader)
	{
		if (p_shader() != 0)
			std::cerr << "Shaders may not be initialized for copy constructor!!!" << CError::endl;
		shader = 0;
	}

	/**
	 * default constructor
	 */
	CGlShader()
	{
		shader = 0;
	}

	/**
	 * access OpenGL shader id
	 */
	inline GLuint operator()()	const
	{
		return shader;
	}

	/**
	 * load source
	 */
	bool loadSource(
			const std::string &i_filename,		///< filename with source
			const std::string &i_prefix_string	///< prefix to place before source (e. g. definitions)
	)
	{
		std::string file_content;

		CFile file;
		try
		{
			file.fileContents(i_filename, file_content);
		}
		catch (std::exception &e)
		{
			std::ostringstream ss;
			ss << "Error in 'loadSource' for file '" << i_filename << "' with prefix string '" << i_prefix_string << std::endl;
			ss << e.what();
			throw std::runtime_error(ss.str());
		}

		shader_source = i_prefix_string + file_content;
		shader_prefix_string = i_prefix_string;
		shader_filename = i_filename;

		const char *shader_source_array[1];
		shader_source_array[0] = shader_source.c_str();

		glShaderSource(shader, 1, shader_source_array, NULL);
		CGlErrorCheck();

		return true;
	}


	/**
	 * compile the shader
	 */
	bool compile()
	{
		glCompileShader(shader);
		CGlErrorCheck();

		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

		if (status == GL_TRUE)
			return true;

		throw std::runtime_error("compile(): Cannot compile shader");
		return false;
	}

	/**
	 * return the information log for the compilation
	 */
	bool getInfoLog(std::string &infoLog)
	{
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		if (length == 0)
		{
			// no info log available
			infoLog = "";
			return false;
		}

		GLchar *info_log_buf = new GLchar[length];

		// returned string is already zero terminated
		glGetShaderInfoLog(shader, length, NULL, info_log_buf);

		infoLog = info_log_buf;
		return true;
	}

	/**
	 * return the information log for the compilation
	 */
	std::string getInfoLog()
	{
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		if (length == 0)
			return std::string();

		GLchar *info_log_buf = new GLchar[length];

		// returned string is already zero terminated
		glGetShaderInfoLog(shader, length, NULL, info_log_buf);

		std::string ret_string = info_log_buf;
		return ret_string;
	}

	/**
	 * free the shader
	 */
	void free()
	{
		if (shader != 0)
		{
			glDeleteShader(shader);
			CGlErrorCheck();
			shader = 0;
		}
		else
		{
			std::cerr << "Warning: ~CGlShader: shader was not initialized!" << CError::endl;
		}
	}

	/**
	 * return true, if the shader was loaded or created
	 */
	bool loaded()
	{
		return shader != 0;
	}

	/**
	 * free and delete shader data
	 */
	void freeIfValid()
	{
		if (shader != 0)
		{
			glDeleteShader(shader);
			CGlErrorCheck();
			shader = 0;
		}
	}

	~CGlShader()
	{
		freeIfValid();
	}
};


#endif /* CGLSHADER_HPP_ */
