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
 * CTextures.hpp
 *
 *  Created on: Mar 22, 2010
 *      Author: martin
 */

#ifndef CTEXTURES_HPP_
#define CTEXTURES_HPP_

/**
 * \brief class to handle different textures
 */
class CTextures
{
	bool verbose;

public:
	CError error;	///< error handler

	CGlTexture texture1;	///< texture1
	CGlTexture texture2;	///< texture2
	CGlTexture texture3;	///< texture3
	CGlTexture texture4;	///< texture4

	CTextures(bool p_verbose)	:
			verbose(p_verbose)
	{

	}

	/**
	 * setup (load) textures
	 */
	void setup()
	{
		texture1.loadFromFile("data/textures/img_8598.jpg");
		CError_Append(texture1);

		texture2.loadFromFile("data/textures/skybox/img_5264.jpg");
		CError_Append(texture2);

		texture3.loadFromFile("data/textures/skybox/img_8093.jpg");
		CError_Append(texture3);

		texture4.loadFromFile("data/textures/lena.jpg");
		CError_Append(texture3);
	}
};

#endif /* CTEXTURES_HPP_ */
