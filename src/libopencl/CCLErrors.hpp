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


#ifndef CCOMPLANGERRORS_HH__
#define CCOMPLANGERRORS_HH__

extern "C" {
	#include <CL/cl.h>
}

static const char *cclGetErrorString(cl_int errorNum)
{
	const char *return_string;

	switch(errorNum)
	{
		case CL_SUCCESS:					return_string = "CL_SUCCESS";					break;
		case CL_DEVICE_NOT_FOUND:			return_string = "CL_DEVICE_NOT_FOUND";			break;
		case CL_DEVICE_NOT_AVAILABLE:		return_string = "CL_DEVICE_NOT_AVAILABLE";		break;
		case CL_COMPILER_NOT_AVAILABLE:		return_string = "CL_COMPILER_NOT_AVAILABLE";	break;
		case CL_MEM_OBJECT_ALLOCATION_FAILURE:		return_string = "CL_MEM_OBJECT_ALLOCATION_FAILURE";	break;
		case CL_OUT_OF_RESOURCES:			return_string = "CL_OUT_OF_RESOURCES";			break;
		case CL_OUT_OF_HOST_MEMORY:			return_string = "CL_OUT_OF_HOST_MEMORY";		break;
		case CL_PROFILING_INFO_NOT_AVAILABLE:		return_string = "CL_PROFILING_INFO_NOT_AVAILABLE";	break;
		case CL_MEM_COPY_OVERLAP:			return_string = "CL_MEM_COPY_OVERLAP";			break;
		case CL_IMAGE_FORMAT_MISMATCH:		return_string = "CL_IMAGE_FORMAT_MISMATCH";		break;
		case CL_IMAGE_FORMAT_NOT_SUPPORTED:	return_string = "CL_IMAGE_FORMAT_NOT_SUPPORTED";	break;
		case CL_BUILD_PROGRAM_FAILURE:		return_string = "CL_BUILD_PROGRAM_FAILURE";		break;
		case CL_MAP_FAILURE:				return_string = "CL_MAP_FAILURE";				break;

		case CL_INVALID_VALUE:				return_string = "CL_INVALID_VALUE";				break;
		case CL_INVALID_DEVICE_TYPE:		return_string = "CL_INVALID_DEVICE_TYPE";		break;
		case CL_INVALID_PLATFORM:			return_string = "CL_INVALID_PLATFORM";			break;
		case CL_INVALID_DEVICE:				return_string = "CL_INVALID_DEVICE";			break;
		case CL_INVALID_CONTEXT:			return_string = "CL_INVALID_CONTEXT";			break;
		case CL_INVALID_QUEUE_PROPERTIES:	return_string = "CL_INVALID_QUEUE_PROPERTIES";	break;
		case CL_INVALID_COMMAND_QUEUE:		return_string = "CL_INVALID_COMMAND_QUEUE";		break;
		case CL_INVALID_HOST_PTR:			return_string = "CL_INVALID_HOST_PTR";			break;
		case CL_INVALID_MEM_OBJECT:			return_string = "CL_INVALID_MEM_OBJECT";		break;
		case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:	return_string = "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";	break;
		case CL_INVALID_IMAGE_SIZE:			return_string = "CL_INVALID_IMAGE_SIZE";		break;
		case CL_INVALID_SAMPLER:			return_string = "CL_INVALID_SAMPLER";			break;
		case CL_INVALID_BINARY:				return_string = "CL_INVALID_BINARY";			break;
		case CL_INVALID_BUILD_OPTIONS:		return_string = "CL_INVALID_BUILD_OPTIONS";		break;
		case CL_INVALID_PROGRAM:			return_string = "CL_INVALID_PROGRAM";			break;
		case CL_INVALID_PROGRAM_EXECUTABLE:	return_string = "CL_INVALID_PROGRAM_EXECUTABLE";	break;
		case CL_INVALID_KERNEL_NAME:		return_string = "CL_INVALID_KERNEL_NAME";		break;
		case CL_INVALID_KERNEL_DEFINITION:	return_string = "CL_INVALID_KERNEL_DEFINITION";	break;
		case CL_INVALID_KERNEL:				return_string = "CL_INVALID_KERNEL";			break;
		case CL_INVALID_ARG_INDEX:			return_string = "CL_INVALID_ARG_INDEX";			break;
		case CL_INVALID_ARG_VALUE:			return_string = "CL_INVALID_ARG_VALUE";			break;
		case CL_INVALID_ARG_SIZE:			return_string = "CL_INVALID_ARG_SIZE";			break;
		case CL_INVALID_KERNEL_ARGS:		return_string = "CL_INVALID_KERNEL_ARGS";		break;
		case CL_INVALID_WORK_DIMENSION:		return_string = "CL_INVALID_WORK_DIMENSION";	break;
		case CL_INVALID_WORK_GROUP_SIZE:	return_string = "CL_INVALID_WORK_GROUP_SIZE";	break;
		case CL_INVALID_WORK_ITEM_SIZE:		return_string = "CL_INVALID_WORK_ITEM_SIZE";	break;
		case CL_INVALID_GLOBAL_OFFSET:		return_string = "CL_INVALID_GLOBAL_OFFSET";		break;
		case CL_INVALID_EVENT_WAIT_LIST:	return_string = "CL_INVALID_EVENT_WAIT_LIST";	break;
		case CL_INVALID_EVENT:				return_string = "CL_INVALID_EVENT";				break;
		case CL_INVALID_OPERATION:			return_string = "CL_INVALID_OPERATION";			break;
		case CL_INVALID_GL_OBJECT:			return_string = "CL_INVALID_GL_OBJECT";			break;
		case CL_INVALID_BUFFER_SIZE:		return_string = "CL_INVALID_BUFFER_SIZE";		break;
		case CL_INVALID_MIP_LEVEL:			return_string = "CL_INVALID_MIP_LEVEL";			break;
		case CL_INVALID_GLOBAL_WORK_SIZE:			return_string = "CL_INVALID_GLOBAL_WORK_SIZE";			break;
		default:							return_string = "UNKNOWN";						break;
	}

#if 0
		std::cerr << return_string << std::endl;
		std::cerr << "[OPENCL ERROR: creating null pointer exception to debug program...]" << std::endl;
		int *null_exception = 0;
		*null_exception = 0;
#endif

	return return_string;
}


#if LBM_OPENCL_FS_DEBUG
	#define CCL_ERRORS_RET				\
		std::cerr << return_string << std::endl;				\
		std::cerr << "[OPENCL ERROR: creating null pointer exception to debug program...]" << std::endl;	\
		int *null_exception = 0;								\
		*null_exception = 0;
#else
	#define CCL_ERRORS_RET
#endif

#define CL_CHECK_ERROR(val_a)									\
if ((val_a) != CL_SUCCESS)										\
	{															\
		const char *return_string = cclGetErrorString(val_a);	\
		std::cerr	<< "CL_ERROR: file: '" << __FILE__			\
	       			<< "', line: " << __LINE__					\
	       			<< " - " << return_string << "(#" << val_a << ")" << std::endl;	\
																\
		CCL_ERRORS_RET											\
	}


#endif
