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




#ifndef C_GL_TEXT_OUTPUT_HPP
#define C_GL_TEXT_OUTPUT_HPP

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlProgram.hpp"

/**
 * \brief Output text to the OpenGL rendering context
 *
 * this class uses a texture (16 x 16 chars) to draw a string to the screen
 *
 * the implementation is really slow. thus it shouldn't be used for the productive system.
 */
class CGlTextOutput
{
	private:
		CGlTexture fontTexture;
		CGlProgram font_program;
		CGlUniform pmv_uniform;

		float charWidth, charHeight;
		float scaleX, scaleY;

	public:
		CError error;	///< error handler
		bool init;		///< true, if initialization if successful

		CGlTextOutput(const char *file = "images/fontmap.png");
		void load(const char *file = "images/fontmap.png");
		void fontSize(float size);
		void printfxy(int x, int y, const char *format, ...);
};


#endif
