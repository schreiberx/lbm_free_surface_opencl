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


#ifndef CGL_RENDER_OBJFILE_HPP
#define CGL_RENDER_OBJFILE_HPP

#include "lib/CObjFile.hpp"
#include "lib/CError.hpp"
#include "libgl/CGlMaterial.hpp"
#include "libgl/core/CGlVertexArrayObject.hpp"
#include "libgl/core/CGlBuffer.hpp"

/**
 * \brief render a object file loaded by CObjFile.hpp
 */
class CGlRenderObjFile
{
	CGlVertexArrayObject vao;
	CGlBuffer buffer;

	GLuint faces3_count;

public:
	CError error;				///< error handler
	CGlMaterial cGlMaterial;
	bool texture_valid;

public:
	/**
	 * load and initialize buffers with object file handler
	 */
	void load(	CObjFile &cObjFile)
	{
		cleanup();

		// load first group
		CObjFile::CGroup &g = cObjFile.groups.front();
		CObjFile::CGroupMaterialMesh &m = g.meshes.front();

		// initialize buffers
		vao.bind();
			buffer.bind();
			buffer.data(g.lin_buffer_size, g.lin_buffer);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, g.rel_vertices);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, g.rel_normals);
			glEnableVertexAttribArray(1);

			if (g.rel_texture_coords != NULL)
			{
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, g.rel_texture_coords);
				glEnableVertexAttribArray(2);
			}
		vao.unbind();


		faces3_count = g.lin_faces3;

		if (g.rel_texture_coords > 0)
		{
			if (m.material->texture_file != "")
			{
				cGlMaterial.loadTexture0(std::string("data/textures/") + m.material->texture_file);

				if (cGlMaterial.texture0->error())
				{
					assert(false);
					error << cGlMaterial.texture0->error.getString();
				}
			}

			cGlMaterial.ambient_color3 = m.material->ambient_color3;
			cGlMaterial.diffuse_color3 = m.material->diffuse_color3;
			cGlMaterial.specular_color3 = m.material->specular_color3;
			cGlMaterial.specular_exponent = m.material->specular_exponent;
		}
	}

	void init()
	{
	}

	CGlRenderObjFile()	:
		buffer(GL_ARRAY_BUFFER)
	{
		init();
		cleanup();
	}


	/**
	 * load and initialize buffers to render an already loaded .obj file
	 */
	CGlRenderObjFile(CObjFile &cObjFile)	:
			buffer(GL_ARRAY_BUFFER)
	{
		init();
		cleanup();
		load(cObjFile);
	}


	/**
	 * render the object file without using a GLSL program
	 */
	void renderWithoutProgram()
	{
		vao.bind();

		glDrawArrays(GL_TRIANGLES, 0, faces3_count*3);

		vao.unbind();

		CGlErrorCheck();
	}

	void cleanup()
	{
		cGlMaterial.cleanup();
	}

	~CGlRenderObjFile()
	{
		cleanup();
	}
};

#endif
