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


#ifndef CGL_VIEWPORT_HPP
#define CGL_VIEWPORT_HPP

#include "libgl/incgl3.h"

/**
 * \brief	handle GL viewport (save, restore)
 */
class CGlViewport
{
	GLfloat viewport[4];

public:
	inline CGlViewport()
	{
	}

	/**
	 * update viewport to new size
	 */
	inline void setSize(GLsizei width, GLsizei height)
	{
		glViewport(0, 0, width, height);
	}

	/**
	 * update viewport to new size
	 */
	inline void set(GLsizei x, GLsizei y, GLsizei width, GLsizei height)
	{
		glViewport(x, y, width, height);
	}

	/**
	 * save current state of viewport
	 */
	inline void saveState()
	{
		glGetFloatv(GL_VIEWPORT, &viewport[0]);
	}

	/**
	 * restore saved viewport state
	 */
	inline void restoreState()
	{
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	}
};
#endif
