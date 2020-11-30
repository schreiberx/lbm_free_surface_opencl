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
 * CGlMarchingCubesSkeleton.hpp
 *
 *  Created on: Jan 19, 2010
 *      Author: martin
 */

#ifndef CGLMARCHINGCUBESSKELETON_HPP_
#define CGLMARCHINGCUBESSKELETON_HPP_

/**
 * sleketon class to unify the triangle vertex lists for marching cubes
 */
class CGlMarchingCubesSkeleton
{
public:
	static const int triangleVertices[256*(5*3+1)];	///< marching cubes triangles list
};


#endif /* CGLMARCHINGCUBESSKELETON_HPP_ */
