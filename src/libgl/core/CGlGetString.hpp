/*
 * CGlGetString.hpp
 *
 *  Created on: Nov 30, 2020
 *      Author: martin
 */

#ifndef SRC_LIBGL_CORE_CGLGETSTRING_HPP_
#define SRC_LIBGL_CORE_CGLGETSTRING_HPP_

#include <GL3/gl3.h>
#include <string>

class CGlGetString
{
public:
	static
	const std::string getVendor()
	{
		const char* retstr = (const char*)glGetString(GL_VENDOR);

		if (retstr == 0)
			throw std::runtime_error("getVendor()");

		return retstr;
	}

	static
	const std::string getRenderer()
	{
		const char* retstr = (const char*)glGetString(GL_RENDERER);

		if (retstr == 0)
			throw std::runtime_error("getRenderer()");

		return retstr;
	}

	static
	const std::string getVersion()
	{
		const char* retstr = (const char*)glGetString(GL_VERSION);

		if (retstr == 0)
			throw std::runtime_error("getVersion()");

		return retstr;
	}
};



#endif /* SRC_LIBGL_CORE_CGLGETSTRING_HPP_ */
