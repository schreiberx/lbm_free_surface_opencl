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


#ifndef CCL_STACK_HPP
#define CCL_STACK_HPP

#include "CL/cl.hpp"
#include "libopencl/CCLSkeleton.hpp"


/**
 * CCLStack implements a stack with the capabilities of adding and removing
 * arbitrary items while keeping the stack gapless after triggering the
 * execution of the remove_gaps kernel.
 *
 * For convenience, the first item of each stack contains the number of items
 * in the stack _including_ this first 'items in stack' element. therefore
 * this number is the index to the next free item in the stack.
 */
template <typename T>
class CCLStack
{
	size_t max_stack_size;

	/**
	 * The stack_push stack can be used by programs to simply push items on
	 * the stack.
	 *
	 * This stack is the only one which stores the values
	 */
	cl::Buffer stack_push;

	/**
	 * Together with stack_push, this stack is used store those item indices
	 * which have to be deleted. the corresponding item in stack_push is
	 * marked as being deleted.
	 *
	 * This stack only stored indices of stack_push!
	 */
	cl::Buffer stack_delete;
	cl::Kernel remove_gaps_kernel;
	CCLSkeleton &skeleton;

	CCLStack(	CCLSkeleton &p_skeleton,
				size_t p_max_stack_size
				) :
					skeleton(p_skeleton),
					max_stack_size(p_max_stack_size)
	{
		stack_push = cl::Buffer(skeleton.cContext, CL_MEM_READ_WRITE, max_stack_size*sizeof(T));
		stack_delete = cl::Buffer(skeleton.cContext, CL_MEM_READ_WRITE, max_stack_size*sizeof(cl_int));
	}


};
#endif
