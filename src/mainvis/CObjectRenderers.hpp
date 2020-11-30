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

/*
 * CObjectRenderers.hpp
 *
 *  Created on: Mar 22, 2010
 *      Author: martin
 */

#ifndef COBJECT_RENDERERS_HPP_
#define COBJECT_RENDERERS_HPP_

#include "libgl/CGlRenderObjFile2.hpp"

#include "libgl/CGlRenderObjFile.hpp"
#include "libgl/draw/CGlDrawSphere.hpp"
#include "libgl/draw/CGlDrawQuad.hpp"
#include "lib/CObjFile.hpp"

/**
 * \brief class to handle different object renderers (table, sphere, quad, bunny, etc.)
 */
class CObjectRenderers
{
	bool verbose;

public:
	CError error;	///< error handler

	CGlDrawSphere cGlDrawSphere;	///< sphere renderer
	CGlDrawQuad cGlDrawQuad;		///< quad renderer
	CGlRenderObjFile cGlRenderObjFileSphere;	///< renderer for sphere loaded from object file
	CGlRenderObjFile cGlRenderObjFileBunny;		///< renderer for bunny loaded from object file

	CGlRenderObjFile2 cGlRenderObjFileTable;	///< renderer for table loaded from object file
	CGlRenderObjFile2 cGlRenderScene;			///< renderer for scene
	CGlRenderObjFile2 cGlRenderFluidBorder;		///< renderer the fluid border

	CObjectRenderers(	bool p_verbose	)	:
		verbose(p_verbose)
	{
	}

	/**
	 * initialize renderers with default parameters
	 */
	void setup()
	{
		CGlErrorCheck();
		cGlDrawSphere.initSphere(30, 30);

		/**
		 * LOAD OBJ FILES
		 */
		CGlErrorCheck();
		if (verbose)
			std::cout << "loading bunny obj file" << std::endl;
		CObjFile cObjFileBunny;
		cObjFileBunny.load("data/meshes/bunny_world.obj");
		CError_AppendReturn(cObjFileBunny);
		std::cout << cObjFileBunny.message.getString();

		cGlRenderObjFileBunny.load(cObjFileBunny);
		CError_AppendReturn(cGlRenderObjFileBunny);


		CGlErrorCheck();
		if (verbose)
			std::cout << "loading sphere obj file" << std::endl;
		CObjFile cObjFileSphere;
		cObjFileSphere.load("data/meshes/sphere.obj");
		CError_AppendReturn(cObjFileSphere);
		std::cout << cObjFileSphere.message.getString();

		cGlRenderObjFileSphere.load(cObjFileSphere);
		CError_AppendReturn(cGlRenderObjFileSphere);


		CGlErrorCheck();
		if (verbose)
			std::cout << "loading table obj file" << std::endl;
		CObjFile cObjFileTable;
		cObjFileTable.load("data/meshes/quad_table.obj");
		CError_AppendReturn(cObjFileTable);
		std::cout << cObjFileTable.message.getString();

		cGlRenderObjFileTable.load(cObjFileTable);
		CError_AppendReturn(cGlRenderObjFileTable);


		CGlErrorCheck();
		if (verbose)
			std::cout << "loading scene file" << std::endl;
		CObjFile cObjFileScene;
		cObjFileScene.load("data/meshes/scene.obj", false);
		CError_AppendReturn(cObjFileScene);
		std::cout << cObjFileScene.message.getString();

		cGlRenderScene.load(cObjFileScene);
		CError_AppendReturn(cGlRenderScene);


		CGlErrorCheck();
		if (verbose)
			std::cout << "loading fluid border file" << std::endl;
		CObjFile cObjFileFluidBorder;
		cObjFileFluidBorder.load("data/meshes/fluid_border.obj", false);
		CError_AppendReturn(cObjFileFluidBorder);
		std::cout << cObjFileFluidBorder.message.getString();

		cGlRenderFluidBorder.load(cObjFileFluidBorder);
		CError_AppendReturn(cGlRenderFluidBorder);
	}
};


#endif /* CRENDERERS_HPP_ */
