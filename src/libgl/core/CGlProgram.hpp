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


#ifndef C_GL_PROGRAM_HPP
#define C_GL_PROGRAM_HPP

#include "libmath/CGlSlMath.hpp"
#include "CGlError.hpp"

#include "lib/CFile.hpp"
#include <list>
#include <string>
#include <algorithm>
#include "lib/CError.hpp"
#include "libgl/shaders/CDefaultShaderDir.hpp"


#include "libgl/core/CGlUniform.hpp"
#include "libgl/core/CGlShader.hpp"

/**
 * \brief GLSL program abstraction class
 */
class CGlProgram
{
public:
	GLuint program;					///< OpenGL program ID

	std::list<CGlShader> shaders;	///< list to attached shaders

	std::string shaders_dir;	///< for debug purposes
	std::string prefix_string;	///< string to place before the source code

#if LBM_OPENCL_FS_DEBUG
	int prefix_string_lines;	///< lines for prefix string to help debugging the code
#endif

	//CError error;	///< error handler

	/**
	 * set a string which is used as a prefix for every sourcefile
	 */
	void setSourcePrefix(const std::string &i_prefix_string)
	{
		prefix_string = i_prefix_string;

#if LBM_OPENCL_FS_DEBUG
		prefix_string_lines = std::count(prefix_string.begin(), prefix_string.end(), '\n');
#endif
	}

	/**
	 * initialize and allocate OpenGL program handler
	 */
	void init()
	{
#if LBM_OPENCL_FS_DEBUG
		prefix_string_lines = 0;
#endif

		// Always prefix with this version
		setSourcePrefix("#version 150\n");

		program = glCreateProgram();
		CGlErrorCheck();
	}

	CGlProgram()
	{
		init();
	}

	/**
	 * attach shader to OpenGL program
	 */
	void attachShader(CGlShader &shader)
	{
		glAttachShader(program, shader());
		CGlErrorCheck();
	}

	/**
	 * detach and free shader if valid
	 */
	void detachAndFreeShaders()
	{
		while (!shaders.empty())
		{
			CGlShader &shader = shaders.front();
			glDetachShader(program, shader());
			shaders.pop_front();
		}
	}

	/**
	 * load shader from file and attach shader to program
	 */
	void attachShader(const char *shader_file, GLuint type)
	{
		// insert new shader at begin of texture
		shaders.push_front(CGlShader());

		CGlShader &new_shader = shaders.front();

		try
		{
			new_shader.init(type);
			new_shader.loadSource(shader_file, prefix_string);
		}
		catch (std::exception &e)
		{
			shaders.pop_front();

			std::ostringstream ss;
			ss << "Error while attaching shader for file '" << shader_file << "'" << std::endl;
			ss << e.what() << std::endl;

			throw std::runtime_error(ss.str());
		}

		try
		{
			new_shader.compile();
		}
		catch (std::exception &e)
		{
			std::ostringstream ss;
			ss << "Error while compiling shader for file '" << shader_file << "'" << std::endl;

			ss << "Source: \"\"\"" << std::endl;
			ss << new_shader.shader_source << std::endl;
			ss << "\"\"\"" << std::endl;

			ss << "InfoLog: \"\"\"" << std::endl;
			ss << new_shader.getInfoLog() << std::endl;
			ss << "\"\"\"" << std::endl;

			ss << e.what() << std::endl;

			shaders.pop_front();

			throw std::runtime_error(ss.str());
		}

		glAttachShader(program, new_shader());
	}

	/**
	 * attach vertex shader loaded from shader_file
	 */
	void attachVertShader(const char *shader_file)
	{
		attachShader(shader_file, GL_VERTEX_SHADER);
	}

	/**
	 * attach geometry shader loaded from shader_file
	 */
	void attachGeomShader(const char *shader_file)
	{
		attachShader(shader_file, GL_GEOMETRY_SHADER);
	}

	/**
	 * attach fragment shader loaded from shader_file
	 */
	void attachFragShader(const char *shader_file)
	{
		attachShader(shader_file, GL_FRAGMENT_SHADER);
	}

	/**
	 * initialize and attach vertex and fragment shaders from file
	 */
	void initVertFragShaders(
				const char *vert_shader_file,
				const char *frag_shader_file
	)
	{
		std::string infoLog;

		detachAndFreeShaders();

		attachVertShader(vert_shader_file);

		attachFragShader(frag_shader_file);
	}


	/**
	 * initialize and attach geometry, vertex and fragment shaders from file
	 */
	void initGeomVertFragShaders(
				const char *vert_shader_file,
				const char *geom_shader_file,
				const char *frag_shader_file
	)
	{
		std::string infoLog;
		detachAndFreeShaders();

		attachVertShader(vert_shader_file);

		attachGeomShader(geom_shader_file);

		attachFragShader(frag_shader_file);
	}

	/**
	 * initialize and attach vertex and fragment shaders from directory assuming that
	 * vertex shader is given in [directory]/vertex_shader.glsl and
	 * fragment shader is given in [directory]/fragment_shader.glsl
	 */
	void initVertFragShadersFromDirectory(const char *directory)
	{
		shaders_dir = directory;

		std::string dir;

		dir = SHADER_GLSL_DEFAULT_DIR;
		dir += directory;
		dir += "/";

		std::string vert_shader = dir;
		vert_shader += "vertex_shader.glsl";

		std::string fragment_shader = dir;
		fragment_shader += "fragment_shader.glsl";

		initVertFragShaders(vert_shader.c_str(), fragment_shader.c_str());
	}


	/**
	 * initialize and attach geometry, vertex and fragment shaders from directory assuming that
	 * vertex shader is given in [directory]/vertex_shader.glsl and
	 * geometry shader is given in [directory]/geometry_shader.glsl and
	 * fragment shader is given in [directory]/fragment_shader.glsl
	 */
	void initGeomVertFragShadersFromDirectory(const char *directory)
	{
		shaders_dir = directory;

		std::string dir;
		dir = SHADER_GLSL_DEFAULT_DIR;
		dir += directory;
		dir += "/";

		std::string geom_shader = dir;
		geom_shader += "geometry_shader.glsl";

		std::string vert_shader = dir;
		vert_shader += "vertex_shader.glsl";

		std::string fragment_shader = dir;
		fragment_shader += "fragment_shader.glsl";

		initGeomVertFragShaders(vert_shader.c_str(), geom_shader.c_str(), fragment_shader.c_str());
	}




	/**
	 * return true, if information log is valid and store information log to parameter
	 */
	bool getInfoLog(std::string &infoLog)
	{
		GLint length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

		if (length == 0)
		{
			// no info log available
			infoLog = "";
			return false;
		}

		GLchar *info_log_buf = new GLchar[length];

		// returned string is already zero terminated
		glGetProgramInfoLog(program, length, NULL, info_log_buf);

		infoLog = info_log_buf;
		return true;
	}

	/**
	 * return string for info log
	 */
	std::string getInfoLog()
	{
		std::string infoLog;

		GLint length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

		if (length == 0)
			return infoLog;

		GLchar *info_log_buf = new GLchar[length];

		// returned string is already zero terminated
		glGetProgramInfoLog(program, length, NULL, info_log_buf);

		infoLog = info_log_buf;
		return infoLog;
	}

	/**
	 * link compiled shaders
	 */
	bool link()
	{
		glLinkProgram(program);
		CGlErrorCheck();

		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);

		if (status == GL_TRUE)
			return true;

		std::ostringstream ss;
		ss << "Failed to link() shader code" << std::endl;
#if LBM_OPENCL_FS_DEBUG
		ss << "prefix_string_lines: " << prefix_string_lines << std::endl;
#endif
		ss << "Info Log:" << std::endl;
		ss << getInfoLog() << std::endl;
		for (auto iter = shaders.begin(); iter != shaders.end(); iter++)
			ss << " + " << iter->shader_filename << std::endl;
		std::cout << "INFO Log End" << std::endl;
		throw std::runtime_error(ss.str());
		return false;
	}

	/**
	 * bind attribute (input) location
	 */
	inline void bindAttribLocation(GLuint index, const GLchar *name)
	{
		glBindAttribLocation(program, index, name);
		CGlErrorCheck();
	}

	/**
	 * bind fragment data (output) location
	 */
	void bindFragDataLocation(GLuint color_number, const GLchar *name)
	{
		glBindFragDataLocation(program, color_number, name);
		CGlErrorCheck();
	}

	/**
	 * set uniform to the value
	 */
	inline void setUniform1i(const GLchar *name, GLint value)
	{
		CGlUniform uniform;
		setupUniform(uniform, name);
		uniform.set1i(value);

		CGlErrorCheckWithMessage(name);
	}

	/**
	 * set uniform to the 3 components of value
	 */
	inline void setUniform3fv(const GLchar *name, GLfloat *value)
	{
		CGlUniform uniform;
		setupUniform(uniform, name);
		uniform.set3fv(value);

		CGlErrorCheckWithMessage(name);
	}

	/**
	 * return the uniform location for a given uniform
	 */
	inline GLint getUniformLocation(const GLchar *p_name)
	{
		return glGetUniformLocation(program, p_name);
	}


	/**
	 * initialize uniform from program
	 */
	inline void setupUniform(
			CGlUniform &p_uniform,	///< uniform to setup
			const GLchar *p_name	///< name of uniform variable in program
	)
	{
		p_uniform.name = p_name;
		p_uniform.location = glGetUniformLocation(program, p_name);

		if (p_uniform.location == -1)
		{
			/**
			 * just output some useful information
			 *
			 * if location is set to -1, setting a value for such a uniform variable
			 * produces no error
			 */
#if LBM_OPENCL_FS_DEBUG
			std::cout << "!!! info: uniform location \"" << p_name << "\" not found";
			if (shaders_dir != "")
				std::cout << " for shaders \"" << shaders_dir << "\"";
			std::cout << "!!!" << std::endl;
#endif
		}
	}


	/**
	 * use the program
	 */
	inline 	void use()
	{
		glUseProgram(program);
		CGlErrorCheck();
	}

	/**
	 * disable usage of program
	 */
	static inline 	void disable()
	{
#if LBM_OPENCL_FS_DEBUG==1
		glUseProgram(0);
		CGlErrorCheck();
#endif
	}

	/**
	 * return state of program (true, if program can be used)
	 */
	inline bool validate()
	{
		glValidateProgram(program);

		GLint status;
		glGetProgramiv(1, GL_VALIDATE_STATUS, &status);

		return status == GL_TRUE;
	}

	/**
	 * free and delete OpenGL program
	 */
	inline void free()
	{
		if (program == 0)
			return;
		glDeleteProgram(program);
		program = 0;
	}

	inline ~CGlProgram()
	{
		free();
		CGlErrorCheck();
	}
};

/**
 * \brief	convenient function to activate usage of programs within {} blocks
 */
class CGlProgramUse
{
public:
	/**
	 * activate usage of program in current program block {}
	 */
	inline CGlProgramUse(CGlProgram &p_program)
	{
		p_program.use();
	}

	/**
	 * disable program
	 */
	inline ~CGlProgramUse()
	{
		CGlProgram::disable();
	}
};

#endif
