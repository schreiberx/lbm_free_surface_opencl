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


/**
 * !!!!!!!!!!! IMPORTANT !!!!!!!!!!!
 *
 * this is not a optimized version. this version is really slow.
 * it should be only used to output debug messages.
 */

/**
 * version 0.2
 *
 * history:
 * 2010-12-22: ported to opengl 3.2 core mode.
 * 2007-09-02: implemented CGlTexture class instead of static function
 */

#include "libgl/hud/CGlTextOutput.hpp"
#include "libmath/CMath.hpp"
#include "libgl/core/CGlError.hpp"
#include "libgl/core/CGlState.hpp"
#include "libmath/CGlSlMath.hpp"


/**
 * Load output texture handler with fontmap from 'file'
 * \param file	image file to load fontmap from
 */
CGlTextOutput::CGlTextOutput(const char *file)
{
	load(file);
}

/**
 * Load output texture handler with fontmap from 'file'
 * \param file	image file to load fontmap from
 */
void CGlTextOutput::load(const char *file)
{
	/*
	 * initialize font map
	 */
	fontTexture.loadFromFile(file);

	if (fontTexture.error())
	{
		error << "failed to load fontfile '" << file << "': " << fontTexture.error.getString();
		init = false;
		return;
	}
#if 1
	fontTexture.setParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	fontTexture.setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
	fontTexture.setParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	fontTexture.setParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
//	fontTexture.setParam(GL_TEXTURE_WRAP_S, GL_REPEAT);
//	fontTexture.setParam(GL_TEXTURE_WRAP_T, GL_REPEAT);

	charWidth = (float)fontTexture.width / 16.0f;
	charHeight = (float)fontTexture.height / 16.0f;

	fontSize(charHeight);

	CGlErrorCheck();

	/*
	 * load shader programs
	 */
	font_program.initVertFragShadersFromDirectory("text_output");
	font_program.bindAttribLocation(0, "vertex_position");
	font_program.bindAttribLocation(1, "vertex_texture_coord");

	font_program.link();

	font_program.bindFragDataLocation(0, "out_color");
//	font_program.getUniformLocation("pvm_matrix", pmv_uniform);

	init = true;
}


/**
 * set the font size to draw characters
 */
void CGlTextOutput::fontSize(float size)
{
	scaleY = size;
	scaleX = charWidth * (scaleY / charHeight);
}

/**
 * output formatted string at character position (x,y)
 *
 * the output coordinates are in character position
 */
void CGlTextOutput::printfxy(int x, int y, const char *format, ...)
{
	if (init == false)	return;

	fontTexture.bind();

	va_list list;
	va_start(list, format);

	static char text[2048];
	vsnprintf(text, 2048, format, list);

	CGlStateDisable depthTest(GL_DEPTH_TEST);

	float viewport[4];	// [2]: width, [3]: height
	glGetFloatv(GL_VIEWPORT, viewport);

	// draw chars
	size_t text_length = strlen(text);
	float vdx = (2.0f*charWidth)/viewport[2];
	float vdy = (2.0f*charHeight)/viewport[3];

	float vx = -1.0f+vdx*(float)x;
	float vy = 1.0f-vdy*(float)y;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	float vertices[4][2];
	float texcoords[4][2];

	// index, size, type, normalized, stride, pointer
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoords);

	font_program.use();

	for (size_t i = 0; i < text_length; i++)
	{
		unsigned int font_char = text[i];
		if (font_char == '\n')
		{
			vy -= vdy;
			vx = -1.0f+vdx*(float)x;;
			continue;
		}

		font_char -= 32;

		float tx = (float)(font_char & 0x0f) / 16.0f;
		float ty = (float)(font_char >> 4) / 16.0f;

		vertices[0][0] = vx;
		vertices[0][1] = vy-vdy;

		vertices[1][0] = vx+vdx;
		vertices[1][1] = vy-vdy;

		vertices[2][0] = vx+vdx;
		vertices[2][1] = vy;

		vertices[3][0] = vx;
		vertices[3][1] = vy;

#define D	(1.0f/16.0f)
		texcoords[0][0] = tx;
		texcoords[0][1] = ty+D;

		texcoords[1][0] = tx+D;
		texcoords[1][1] = ty+D;

		texcoords[2][0] = tx+D;
		texcoords[2][1] = ty;

		texcoords[3][0] = tx;
		texcoords[3][1] = ty;
#undef D

		CGlErrorCheck();

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		vx += vdx;
	}
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	font_program.disable();

	CGlErrorCheck();
}
