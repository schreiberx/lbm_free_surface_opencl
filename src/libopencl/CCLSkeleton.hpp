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
 * CCLSkeleton.hpp
 *
 *  Created on: Jan 3, 2010
 *      Author: martin
 */

#ifndef CCLSKELETON_HPP_
#define CCLSKELETON_HPP_

#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#include <CL/opencl.hpp>

#include "lib/CError.hpp"
#include "libopencl/CCLErrors.hpp"
#include <libgl/incgl3.h>
#include <exception>

#ifdef C_GL_TEXTURE_HPP
#define __gl_h_	1
#endif


#ifdef C_GL_TEXTURE_HPP
	#include <CL/cl_gl.h>

	#ifndef CL_GL_CONTEXT_KHR
		#define		CL_GL_CONTEXT_KHR		0x2008
		#define		CL_EGL_DISPLAY_KHR		0x2009
		#define		CL_GLX_DISPLAY_KHR		0x200A
		#define		CL_WGL_HDC_KHR			0x200B
		#define		CL_CGL_SHAREGROUP_KHR	0x200C
	#endif

#if __unix
	#include <GL/glx.h>
#endif

#endif

/**
 * \brief OpenCL skeleton
 *
 * this class can be used to handle the OpenCL access variables/ids
 * conveniently to share them with other classes (e.g. simulation and gui)
 */
class CCLSkeleton
{
public:
    CError error;		///< error handler
    bool verbose;		///< verbose informations on/off

    cl::Device cDevice;							///< OpenCL device for computations
	std::vector<cl::Platform> cPlatforms;		///< OpenCL platforms
	cl::Platform cPlatform;						///< OpenCL platform which is actually used
	std::vector<cl::Device> cDevices;			///< OpenCL devices
    cl::CommandQueue cCommandQueue;				///< OpenCL command queue for computations
    cl::Context cContext;						///< OpenCL context for computations


    CCLSkeleton& operator=(const CCLSkeleton &s)
    {
    	cDevice = s.cDevice;
    	cPlatforms = s.cPlatforms;
    	cPlatform = s.cPlatform;
    	cDevices = s.cDevices;
    	cCommandQueue = s.cCommandQueue;
    	cContext = s.cContext;
    	return *this;
    }

	/**
	 * \brief load information about a specific device
	 */
	class CDeviceInfo
	{
    	cl::Device cDevice;

	public:
		cl_device_type device_type;		///< OpenCL device type
		cl_uint vendor_id;				///< OpenCL vendor id
		cl_uint max_compute_units;		///< maximum compute units available
		cl_uint max_work_item_dimensions;	///< maximum number of dimensions

		size_t *max_work_item_sizes;	///< maximum amount of work items
		size_t max_work_group_size;		///< maximum group size

		cl_uint preferred_vector_width_char;	///< preferred vector width for type char
		cl_uint preferred_vector_width_short;	///< preferred vector width for type short
		cl_uint preferred_vector_width_int;		///< preferred vector width for type int
		cl_uint preferred_vector_width_long;	///< preferred vector width for type long
		cl_uint preferred_vector_width_float;	///< preferred vector width for type float
		cl_uint preferred_vector_width_double;	///< preferred vector width for type float

		cl_uint max_clock_frequency;			///< maximum clock frequency
		cl_uint address_bits;					///< address bits for device

		cl_ulong max_mem_alloc_size;			///< maximum number of allocatable bytes

		cl_bool image_support;					///< image support available
		cl_uint max_read_image_args;			///< maximum number of images as read kernel arguments
		cl_uint max_write_image_args;			///< maximum number of images as write kernel arguments

		size_t image2d_max_width;				///< maximum 2d image width
		size_t image2d_max_height;				///< maximum 2d image height

		size_t image3d_max_width;				///< maximum 3d image width
		size_t image3d_max_height;				///< maximum 3d image height
		size_t image3d_max_depth;				///< maximum 3d image depth

		cl_uint max_samplers;					///< maximum number of samplers
		size_t max_parameter_size;				///< maximum number of kernel parameters
		cl_uint mem_base_addr_align;			///< alignment of device base memory
		cl_uint min_data_type_align_size;		///< minimum alignment needed for device memory

		cl_device_fp_config single_fp_config;	///< single precision floating point capabilities
		cl_device_mem_cache_type global_mem_cache_type;	///< cache type of global memory
		cl_uint global_mem_cacheline_size;		///< size of a line of global memory cache
		cl_ulong global_mem_cache_size;			///< size of global memory cache
		cl_ulong global_mem_size;				///< size of global memory

		cl_ulong max_constant_buffer_size;		///< maximum bytes for constant buffer
		cl_uint max_constant_args;				///< maximum number of constant arguments
		cl_device_local_mem_type local_mem_type;	///< type of local memory
		cl_ulong local_mem_size;				///< size of local memory
		cl_bool error_correction;				///< error correction available
		size_t profiling_timer_resolution;		///< resolution of profiling timer
		cl_bool endian_little;					///< little endian device
		cl_bool available;						///< true, if device available
		cl_bool compiler_available;				///< true, if compiler for device is available
		cl_device_exec_capabilities execution_capabilities;	///< kernel execution capabilities
		cl_command_queue_properties queue_properties;	///< queue properties

		std::string name;				///< name of device
		std::string vendor;				///< vendor of device
		std::string driver_version;		///< driver version of device
		std::string profile;			///< profile of device
		std::string version;			///< version of device
		std::string extensions;			///< extensions available for device

		/**
		 * initialize device information with NULL data
		 */
		inline void initCDeviceInfo()
		{
			max_work_item_sizes = NULL;
		}

		inline CDeviceInfo()
		{
			initCDeviceInfo();
		}

		/**
		 * initialize device information from existing device
		 */
		inline CDeviceInfo(cl::Device &p_cDevice)
		{
			initCDeviceInfo();
			loadDeviceInfo(p_cDevice);
		}

		inline ~CDeviceInfo()
		{
			delete[] max_work_item_sizes;

		}

		/**
		 * return type of the device
		 */
		inline const char* getTypeString()
		{
			switch(device_type)
			{
				case CL_DEVICE_TYPE_CPU:			return "CPU";
				case CL_DEVICE_TYPE_GPU:			return "GPU";
				case CL_DEVICE_TYPE_ACCELERATOR:	return "ACCELERATOR";
				case CL_DEVICE_TYPE_DEFAULT:		return "DEFAULT";
				case CL_DEVICE_TYPE_ALL:			return "ALL";
				default:							return "unknown";
			}
		}

		/**
		 * load device information given by device_id
		 */
		inline void loadDeviceInfo(cl::Device &p_cDevice)
		{
			cDevice = p_cDevice;


			cDevice.getInfo(CL_DEVICE_TYPE, &device_type);
			cDevice.getInfo(CL_DEVICE_VENDOR_ID, &vendor_id);
			cDevice.getInfo(CL_DEVICE_MAX_COMPUTE_UNITS, &max_compute_units);
			cDevice.getInfo(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, &max_work_item_dimensions);

			delete [] max_work_item_sizes;
			max_work_item_sizes = new size_t[max_work_item_dimensions];

			cDevice.getInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE, &max_work_group_size);
			cDevice.getInfo(CL_DEVICE_MAX_WORK_ITEM_SIZES, &max_work_item_sizes);

			cDevice.getInfo(CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, &preferred_vector_width_char);
			cDevice.getInfo(CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, &preferred_vector_width_short);
			cDevice.getInfo(CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, &preferred_vector_width_int);
			cDevice.getInfo(CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, &preferred_vector_width_long);
			cDevice.getInfo(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, &preferred_vector_width_float);
			cDevice.getInfo(CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, &preferred_vector_width_double);

			cDevice.getInfo(CL_DEVICE_MAX_CLOCK_FREQUENCY,		&max_clock_frequency);

			cDevice.getInfo(CL_DEVICE_ADDRESS_BITS,				&address_bits);
			cDevice.getInfo(CL_DEVICE_MAX_MEM_ALLOC_SIZE,		&max_mem_alloc_size);
			cDevice.getInfo(CL_DEVICE_IMAGE_SUPPORT,			&image_support);
			cDevice.getInfo(CL_DEVICE_MAX_READ_IMAGE_ARGS,		&max_read_image_args);
			cDevice.getInfo(CL_DEVICE_MAX_WRITE_IMAGE_ARGS,		&max_write_image_args);
			cDevice.getInfo(CL_DEVICE_IMAGE2D_MAX_WIDTH,		&image2d_max_width);
			cDevice.getInfo(CL_DEVICE_IMAGE2D_MAX_HEIGHT,		&image2d_max_height);
			cDevice.getInfo(CL_DEVICE_IMAGE3D_MAX_WIDTH,		&image3d_max_width);
			cDevice.getInfo(CL_DEVICE_IMAGE3D_MAX_HEIGHT,		&image3d_max_height);
			cDevice.getInfo(CL_DEVICE_IMAGE3D_MAX_DEPTH,		&image3d_max_depth);

			cDevice.getInfo(CL_DEVICE_MAX_SAMPLERS,				&max_samplers);
			cDevice.getInfo(CL_DEVICE_MAX_PARAMETER_SIZE,		&max_parameter_size);
			cDevice.getInfo(CL_DEVICE_MEM_BASE_ADDR_ALIGN,		&mem_base_addr_align);
			cDevice.getInfo(CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE,	&min_data_type_align_size);

			cDevice.getInfo(CL_DEVICE_SINGLE_FP_CONFIG,			&single_fp_config);
			cDevice.getInfo(CL_DEVICE_GLOBAL_MEM_CACHE_TYPE,	&global_mem_cache_type);
			cDevice.getInfo(CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,	&global_mem_cacheline_size);
			cDevice.getInfo(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,	&global_mem_cache_size);
			cDevice.getInfo(CL_DEVICE_GLOBAL_MEM_SIZE,			&global_mem_size);

			cDevice.getInfo(CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,	&max_constant_buffer_size);
			cDevice.getInfo(CL_DEVICE_MAX_CONSTANT_ARGS,		&max_constant_args);
			cDevice.getInfo(CL_DEVICE_LOCAL_MEM_TYPE,			&local_mem_type);
			cDevice.getInfo(CL_DEVICE_LOCAL_MEM_SIZE,			&local_mem_size);
			cDevice.getInfo(CL_DEVICE_ERROR_CORRECTION_SUPPORT,	&error_correction);
			cDevice.getInfo(CL_DEVICE_PROFILING_TIMER_RESOLUTION,	&profiling_timer_resolution);
			cDevice.getInfo(CL_DEVICE_ENDIAN_LITTLE,			&endian_little);
			cDevice.getInfo(CL_DEVICE_AVAILABLE,				&available);
			cDevice.getInfo(CL_DEVICE_COMPILER_AVAILABLE,		&compiler_available);
			cDevice.getInfo(CL_DEVICE_EXECUTION_CAPABILITIES,	&execution_capabilities);
			cDevice.getInfo(CL_DEVICE_QUEUE_PROPERTIES,			&queue_properties);

			cDevice.getInfo(CL_DEVICE_NAME,				&name);
			cDevice.getInfo(CL_DEVICE_VENDOR,			&vendor);
			cDevice.getInfo(CL_DRIVER_VERSION,			&driver_version);
			cDevice.getInfo(CL_DEVICE_PROFILE,			&profile);
			cDevice.getInfo(CL_DEVICE_VERSION,			&version);
			cDevice.getInfo(CL_DEVICE_EXTENSIONS,		&extensions);
		}

		/**
		 * output previously loaded device information
		 */
		inline void printDeviceInfo(std::string sLinePrefix)
		{

			std::cout << sLinePrefix << "TYPE: ";

			if (device_type & CL_DEVICE_TYPE_CPU)	std::cout << "CPU ";
			if (device_type & CL_DEVICE_TYPE_GPU)	std::cout << "GPU ";
			if (device_type & CL_DEVICE_TYPE_ACCELERATOR)	std::cout << "ACCELERATOR ";
			if (device_type & CL_DEVICE_TYPE_DEFAULT)	std::cout << "DEFAULT ";
			std::cout << std::endl;

			std::cout << sLinePrefix << "VENDOR_ID: " << vendor_id << std::endl;
			std::cout << sLinePrefix << "MAX_COMPUTE_UNITS: " << max_compute_units << std::endl;
			std::cout << sLinePrefix << "MAX_WORK_ITEM_DIMENSIONS: " << max_work_item_dimensions << std::endl;

			for (cl_uint w = 0; w < max_work_item_dimensions; w++)
			{
				std::cout << sLinePrefix << "MAX_WORK_ITEM_SIZES[" << w << "]: " << max_work_item_sizes[w] << std::endl;
			}
			std::cout << sLinePrefix << "MAX_WORK_GROUP_SIZE: " << max_work_group_size << std::endl;
			std::cout << sLinePrefix << std::endl;
			std::cout << sLinePrefix << "PREFERRED_VECTOR_WIDTH_CHAR: " << preferred_vector_width_char << std::endl;
			std::cout << sLinePrefix << "PREFERRED_VECTOR_WIDTH_SHORT: " << preferred_vector_width_short << std::endl;
			std::cout << sLinePrefix << "PREFERRED_VECTOR_WIDTH_INT: " << preferred_vector_width_int << std::endl;
			std::cout << sLinePrefix << "PREFERRED_VECTOR_WIDTH_LONG: " << preferred_vector_width_long << std::endl;
			std::cout << sLinePrefix << "PREFERRED_VECTOR_WIDTH_FLOAT: " << preferred_vector_width_float << std::endl;
			std::cout << sLinePrefix << "PREFERRED_VECTOR_WIDTH_DOUBLE: " << preferred_vector_width_double << std::endl;
			std::cout << sLinePrefix << std::endl;
			std::cout << sLinePrefix << "MAX_CCLOCK_FREQUENCY: " << max_clock_frequency << std::endl;
			std::cout << sLinePrefix << "ADDRESS_BITS: " << address_bits << std::endl;
			std::cout << sLinePrefix << "MAX_MEM_ALLOC_SIZE: " << max_mem_alloc_size << std::endl;
			std::cout << sLinePrefix << std::endl;
			std::cout << sLinePrefix << "IMAGE_SUPPORT: " << image_support << std::endl;
			std::cout << sLinePrefix << "MAX_READ_IMAGE_ARGS: " << max_read_image_args << std::endl;
			std::cout << sLinePrefix << "MAX_WRITE_IMAGE_ARGS: " << max_write_image_args << std::endl;
			std::cout << sLinePrefix << "IMAGE2D_MAX_WIDTH: " << image2d_max_width << std::endl;
			std::cout << sLinePrefix << "IMAGE2D_MAX_HEIGHT: " << image2d_max_height << std::endl;
			std::cout << sLinePrefix << "IMAGE3D_MAX_WIDTH: " << image3d_max_width << std::endl;
			std::cout << sLinePrefix << "IMAGE3D_MAX_HEIGHT: " << image3d_max_height << std::endl;
			std::cout << sLinePrefix << "IMAGE3D_MAX_DEPTH: " << image3d_max_depth << std::endl;
			std::cout << sLinePrefix << std::endl;
			std::cout << sLinePrefix << "MAX_SAMPLERS: " << max_samplers << std::endl;
			std::cout << sLinePrefix << "MAX_PARAMETER_SIZE: " << max_parameter_size << std::endl;
			std::cout << sLinePrefix << "MEM_BASE_ADDR_ALIGN: " << mem_base_addr_align << std::endl;
			std::cout << sLinePrefix << "MIN_DATA_TYPE_ALIGN_SIZE: " << min_data_type_align_size << std::endl;
			std::cout << sLinePrefix << "SINGLE_FP_CONFIG: ";
			if (single_fp_config & CL_FP_DENORM)	std::cout << "FP_DENORM ";
			if (single_fp_config & CL_FP_INF_NAN)	std::cout << "FP_INF_NAN ";
			if (single_fp_config & CL_FP_ROUND_TO_NEAREST)	std::cout << "FP_ROUND_TO_NEAREST ";
			if (single_fp_config & CL_FP_ROUND_TO_ZERO)	std::cout << "FP_ROUND_TO_ZERO ";
			if (single_fp_config & CL_FP_ROUND_TO_INF)	std::cout << "FP_ROUND_TO_INF ";
			if (single_fp_config & CL_FP_FMA)	std::cout << "FP_FMA ";
			std::cout << std::endl;
			std::cout << sLinePrefix << std::endl;

			std::cout << sLinePrefix << "GLOBAL_MEM_CACHE_TYPE: ";
			switch(global_mem_cache_type)
			{
				case CL_NONE:			std::cout << "NONE";	break;
				case CL_READ_ONLY_CACHE:	std::cout << "CL_READ_ONLY_CACHE";	break;
				case CL_READ_WRITE_CACHE:	std::cout << "CL_READ_WRITE_CACHE";	break;
			}
			std::cout << std::endl;
			std::cout << sLinePrefix << "GLOBAL_MEM_CACHELINE_SIZE: " << global_mem_cacheline_size << std::endl;
			std::cout << sLinePrefix << "GLOBAL_MEM_CACHE_SIZE: " << global_mem_cache_size << std::endl;
			std::cout << sLinePrefix << "GLOBAL_MEM_SIZE: " << global_mem_size << " (" << (global_mem_size >> 20) << "MB)" << std::endl;
			std::cout << sLinePrefix << std::endl;
			std::cout << sLinePrefix << "MAX_CONSTANT_BUFFER_SIZE: " << max_constant_buffer_size << std::endl;
			std::cout << sLinePrefix << "MAX_CONSTANT_ARGS: " << max_constant_args << std::endl;

			std::cout << sLinePrefix << "LOCAL_MEM_TYPE: ";
			switch(local_mem_type)
			{
				case CL_LOCAL:	std::cout << "LOCAL";	break;
				case CL_GLOBAL:	std::cout << "GLOBAL";	break;
				default:	std::cout << "UNKNOWN";	break;
			}
			std::cout << std::endl;

			std::cout << sLinePrefix << "LOCAL_MEM_SIZE: " << local_mem_size << std::endl;
			std::cout << sLinePrefix << "ERROR_CORRECTION_SUPPORT: " << error_correction << std::endl;
			std::cout << sLinePrefix << "PROFILING_TIMER_RESOLUTION: " << profiling_timer_resolution << std::endl;
			std::cout << sLinePrefix << "ENDIAN_LITTLE: " << endian_little << std::endl;
			std::cout << sLinePrefix << "AVAILABLE: " << available << std::endl;
			std::cout << sLinePrefix << "COMPILER_AVAILABLE: " << compiler_available << std::endl;
			std::cout << sLinePrefix << "EXECUTION_CAPABILITIES: ";
			if (execution_capabilities & CL_EXEC_KERNEL)		std::cout << "EXEC_KERNEL ";
			if (execution_capabilities & CL_EXEC_NATIVE_KERNEL)	std::cout << "EXEC_NATIVE_KERNEL ";
			std::cout << std::endl;
			std::cout << sLinePrefix << "QUEUE_PROPERTIES: ";
			if (queue_properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)		std::cout << "QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE ";
			if (queue_properties & CL_QUEUE_PROFILING_ENABLE)	std::cout << "QUEUE_PROFILING_ENABLE";
			std::cout << std::endl;

			std::cout << sLinePrefix << std::endl;
			std::cout << sLinePrefix << "NAME: " << name << std::endl;
			std::cout << sLinePrefix << "VENDOR: " << vendor << std::endl;
			std::cout << sLinePrefix << "DRIVER_VERSION: " << driver_version << std::endl;
			std::cout << sLinePrefix << "PROFILE: " << profile << std::endl;
			std::cout << sLinePrefix << "VERSION: " << version << std::endl;
			std::cout << sLinePrefix << "EXTENSIONS: " << extensions << std::endl;
		}
	};

	/**
	 * initialize skeleton
	 */
	CCLSkeleton(bool p_verbose = false)
	{
		verbose = p_verbose;
	}


	/**
	 * initialize skeleton with existing skeleton
	 *
	 * only cDevice, cCommandQueue and cContext are copied
	 */
	CCLSkeleton(	const CCLSkeleton &cClSkeleton,
					bool p_verbose = false
	)
	{
		verbose = p_verbose;
		initCL(cClSkeleton);
	}

	/**
	 * initialize skeleton with existing skeleton
	 *
	 * the ids of cDevice, cCommandQueue and cContext are copied, not re-initialized
	 */
	void initCL(const CCLSkeleton &cClSkeleton)
	{
		*this = cClSkeleton;
	}

	/**
	 * initialize OpenCL with given parameters
	 */
	void initCL(	const cl::Context &p_cContext,			///< existing OpenCL context
					const cl::Device &p_cDevice,			///< existing OpenCL device
					const cl::CommandQueue &p_cCommandQueue	///< existing OpenCL command queue
			)
	{
		cDevice = p_cDevice;
		cContext = p_cContext;
		cCommandQueue = p_cCommandQueue;
	}

	/**
	 * initialize OpenCL skeleton
	 */
	void initCL(	int p_device_nr = -2,		///< device number (use -1 to list available devices, -2: autodetect)
					int p_platform_nr = -2		///< platform number (use -1 to list available platforms, -2: autodetect)
	)
	{
		// load platform information
		if (verbose)
			std::cout << "loading platforms" << std::endl;

		cl::Platform::get(&cPlatforms);

		if (cPlatforms.size() == 0)
		{
			throw std::runtime_error("No OpenCL Platform found!");
			return;
		}

		if (p_platform_nr == -1)
		{
			for (size_t i = 0; i < cPlatforms.size(); i++)
			{
				cl::string profile_string;
				cl::string tmp_string;
				cPlatforms[i].getInfo(CL_PLATFORM_PROFILE, &profile_string);

				std::cout << "****************************************" << std::endl;
				std::cout << "Platform " << (i) << ":" << std::endl;
				std::cout << "****************************************" << std::endl;

				cPlatforms[i].getInfo(CL_PLATFORM_NAME, &tmp_string);
				std::cout << " +       Name: " << tmp_string << std::endl;
				std::cout << " +    Profile: " << profile_string << std::endl;
				cPlatforms[i].getInfo(CL_PLATFORM_VERSION, &tmp_string);
				std::cout << " +    Version: " << tmp_string << std::endl;
				cPlatforms[i].getInfo(CL_PLATFORM_VENDOR, &tmp_string);
				std::cout << " +     Vendor: " << tmp_string << std::endl;
				cPlatforms[i].getInfo(CL_PLATFORM_EXTENSIONS, &tmp_string);
				std::cout << " + Extensions: " << tmp_string << std::endl;
				std::cout << "****************************************" << std::endl;
				std::cout << std::endl;
			}

			throw std::runtime_error("Please choose platform with option -P [platform nr]");
			return;
		}

		if (p_platform_nr == -2)
		{
			p_platform_nr = 0;
			std::cout << "Using default platform number " << p_platform_nr << std::endl;
		}

		if (p_platform_nr < 0 || p_platform_nr >= (int)cPlatforms.size())
		{
			throw std::runtime_error("invalid platform number - use option \"-P -1\" to list all devices");
			return;
		}

		cPlatform = cPlatforms[p_platform_nr];

		// load devices belonging to platform
		if (verbose)
		{
			std::cout << "Using platform " << p_platform_nr << std::endl;
			std::cout << "Loading devices for platform" << std::endl;
		}
		CL_CHECK_ERROR(cPlatform.getDevices(CL_DEVICE_TYPE_ALL, &cDevices));

		if (cDevices.size() == 0)
		{
			throw std::runtime_error("No OpenCL devices for this platform found!");
			return;
		}

		if (p_device_nr == -1)
		{
			// list available devices
			for (int i = 0; i < (int)cDevices.size(); i++)
			{
				cl::string tmp_string;
				CDeviceInfo cDeviceInfo(cDevices[i]);

				std::cout << std::endl;
				std::cout << "****************************************" << std::endl;
				std::cout << "Device " << (i) << ":" << std::endl;
				std::cout << "****************************************" << std::endl;
				std::cout << " +       Type: " << cDeviceInfo.getTypeString() << std::endl;
				std::cout << " +       Name: " << cDeviceInfo.name << std::endl;
				std::cout << " +    Profile: " << cDeviceInfo.profile << std::endl;
				std::cout << " +    Version: " << cDeviceInfo.version << std::endl;
				std::cout << " +     Vendor: " << cDeviceInfo.vendor << std::endl;
				std::cout << " + Extensions: " << cDeviceInfo.extensions << std::endl;
				std::cout << "****************************************" << std::endl;
				std::cout << std::endl;
			}

			throw std::runtime_error("Please choose device with option -D [device nr]");
		}

		if (p_device_nr == -2)
		{
			p_device_nr = 0;
			std::cout << "Using default device number " << p_device_nr << std::endl;
		}

		if (p_device_nr < 0 || p_device_nr >= (int)cDevices.size())
		{
			throw std::runtime_error("invalid device number - use option \"-D -1\" to list all devices");
			return;
		}

		cDevice = cDevices[p_device_nr];

		cl_int err;

		// load standard context for devices
		if (verbose)	std::cout << "loading context" << std::endl;
		std::vector<cl::Device> tmp_devices(1, cDevice);

		cl_context_properties properties[4] = {CL_CONTEXT_PLATFORM, (cl_context_properties)cPlatform(), 0, 0};

		cContext = cl::Context(tmp_devices, properties, 0, 0, &err);
		CL_CHECK_ERROR(err);

		// initialize queue
		if (verbose)	std::cout << "creating command queue" << std::endl;

		cCommandQueue = cl::CommandQueue(cContext, cDevice, 0, &err);
		CL_CHECK_ERROR(err);
	}



#ifdef C_GL_TEXTURE_HPP

	/**
	 * initialize OpenCL context from existing OpenGL context
	 */
	void initCLFromGLContext(	int p_device_nr = -1,		///< device number (use -1 to list available devices)
								int p_platform_nr = -1		///< platform number (use -1 to list available platforms)
	)
	{
		// load platform information
		if (verbose)	std::cout << "loading platforms" << std::endl;
		cl::Platform::get(&cPlatforms);

		if (cPlatforms.size() == 0)
		{
			error << "no platform found!" << std::endl;
			return;
		}

		if (p_platform_nr == -1)
		{
			for (size_t i = 0; i < cPlatforms.size(); i++)
			{
				cl::string profile_string;
				cl::string tmp_string;
				cPlatforms[i].getInfo(CL_PLATFORM_PROFILE, &profile_string);
				if (verbose)
				{
					std::cout << "Platform " << (i) << ":" << std::endl;

					cPlatforms[i].getInfo(CL_PLATFORM_NAME, &tmp_string);
					std::cout << "        Name: " << tmp_string << std::endl;
					std::cout << "     Profile: " << profile_string << std::endl;
					cPlatforms[i].getInfo(CL_PLATFORM_VERSION, &tmp_string);
					std::cout << "     Version: " << tmp_string << std::endl;
					cPlatforms[i].getInfo(CL_PLATFORM_VENDOR, &tmp_string);
					std::cout << "      Vendor: " << tmp_string << std::endl;
					cPlatforms[i].getInfo(CL_PLATFORM_EXTENSIONS, &tmp_string);
					std::cout << "  Extensions: " << tmp_string << std::endl;
					std::cout << std::endl;
				}

				/**
				 * autodetect platform
				 */
				if (p_platform_nr == -1)
					if (profile_string == "FULL_PROFILE")
						p_platform_nr = i;

				if (verbose && (p_platform_nr == (int)i))
				{
					std::cout << ">>> Using Platform " << (i) << " for simulation" << std::endl;
					std::cout << std::endl;
				}
			}
		}

		if (p_platform_nr == -1)
		{
			error << "no usable platform found... exiting" << std::endl;
			return;
		}

		if (p_platform_nr < 0 || p_platform_nr >= (int)cPlatforms.size())
		{
			error << "invalid platform number - use option \"-P -1\" and verbose mode \"-v\" to list all devices" << std::endl;
			return;
		}

		cPlatform = cPlatforms[p_platform_nr];

		/*
		 * load context which belongs to current GL context
		 */
#ifdef WIN32
#error TODO
		// TODO: test this
		HGLRC wglContext = wglGetCurrentContext();
		if (wglContext == NULL)
		{
			error << "no current wgl context found" << std::endl;
			return false;
		}

		HDC display = wglGetCurrentDC();
		if (display == NULL)
		{
			error << "no current wgl display found" << std::endl;
			return false;
		}

		cl_context_properties context_properties[] = {
						CL_CONTEXT_PLATFORM, (cl_context_properties)cPlatform.platform_id,
						CL_GL_CONTEXT_KHR,	(cl_context_properties)wglContext,
						CL_WGL_HDC_KHR, 	(cl_context_properties)display,
						NULL, NULL
				};
#endif


#ifdef MACOSX
	#error TODO
#endif

#ifdef __unix
		GLXContext glxContext = glXGetCurrentContext();
		if (glxContext == NULL)
		{
			error << "no current glx context found" << std::endl;
			return;
		}

		Display *display = glXGetCurrentDisplay();
		if (display == NULL)
		{
			error << "no current glx display found" << std::endl;
			return;
		}

		cl_context_properties context_properties[] = {
						CL_CONTEXT_PLATFORM, (cl_context_properties)cPlatform(),	// TODO: maybe () does not return the id
						CL_GL_CONTEXT_KHR,	(cl_context_properties)glxContext,
						CL_GLX_DISPLAY_KHR,	(cl_context_properties)display,
						0, 0
				};
#endif

		cl_int err;
		cContext = cl::Context(CL_DEVICE_TYPE_GPU, context_properties, NULL, NULL, &err);
		CL_CHECK_ERROR(err);
		cDevices = cContext.getInfo<CL_CONTEXT_DEVICES>();

		if (cDevices.size() == 0)
		{
			error << "no devices found - aborting" << std::endl;
			return;
		}

		if (p_device_nr == -1)
		{
			// list available devices
			for (int i = 0; i < (int)cDevices.size(); i++)
			{
				CDeviceInfo cDeviceInfo(cDevices[i]);
				if (verbose)
				{
					std::cout << "Device " << (i) << ":" << std::endl;
					std::cout << "        Type: " << cDeviceInfo.getTypeString() << std::endl;
					std::cout << "        Name: " << cDeviceInfo.name << std::endl;
					std::cout << "     Profile: " << cDeviceInfo.profile << std::endl;
					std::cout << "     Version: " << cDeviceInfo.version << std::endl;
					std::cout << "      Vendor: " << cDeviceInfo.vendor << std::endl;
					std::cout << "  Extensions: " << cDeviceInfo.extensions << std::endl;
					std::cout << std::endl;
				}

				if (p_device_nr == -1)
					if (cDeviceInfo.device_type == CL_DEVICE_TYPE_GPU)
						p_device_nr = i;

				if (verbose && (p_device_nr == i))
				{
					std::cout << ">>> Using Device " << (i) << " for simulation" << std::endl;
					std::cout << std::endl;
				}
			}
		}

		if (p_device_nr < 0 || p_device_nr >= (int)cDevices.size())
		{
			error << "invalid device number - use option \"-D -1\" to list all devices" << std::endl;
			return;
		}

		cDevice = cDevices[p_device_nr];

		// initialize queue
		if (verbose)	std::cout << "creating command queue" << std::endl;
		cCommandQueue = cl::CommandQueue(cContext, cDevice);
	}
#endif
};

#endif /* CCLSKELETON_HPP_ */
