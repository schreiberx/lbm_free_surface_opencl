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
 * CMarchingCubesRenderers.hpp
 *
 *  Created on: Mar 22, 2010
 *      Author: martin
 */

#ifndef CMARCHINGCUBESRENDERERS_HPP_
#define CMARCHINGCUBESRENDERERS_HPP_

#include "libgl/marching_cubes/CGlMarchingCubesGeometryShader.hpp"
#include "libgl/marching_cubes/CGlMarchingCubesVertexArray.hpp"
#include "libgl/marching_cubes/CGlMarchingCubesVertexArrayRGBA.hpp"

/**
 * class handler for different marching cubes renderers
 */
class CMarchingCubesRenderers
{
public:
		CError error;	///< error informations

		CGlMarchingCubesGeometryShader cGlMarchingCubesGeometryShader;	///< MC using geometry shader
		CGlMarchingCubesVertexArray cGlMarchingCubesVertexArray;		///< MC using vertex array and R channel for HistoPyramid
		CGlMarchingCubesVertexArrayRGBA cGlMarchingCubesVertexArrayRGBA;	///< MC using vertex array and RGBA channels for HistoPyramid

		/**
		 * initalize MC renderers
		 */
		CMarchingCubesRenderers(bool verbose)	:
				cGlMarchingCubesGeometryShader(verbose),
				cGlMarchingCubesVertexArray(verbose),
				cGlMarchingCubesVertexArrayRGBA(verbose)
		{
			CError_AppendReturn(cGlMarchingCubesGeometryShader);
			CError_AppendReturn(cGlMarchingCubesVertexArray);
			CError_AppendReturn(cGlMarchingCubesVertexArrayRGBA);
		}

		void reset(CVector<3,int> &domain_cells)
		{
			cGlMarchingCubesGeometryShader.reset(domain_cells);
			cGlMarchingCubesVertexArray.reset(domain_cells);
			cGlMarchingCubesVertexArrayRGBA.reset(domain_cells);

			CError_AppendReturn(cGlMarchingCubesGeometryShader);
			CError_AppendReturn(cGlMarchingCubesVertexArray);
			CError_AppendReturn(cGlMarchingCubesVertexArrayRGBA);
		}
};


#endif /* CMARCHINGCUBESRENDERERS_HPP_ */
