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
 * CShaders.hpp
 *
 *  Created on: Mar 22, 2010
 *      Author: martin
 */

#ifndef CSHADERS_HPP_
#define CSHADERS_HPP_

#include "shaders/shader_blinn/CShaderBlinn.hpp"
#include "shaders/shader_blinn_shadow_map/CShaderBlinnShadowMap.hpp"
#include "shaders/shader_cube_map_mirror/CShaderCubeMapMirror.hpp"
#include "shaders/shader_texturize/CShaderTexturize.hpp"
#include "shaders/shader_color/CShaderColor.hpp"

/**
 * \brief class to handle different shaders
 */
class CShaders
{
	bool verbose;

public:
	CError error;	///< error handler

	CShaderCubeMapMirror	cCubeMapMirror;		///< shader to render cubemap
	CShaderTexturize		cTexturize;			///< shader for simple texturization
	CShaderBlinn			cBlinn;				///< blinn shader
	CShaderBlinnShadowMap	cBlinnShadowMap;	///< blinn shader with shadow map
	CShaderColor			cColor;				///< color shader

	CShaders(bool p_verbose)	:
			verbose(p_verbose)
	{
	}
};

#endif
