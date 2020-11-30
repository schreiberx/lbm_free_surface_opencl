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


#ifndef CLBMINTERFACE_HPP
#define CLBMINTERFACE_HPP


#include "libopencl/CCLSkeleton.hpp"
#include "libmath/CMath.hpp"
#include "libmath/CVector.hpp"
#include "lib/CError.hpp"
#include "lbm/CLbmParameters.hpp"
#include <typeinfo>
#include <iomanip>
#include <list>


/**
 * \brief interface for lattice boltzmann simulation to implement different lattice boltzmann versions
 *
 * some general informations:
 *
 * the density distributions are packed cell wise to optimize the collision operation and stored
 * linearly (first x, then y, then z) in the buffer cMemDensityDistributions
 *
 * this implementation is using the D3Q19 implementation
 *
 *
 * D3Q19 Vector enumerations + directions (similar to LBM OpenGL implementation [kaufmann et al.]):
 * <pre>
 * f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0)	// 0-3
 * f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0)	// 4-7
 * f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1)	// 8-11
 * f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1)	// 12-15
 * f(0,0,1), f(0,0,-1),  f(0,0,0),  X			// 16-18
 * </pre>
 *
 * velocity(vx, vy, vz, phi)
 *
 * <pre>
 *       14,12
 *     15  |  18
 *       \ | /
 *        \|/
 * 7,9----- ----- 8,10
 *        /|\
 *       / | \
 *     17  |  16
 *       11,13
 * </pre>
 */


template <typename T>
class CLbmOpenClInterface
{
public:
	CCLSkeleton	cl;					///< OpenCL Skeleton
	CLbmParameters<T>	params;		///< LBM Skeleton
	bool verbose;					///< true, if verbose mode is active
	CError error;					///< error handler

	size_t simulation_step_counter;	///< number of simulation steps done so far

	int fluid_init_flags;			///< flags to control how the fluid domain is initialized

	static const size_t SIZE_DD_HOST = 19;
	static const size_t SIZE_DD_HOST_BYTES = SIZE_DD_HOST*sizeof(T);

	size_t domain_cells_count;

	std::ostringstream cl_interface_program_defines;

	float simulation_mass_on_reset;		///< simulation mass on fluid reset

	std::list<int> lbm_opencl_number_of_threads_list;	///< list with number of threads for each successively created kernel
	std::list<int> lbm_opencl_number_of_registers_list;	///< list with number of registers for each thread threads for each successively created kernel


	/**
	 * FLAGS, FLAGS, FLAGS
	 */
	enum
	{
		LBM_FLAG_OBSTACLE			= (1<<0),
		LBM_FLAG_FLUID				= (1<<1),
		LBM_FLAG_INTERFACE			= (1<<2),
		LBM_FLAG_GAS				= (1<<3),

		LBM_FLAG_INTERFACE_TO_FLUID	= (1<<4),
		LBM_FLAG_INTERFACE_TO_GAS	= (1<<5),
		LBM_FLAG_GAS_TO_INTERFACE	= (1<<6)
	};

	/**
	 * initialization flags (insert fluids or obstacle cells)
	 */
	enum
	{
		INIT_CREATE_BREAKING_DAM			= (1<<0),
		INIT_CREATE_POOL					= (1<<1),
		INIT_CREATE_SPHERE					= (1<<2),
		INIT_CREATE_OBSTACLE_HALF_SPHERE	= (1<<3),
		INIT_CREATE_OBSTACLE_VERTICAL_BAR	= (1<<4),
		INIT_CREATE_FLUID_WITH_GAS_SPHERE	= (1<<5),
		INIT_CREATE_FILLED_CUBE				= (1<<6)
	};



	/**
	 * mandatory memory buffers
	 */
	cl::Buffer cMemDensityDistributions;	///< density distributions (D3Q19 model)
	cl::Buffer cMemCellFlags;			///< buffer with flags to setup e. g. obstacles, (maybe gas) or other properties

	cl::Buffer cMemVelocity;				///< buffer with velocity (3 components for each cell)
	cl::Buffer cMemDensity;				///< buffer with density values

	cl::Buffer cMemFluidMass;			///< buffer for mass within a cell
	cl::Buffer cMemFluidFraction;		///< fluid-gas fraction within a cell

	cl::Buffer cMemNewFluidFraction;		///< buffer with new fluid fractions
	cl::Buffer cMemNewCellFlags;			///< buffer with new flags

	/**
	 * OpenCL interops to OpenGL context
	 */
#ifdef LBM_OPENCL_GL_INTEROP
	cl::Image3DGL cMemFluidFractionTexture;		///< memory buffer directed to OpenGL volume texture
	cl::Image2DGL cMemFluidFractionFlatTexture;	///< memory buffer directed to OpenGL raw flat texture

	cl::size_t<3> fluid_fraction_flat_texture_copy1_region;	///< size of texture area to copy to (1st area)
	cl::size_t<3> fluid_fraction_flat_texture_copy1_origin;	///< origin of texture area to copy to (1st area)

	cl::size_t<3> fluid_fraction_flat_texture_copy2_region;	///< size of texture area to copy to (2nd area [last slices])
	cl::size_t<3> fluid_fraction_flat_texture_copy2_origin;	///< origin of texture area to copy to (2nd area)


	/**
	 * initialize memory object for fluid fraction for GL_INTEROPs
	 */
	void initFluidFractionMemObject(CGlTexture &fluid_fraction_texture)
	{
		// get handler to fluid fraction texture
		cl_uint err;
		cMemFluidFractionTexture = cl::BufferGL(cl.cContext, CL_MEM_READ_WRITE, fluid_fraction_texture.textureid, err);
		if (err != CL_SUCCESS)	error << "failed to initialize memory object for fluid fraction for GL_INTEROPs" << CError::endl;
	}

	/**
	 * initialize memory object from 2d opengl texture
	 *
	 * the format of the raw texture has to be RGBA!
	 * raw means, that the data is copied directly and has to be converted to the flat texture layout afterwards
	 */
	void initFluidFractionMemObject2D(CGlTexture &fluid_fraction_raw_texture)
	{
		// because all color channels are used the size is 4 times larger
		if (4 * fluid_fraction_raw_texture.width * fluid_fraction_raw_texture.height == (int)domain_cells_count)
		{
			if (verbose)
				std::cout << "gl_interops: only one copy buffer2texture needed (" << fluid_fraction_raw_texture.width << " * " << fluid_fraction_raw_texture.height << " == " << (int)domain_cells_count << ")" << std::endl;;

			fluid_fraction_flat_texture_copy1_origin[0] = 0;
			fluid_fraction_flat_texture_copy1_origin[1] = 0;
			fluid_fraction_flat_texture_copy1_origin[2] = 0;

			fluid_fraction_flat_texture_copy1_region[0] = fluid_fraction_raw_texture.width;
			fluid_fraction_flat_texture_copy1_region[1] = fluid_fraction_raw_texture.height;
			fluid_fraction_flat_texture_copy1_region[2] = 1;


			fluid_fraction_flat_texture_copy2_origin[0] = 0;
			fluid_fraction_flat_texture_copy2_origin[1] = 0;
			fluid_fraction_flat_texture_copy2_origin[2] = 0;

			fluid_fraction_flat_texture_copy2_region[0] = 0;
			fluid_fraction_flat_texture_copy2_region[1] = 0;
			fluid_fraction_flat_texture_copy2_region[2] = 0;
		}
		else
		{
			/*
			 * the size of the texture does not match the size of the buffer
			 * => split it up into 2 copy operations
			 */

			if (verbose)
				std::cout << "gl_interops: 2 copy buffer2texture needed..." << std::endl;

			fluid_fraction_flat_texture_copy1_origin[0] = 0;
			fluid_fraction_flat_texture_copy1_origin[1] = 0;
			fluid_fraction_flat_texture_copy1_origin[2] = 0;

			fluid_fraction_flat_texture_copy1_region[0] = fluid_fraction_raw_texture.width;
			fluid_fraction_flat_texture_copy1_region[1] = (domain_cells_count/4)/fluid_fraction_raw_texture.width;
			fluid_fraction_flat_texture_copy1_region[2] = 1;


			fluid_fraction_flat_texture_copy2_origin[0] = 0;
			fluid_fraction_flat_texture_copy2_origin[1] = fluid_fraction_flat_texture_copy1_region[1];
			fluid_fraction_flat_texture_copy2_origin[2] = 0;

			fluid_fraction_flat_texture_copy2_region[0] = (domain_cells_count/4) - fluid_fraction_raw_texture.width*fluid_fraction_flat_texture_copy1_region[1];
			fluid_fraction_flat_texture_copy2_region[1] = 1;
			fluid_fraction_flat_texture_copy2_region[2] = 1;
		}

		// get handler to fluid fraction texture
		cl_int err;
		cMemFluidFractionFlatTexture = cl::Image2DGL(cl.cContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, fluid_fraction_raw_texture.textureid, &err);
		CL_CHECK_ERROR(err);
	}

	/**
	 * store fluid fraction from memory buffer to opengl 2d flat texture buffer
	 */
	void loadFluidFractionToRawFlatTexture()
	{
		std::vector<cl::Memory> glObjects(1, cMemFluidFractionFlatTexture);

		CL_CHECK_ERROR(cl.cCommandQueue.enqueueAcquireGLObjects(&glObjects));
		cl.cCommandQueue.enqueueBarrier();

		CL_CHECK_ERROR(cl.cCommandQueue.enqueueCopyBufferToImage(	getFluidFractionMemObject(),
													cMemFluidFractionFlatTexture,
													0,
													fluid_fraction_flat_texture_copy1_origin,
													fluid_fraction_flat_texture_copy1_region
												));

		if (fluid_fraction_flat_texture_copy2_region[0] != 0)
		{
			CL_CHECK_ERROR(cl.cCommandQueue.enqueueCopyBufferToImage(	getFluidFractionMemObject(),
														cMemFluidFractionFlatTexture,
														0,
														fluid_fraction_flat_texture_copy2_origin,
														fluid_fraction_flat_texture_copy2_region
													));
		}

		cl.cCommandQueue.enqueueBarrier();
		cl.cCommandQueue.enqueueReleaseGLObjects(&glObjects);
		cl.cCommandQueue.enqueueBarrier();
		cl.cCommandQueue.finish();
		glFinish();
	}

	/**
	 * store fluid fraction from memory buffer to OpenGL 3d texture buffer
	 */
	void loadFluidFractionToTexture()
	{
		std::vector<cl::Memory> glObjects(1, cMemFluidFractionTexture);

		cl.cCommandQueue.enqueueAcquireGLObjects(&glObjects);
		cl.cCommandQueue.enqueueBarrier();

		const size_t origin[3] = {0,0,0};
		const size_t domain_cells[3] = {domain_cells[0], domain_cells[1], domain_cells[2]};

		CL_CHECK_ERROR(cl.cCommandQueue.enqueueCopyBufferToImage(	getFluidFractionMemObject(),
													cMemFluidFractionTexture,
													0,
													origin, domain_cells));

		cl.cCommandQueue.enqueueBarrier();
		cl.cCommandQueue.enqueueReleaseGLObjects(&glObjects);
		cl.cCommandQueue.enqueueBarrier();
		cl.cCommandQueue.finish();
		glFinish();
	}
#endif

	/**
	 * Setup class with OpenCL skeleton.
	 *
	 * No simulation kernels are loaded in this constructor!
	 */
	CLbmOpenClInterface(	const CCLSkeleton &cClSkeleton,
							bool p_verbose = false
	)	:
		cl(cClSkeleton),
		params(p_verbose),
		verbose(p_verbose)
	{
	}

	virtual ~CLbmOpenClInterface()
	{
#if 0
		if (cl.cCommandQueue.error())
			std::cout << cl.cCommandQueue.error.getString() << std::endl;
#endif
	}


	/**
	 * initialize simulation with given parameters
	 */
	void initInterface(
				CVector<3,int> &p_domain_cells,		///< domain cells in each dimension
				T p_d_domain_x_length,				///< domain length in x direction
				T p_d_viscosity,					///< viscocity of fluid
				CVector<3,T> &p_d_gravitation,		///< gravitation
				T p_max_sim_gravitation_length,		///< maximum length of gravitation to limit timestep
				T p_d_timestep,						///< timestep for one simulation step - computed automagically
				T p_mass_exchange_factor,			///< mass exchange

				std::list<int> &p_lbm_opencl_number_of_threads_list,		///< list with number of threads for each successively created kernel
				std::list<int> &p_lbm_opencl_number_of_registers_list		///< list with number of registers for each thread threads for each successively created kernel
		)
	{
		lbm_opencl_number_of_threads_list = p_lbm_opencl_number_of_threads_list;
		lbm_opencl_number_of_registers_list = p_lbm_opencl_number_of_registers_list;

		params.initSkeleton(	p_domain_cells,
								p_d_domain_x_length,
								p_d_viscosity,
								p_d_gravitation,
								p_max_sim_gravitation_length,
								p_d_timestep,
								p_mass_exchange_factor
							);

		if (params.error())
		{
			error << params.error.getString();
			return;
		}
	}



	/**
	 * get memory reference to OpenCL fluid fraction buffer
	 */
    cl::Buffer &getFluidFractionMemObject()
    {
    	if (simulation_step_counter & 1)
    		return cMemNewFluidFraction;
    	else
    		return cMemFluidFraction;
    }

    /**
     * update the domain size during the simulation
     */
	void updateDomainXLength(	T p_domain_x_length	///< domain length in x axis
	)
	{
		params.setDomainXLength(p_domain_x_length);
		params.computeParametrization();

		setKernelArguments();
	}


    /**
     * update the viscosity during the simulation
     */
	void updateViscosity(	T p_viscosity	///< viscosity
	)
	{
		params.setViscosity(p_viscosity);
		params.computeParametrization();

		setKernelArguments();
	}

    /**
     * update the mass exchange factor
     */
	void updateMassExchangeFactor(	T p_mass_exchange_factor	///< mass exchange factor
	)
	{
		params.setMassExchangeFactor(p_mass_exchange_factor);
		params.computeParametrization();

		setKernelArguments();
	}


    /**
     * update the gravitation vector:
     *
     * parametrization and setting kernel arguments
     */
	void updateGravitation(	CVector<3,T> p_gravitation	///< gravitation vector with dimensions
	)
	{
		params.setGravitation(p_gravitation);
		params.computeParametrization();

		setKernelArguments();
	}

	/**
	 * reload method for interface
	 *
	 * e. g. used to initialize default #defines for opencl kernels, allocate memory buffers, etc.
	 */
	void reloadInterface()
	{
		domain_cells_count = params.domain_cells.elements();

		/*
		 * ALLOCATE BUFFERS
		 */
		cl_int err;
		cMemDensityDistributions = cl::Buffer(cl.cContext,	CL_MEM_READ_WRITE, sizeof(T)*domain_cells_count*SIZE_DD_HOST, NULL, &err);	CL_CHECK_ERROR(err);
		cMemCellFlags = cl::Buffer(cl.cContext,	CL_MEM_READ_WRITE, sizeof(cl_int)*domain_cells_count, NULL, &err);		CL_CHECK_ERROR(err);
		cMemVelocity = cl::Buffer(cl.cContext,	CL_MEM_READ_WRITE, sizeof(T)*domain_cells_count*3, NULL, &err);			CL_CHECK_ERROR(err);
		cMemDensity = cl::Buffer(cl.cContext,	CL_MEM_READ_WRITE, sizeof(T)*domain_cells_count, NULL, &err);			CL_CHECK_ERROR(err);
		cMemFluidMass = cl::Buffer(cl.cContext,	CL_MEM_READ_WRITE, sizeof(T)*domain_cells_count, NULL, &err);			CL_CHECK_ERROR(err);
		cMemFluidFraction = cl::Buffer(cl.cContext,	CL_MEM_READ_WRITE, sizeof(T)*domain_cells_count, NULL, &err);		CL_CHECK_ERROR(err);
		cMemNewFluidFraction = cl::Buffer(cl.cContext,	CL_MEM_READ_WRITE, sizeof(T)*domain_cells_count, NULL, &err);	CL_CHECK_ERROR(err);
		cMemNewCellFlags = cl::Buffer(cl.cContext,	CL_MEM_READ_WRITE, sizeof(cl_int)*domain_cells_count, NULL, &err);	CL_CHECK_ERROR(err);

		/*
		 * create #define precompiler directives for opencl kernels
		 */
		cl_interface_program_defines.str("");
		cl_interface_program_defines.clear();

		cl_interface_program_defines << "#define SIZE_DD_HOST_BYTES (" << this->SIZE_DD_HOST_BYTES << ")" << std::endl;

		if (typeid(T) == typeid(float))
		{
			cl_interface_program_defines << "typedef float T;" << std::endl;
			cl_interface_program_defines << "typedef float2 T2;" << std::endl;
			cl_interface_program_defines << "typedef float4 T4;" << std::endl;
			cl_interface_program_defines << "typedef float8 T8;" << std::endl;
			cl_interface_program_defines << "typedef float16 T16;" << std::endl;
		}
		else if (typeid(T) == typeid(double))
		{
			cl_interface_program_defines << "#pragma OPENCL EXTENSION cl_khr_fp64 : enable" << std::endl;
			cl_interface_program_defines << "#ifndef cl_khr_fp64" << std::endl;
			cl_interface_program_defines << "	#error cl_khr_fp64 not supported - please switch to single precision to run the simulation" << std::endl;
			cl_interface_program_defines << "#endif cl_khr_fp64" << std::endl;

			cl_interface_program_defines << "typedef double T;" << std::endl;
			cl_interface_program_defines << "typedef double2 T2;" << std::endl;
			cl_interface_program_defines << "typedef double4 T4;" << std::endl;
			cl_interface_program_defines << "typedef double8 T8;" << std::endl;
			cl_interface_program_defines << "typedef double16 T16;" << std::endl;
		}
		else
		{
			this->error << "unsupported class type T" << std::endl;
			return;
		}

		cl_interface_program_defines << "#define DOMAIN_CELLS_X	(" << params.domain_cells[0] << ")" << std::endl;
		cl_interface_program_defines << "#define DOMAIN_CELLS_Y	(" << params.domain_cells[1] << ")" << std::endl;
		cl_interface_program_defines << "#define DOMAIN_CELLS_Z	(" << params.domain_cells[2] << ")" << std::endl;

		cl_interface_program_defines << "#define FLAG_GAS	(" << CLbmOpenClInterface<T>::LBM_FLAG_GAS << ")" << std::endl;
		cl_interface_program_defines << "#define FLAG_INTERFACE	(" << CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE << ")" << std::endl;
		cl_interface_program_defines << "#define FLAG_FLUID	(" << CLbmOpenClInterface<T>::LBM_FLAG_FLUID << ")" << std::endl;

		cl_interface_program_defines << "#define FLAG_INTERFACE_TO_FLUID	(" << (CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE_TO_FLUID) << ")" << std::endl;
		cl_interface_program_defines << "#define FLAG_INTERFACE_TO_GAS	(" << (CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE_TO_GAS) << ")" << std::endl;
		cl_interface_program_defines << "#define FLAG_GAS_TO_INTERFACE	(" << (CLbmOpenClInterface<T>::LBM_FLAG_GAS_TO_INTERFACE) << ")" << std::endl;

		cl_interface_program_defines << "#define FLAG_OBSTACLE	(" << CLbmOpenClInterface<T>::LBM_FLAG_OBSTACLE << ")" << std::endl;

	}

	/**
	 * reset the interface datastructured
	 *
	 * THIS METHOD HAS TO BE CALLED AFTER RESETTING THE FLUID!!!
	 */
	void resetFluid_Interface()
	{
		simulation_step_counter = 0;

		simulation_mass_on_reset = this->getMassReduction();
	}



	/**
	 * wait until all kernels have finished
	 */
	void wait()
	{
		this->cl.cCommandQueue.finish();
	}

	/**
	 * store velocity and density values to host memory
	 * this is useful for a host memory based visualization
	 */
	void storeVelocity(T *dst)
	{
		size_t byte_size;
		this->cMemVelocity.getInfo(CL_MEM_SIZE, &byte_size);

		wait();
		CL_CHECK_ERROR(this->cl.cCommandQueue.enqueueReadBuffer(
											this->cMemVelocity,
											CL_TRUE,	// sync reading
											0,
											byte_size,
											dst));
		this->cl.cCommandQueue.enqueueBarrier();
	}

	/**
	 * store the density to the allocated host memory 'dst'
	 */
	void storeDensity(T *dst)
	{
		size_t byte_size;
		this->cMemDensity.getInfo(CL_MEM_SIZE, &byte_size);

		wait();
		CL_CHECK_ERROR(this->cl.cCommandQueue.enqueueReadBuffer(
											this->cMemDensity,
											CL_TRUE,	// sync reading
											0,
											byte_size,
											dst));
		this->cl.cCommandQueue.enqueueBarrier();
	}

	/**
	 * store the density distributions to the allocated host memory 'dst'
	 */
	void storeDensityDistributions(T *dst)
	{
		size_t byte_size;
		this->cMemDensityDistributions.getInfo(CL_MEM_SIZE, &byte_size);

		wait();
		CL_CHECK_ERROR(this->cl.cCommandQueue.enqueueReadBuffer(
											this->cMemDensityDistributions,
											CL_TRUE,	// sync reading
											0,
											byte_size,
											dst));
		this->cl.cCommandQueue.enqueueBarrier();
	}

	/**
	 * store the mass to the allocated host memory 'dst'
	 */
	void storeMass(T *dst)
	{
		size_t byte_size;
		this->cMemFluidMass.getInfo(CL_MEM_SIZE, &byte_size);

		wait();
		CL_CHECK_ERROR(this->cl.cCommandQueue.enqueueReadBuffer(
											this->cMemFluidMass,
											CL_TRUE,	// sync reading
											0,
											byte_size,
											dst));
		this->cl.cCommandQueue.enqueueBarrier();
	}

	/**
	 * store the fluid fraction to the allocated host memory 'dst'
	 */
	void storeFraction(T *dst)
	{
		size_t byte_size;
		this->cMemFluidFraction.getInfo(CL_MEM_SIZE, &byte_size);

		wait();
		if (this->simulation_step_counter & 1)
		{
			CL_CHECK_ERROR(this->cl.cCommandQueue.enqueueReadBuffer(
												this->cMemNewFluidFraction,
												CL_TRUE,	// sync reading
												0,
												byte_size,
												dst));
		}
		else
		{
			CL_CHECK_ERROR(this->cl.cCommandQueue.enqueueReadBuffer(
												this->cMemFluidFraction,
												CL_TRUE,	// sync reading
												0,
												byte_size,
												dst));
		}

		this->cl.cCommandQueue.enqueueBarrier();
	}

	/**
	 * store the flags to the allocated host memory 'dst'
	 */
	void storeFlags(cl_int *dst)
	{
		size_t byte_size;
		this->cMemCellFlags.getInfo(CL_MEM_SIZE, &byte_size);

		wait();
		if (this->simulation_step_counter & 1)
		{
			CL_CHECK_ERROR(this->cl.cCommandQueue.enqueueReadBuffer(
												this->cMemNewCellFlags,
												CL_TRUE,	// sync reading
												0,
												byte_size,
												dst));
		}
		else
		{
			CL_CHECK_ERROR(this->cl.cCommandQueue.enqueueReadBuffer(	this->cMemCellFlags,
												CL_TRUE,	// sync reading
												0,
												byte_size,
												dst));
		}

		this->cl.cCommandQueue.enqueueBarrier();
	}

public:

	/**
	 * return the sum of all mass values
	 *
	 * TODO: speedup with OpenCL reduction
	 */
	T getMassReduction()
	{
		T *tmpbuffer = new T[this->params.domain_cells.elements()];
		storeMass(tmpbuffer);

		T sum = (T)0.0;
		for (int a = 0; a < this->params.domain_cells.elements(); a++)
			sum += tmpbuffer[a];

		delete [] tmpbuffer;

		return sum;
	}

	/**
	 * return the checksum over the velocity field for all valid fluid cells (FLAG is FLIUD or INTERFACE)
	 */
	float getVelocityChecksum()
	{
		float *velocity = new T[domain_cells_count*3];
		storeVelocity(velocity);

		int *flags = new int[domain_cells_count*3];
		storeFlags(flags);
		wait();

		T *velx = velocity;
		T *vely = velocity+domain_cells_count;
		T *velz = velocity+domain_cells_count*2;

		float checksum = 0.0;
		for (int a = 0; a < (int)domain_cells_count; a++)
			if (flags[a] == CLbmOpenClInterface<T>::LBM_FLAG_FLUID || flags[a] == CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE)
				checksum += velx[a] + vely[a] + velz[a];

		delete [] velocity;
		delete [] flags;

		return checksum;
	}

	/**
	 * return the maximum velocity of all fluid cells
	 */
	T getMaxVelocity()
	{
		T *velocity = new T[params.domain_cells.elements()*3];
		storeVelocity(velocity);

		int *flags = new int[params.domain_cells.elements()*3];
		storeFlags(flags);
		wait();

		T max_vel_2 = 0.0;
		T *velx = velocity;
		T *vely = velocity+params.domain_cells.elements();
		T *velz = velocity+params.domain_cells.elements()*2;

		for (int a = 0; a < params.domain_cells.elements(); a++)
			if (flags[a] == CLbmOpenClInterface<T>::LBM_FLAG_FLUID || flags[a] == CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE)
			{
				T cur_vel = velx[a]*velx[a] + vely[a]*vely[a] + velz[a]*velz[a];
				if (max_vel_2 < cur_vel)
					max_vel_2 = cur_vel;
			}

		delete [] velocity;
		delete [] flags;

		return max_vel_2;
	}

	/**
	 * return the checksum over the density for all valid fluid cells (FLAG is FLIUD or INTERFACE)
	 */
	float getDensityChecksum()
	{
		float *density = new T[domain_cells_count];
		storeDensity(density);

		int *flags = new int[domain_cells_count*3];
		storeFlags(flags);
		wait();

		float checksum = 0.0;
		for (int a = 0; a < (int)domain_cells_count; a++)
			if (flags[a] == CLbmOpenClInterface<T>::LBM_FLAG_FLUID || flags[a] == CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE)
				checksum += density[a];

		delete [] density;
		delete [] flags;

		return checksum;
	}


	/**
	 * setup the initialization flags
	 */
	void setupInitFlags(int p_fluid_init_flags)
	{
		fluid_init_flags = p_fluid_init_flags;
	}



	/**
	 * update the arguments for the kernels
	 *
	 * this method is usually called after updating gravitation, viscosity or other parameters or during the initialization/reset
	 */
    virtual void setKernelArguments() = 0;

	/**
	 * reset the fluid to it's initial state
	 */
	virtual void resetFluid() = 0;

	/**
	 * start one simulation step (enqueue kernels)
	 */
	virtual void simulationStep() = 0;

	/**
	 * reload the simulation programs
	 */
	virtual void reload() = 0;

	/**
	 * normalize the simulation mass to the amount of the initial mass (EXPERIMENTAL!)
	 */
	void normalizeSimulationMass()
	{
		T new_sim_mass = this->getMassReduction();

		T scale = simulation_mass_on_reset / new_sim_mass;

		scaleMass(scale);
	}

	/**
	 * scale the mass in each cell to stabilize the overall amount of mass in the simulation
	 */
	virtual void scaleMass(T mass_scale_factor) = 0;

	/**
	 * initialize simulation with given parameters
	 */
	virtual void init(	CVector<3,int> &p_domain_cells,	///< domain cells in each dimension
				T p_d_domain_x_length,					///< domain length in x direction
				T p_d_viscosity,						///< viscocity of fluid
				CVector<3,T> &p_d_gravitation,			///< gravitation
				T p_max_sim_gravitation_length,			///< maximum length of gravitation to limit timestep
				T p_d_timestep ,						///< timestep for one simulation step (always set to -1 for automatic computation)
				T p_mass_exchange_factor,				///< mass exchange

				size_t p_max_local_work_group_size,		///< maximum count of computation kernels (threads) used on gpu

				int init_flags,							///< flags for first initialization

				std::list<int> &lbm_opencl_number_of_threads_list,	///< list with number of threads for each successively created kernel
				std::list<int> &lbm_opencl_number_of_registers_list		///< list with number of registers for each thread threads for each successively created kernel
		) = 0;


#if 0
	/**
	 * return the maximum work group size which has to be a factor of domain_cells_count
	 *
	 * THIS IS AN HIGHLY EXPERIMENTAL FEATURE AND WOULD FAIL MOST TIMES!!! don't use it...
	 */
	size_t getMaxWorkGroupSize(
					size_t max_local_work_group_size,	///< desired local work group size
					bool output_information = false		///< set to true, to be verbose
	)
	{
		/**
		 * return 32 cz. this is just for testing purposes
		 */
		if (max_local_work_group_size == 0)
			return 32;

		size_t local_work_group_size = max_local_work_group_size;

		if (this->domain_cells_count < local_work_group_size)
		{
			local_work_group_size = this->domain_cells_count;
			if (output_information)
				std::cerr << "unable to use full local_work_group_size (" << max_local_work_group_size << ") for kernel => reducing to " << local_work_group_size << std::endl;
		}

		if (this->domain_cells_count % local_work_group_size != 0)
		{
			local_work_group_size = CMath<size_t>::gcd(local_work_group_size, this->domain_cells_count);
			if (output_information)
				std::cerr << "unable to use full local_work_group_size (" << max_local_work_group_size << ") for kernel => reducing to " << local_work_group_size << std::endl;
		}

		return local_work_group_size;
	}
#endif

	/**
	 * utility fuction: load a program
	 */
	void loadProgram(	cl::Program &ccl_program,					///< reference to cl program handler to load program to
						const cl::NDRange &ccl_kernel_work_group_size,	///< work group size for initialization
						const int ccl_kernel_max_registers,			///< max amount of registers to restrict for a work item
						const std::string &default_program_defines,	///< default program defines
						const char *filename,						///< filename of program
						bool test_local_work_group_size				///< EXPERIMENTAL feature (and thus uncommented)
	)
	{
		if (this->verbose)
			std::cout << "loading kernel " << filename << std::endl;

		std::ostringstream ccl_program_defines_param;
		ccl_program_defines_param << default_program_defines;
//		ccl_program_defines_param << "#define LOCAL_WORK_GROUP_SIZE	(" << getMaxWorkGroupSize(ccl_kernel_work_group_size, !test_local_work_group_size) << ")" << std::endl;
		ccl_program_defines_param << "#define LOCAL_WORK_GROUP_SIZE	(" << ccl_kernel_work_group_size[0] << ")" << std::endl;

		/*
		 * load program by #include preprocessor directive
		 */
		std::string source = ccl_program_defines_param.str();
		source += "\n";
		source += "#include \"";
		source += filename;
		source += "\"";

		cl::Program::Sources sources(1, std::make_pair(source.c_str(), source.size()));;
		ccl_program = cl::Program(this->cl.cContext, sources);

		std::string ocl_build_parameters = "";
		ocl_build_parameters += "-I ./";
		if (ccl_kernel_max_registers != 0)
		{
			/* TODO: check for cl_nv_compiler_options extension */
			std::stringstream out;
			out << ccl_kernel_max_registers;
			ocl_build_parameters += " -cl-nv-maxrregcount=";
			ocl_build_parameters += out.str();
			ocl_build_parameters += " ";
			ocl_build_parameters += " -cl-nv-opt-level=0 ";
		}

		cl_int err = ccl_program.build(std::vector<cl::Device>(1,this->cl.cDevice), ocl_build_parameters.c_str());
		if (err != CL_SUCCESS)
		{
			this->error << "failed to compile " << filename << CError::endl;
			this->error << ccl_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(this->cl.cDevice) << CError::endl;
		}
	}















#if LBM_OPENCL_FS_DEBUG
/**
 * ***********************************
 * DEBUG STUFF
 * ***********************************
 */

	T *density_array;
	int *flags_array;
	T *mass_array;
	T *fluid_fraction_array;
	T *velocity_array;
	T *dd_array;

	void validate_LoadDatasets()
	{
		density_array = new T[domain_cells_count];
		storeDensity(density_array);

		flags_array = new int[domain_cells_count];
		storeFlags(flags_array);

		mass_array = new T[domain_cells_count];
		storeMass(mass_array);

		fluid_fraction_array = new T[domain_cells_count];
		storeFraction(fluid_fraction_array);

		velocity_array = new T[domain_cells_count*3];
		storeVelocity(velocity_array);

		dd_array = new T[domain_cells_count*19];
		storeDensityDistributions(dd_array);

		wait();

	}

	void validate_FreeDatasets()
	{
		delete [] dd_array;
		delete [] velocity_array;
		delete [] mass_array;
		delete [] fluid_fraction_array;
		delete [] flags_array;
		delete [] density_array;
	}

	/**
	 * do a bunch of validation checks and output some useful informations
	 *
	 * this is just for debugging purposes
	 */

	bool validateDensity()
	{

		bool validation_failed = false;

		for (int a = 0; a < (int)domain_cells_count; a++)
		{
			if (flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_FLUID || flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE)
			{
				T density = density_array[a];

				/**
				 * assume, that a density larger than 2 is invalid
				 */
				if (CMath::abs(density) > 2.0)
				{
					int x = a % params.domain_cells[0];
					int y = (a / params.domain_cells[0]) % params.domain_cells[1];
					int z = a / (params.domain_cells[0]*params.domain_cells[1]);

					std::cout << "density at (" << x << ", " << y << ", " << z << ") exceeds valid value: " << density << "  timestep: " << simulation_step_counter << std::endl;
					validation_failed = true;
				}
			}
		}


		return validation_failed;
	}

	bool validateMass()
	{
		bool validation_failed = false;

		for (int a = 0; a < (int)domain_cells_count; a++)
		{
			if (flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_FLUID || flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE)
			{
				T mass = mass_array[a];

				/**
				 * assume, that a mass larger than the mass exchange factor*2 is invalid
				 */
				if (CMath::abs(mass) > params.mass_exchange_factor*2)
				{
					int x = a % params.domain_cells[0];
					int y = (a / params.domain_cells[0]) % params.domain_cells[1];
					int z = a / (params.domain_cells[0]*params.domain_cells[1]);

					std::cout << "invalid mass at (" << x << ", " << y << ", " << z << "): (";
					std::cout << "Flag: " << getFlagChar(flags_array[a]) << ", Mass: " << mass_array[a] << ", Fluid Fraction: " << fluid_fraction_array[a] << ", Density: " << density_array[a];
					std::cout << ")    timestep: " << simulation_step_counter << std::endl;
					validation_failed = true;
				}
			}
		}


		return validation_failed;
	}



	bool validateFluidFraction()
	{
		bool validation_failed = false;

		for (int a = 0; a < (int)domain_cells_count; a++)
		{
			if (flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_FLUID || flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE)
			{
				T fluid_fraction = fluid_fraction_array[a];

				if (CMath::abs(fluid_fraction) > 2)
				{
					int x = a % params.domain_cells[0];
					int y = (a / params.domain_cells[0]) % params.domain_cells[1];
					int z = a / (params.domain_cells[0]*params.domain_cells[1]);

					std::cout << "fluid fraction at (" << x << ", " << y << ", " << z << ") exceeds valid value: " << fluid_fraction << "  (" << getFlagChar(flags_array[a]) << ")    timestep: " << simulation_step_counter << std::endl;
					validation_failed = true;
				}
			}
		}


		return validation_failed;
	}



	bool validateVelocity()
	{

		bool validation_failed = false;

		for (int a = 0; a < (int)domain_cells_count; a++)
		{
			if (flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_FLUID || flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE)
			{
				T velocity_x = velocity_array[a];
				T velocity_y = velocity_array[a+domain_cells_count];
				T velocity_z = velocity_array[a+domain_cells_count*2];

				/**
				 * assume, that a density larger than 2 is invalid
				 */
				if (velocity_x*velocity_x + velocity_y*velocity_y + velocity_z*velocity_z > 0.2501)
				{
					int x = a % params.domain_cells[0];
					int y = (a / params.domain_cells[0]) % params.domain_cells[1];
					int z = a / (params.domain_cells[0]*params.domain_cells[1]);

					std::cout << "velocity at (" << x << ", " << y << ", " << z << ") exceeds valid value: (" << velocity_x << ", " << velocity_y << ", " << velocity_z << ") " << "  timestep: " << simulation_step_counter << std::endl;
					validation_failed = true;
				}
			}
		}


		return validation_failed;
	}



	bool validateFlags()
	{
		bool validation_failed = false;

		for (int a = 0; a < (int)domain_cells_count; a++)
		{
			switch(flags_array[a])
			{
			case CLbmOpenClInterface<T>::LBM_FLAG_FLUID:
			case CLbmOpenClInterface<T>::LBM_FLAG_GAS:
			case CLbmOpenClInterface<T>::LBM_FLAG_OBSTACLE:
			case CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE:
				break;

			default:
				int x = a % params.domain_cells[0];
				int y = (a / params.domain_cells[0]) % params.domain_cells[1];
				int z = a / (params.domain_cells[0]*params.domain_cells[1]);
				std::cout << "invalid flag at (" << x << ", " << y << ", " << z << "): " << flags_array[a];
				std::cout << "    " << "  timestep: " << simulation_step_counter << std::endl;
			}
		}


		return validation_failed;
	}



	bool validateDensityDistributions()
	{
		bool validation_failed = false;

		T dds[19];
		for (int i = 0; i < 19; i++)
			dds[i] = 0.0;

		for (int a = 0; a < (int)domain_cells_count; a++)
		{
			if (flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_FLUID || flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE)
			{
				int x = a % params.domain_cells[0];
				int y = (a / params.domain_cells[0]) % params.domain_cells[1];
				int z = a / (params.domain_cells[0]*params.domain_cells[1]);

				T max_dd = 0;
				int index;

/**
 * load a value or set it to 0 if it is not valid (e.g. from a gas cell)
 */
	// A-A pattern
	#define load_value(dx, dy, dz, ddid)							\
		index = (x+dx) + (y+dy)*params.domain_cells[0] + (z+dz)*params.domain_cells[0]*params.domain_cells[1];	\
		if (index < 0 || index > (int)domain_cells_count)			\
			dds[ddid] = -1.0;										\
		else														\
			dds[ddid] = dd_array[domain_cells_count*ddid + index]*	\
					(T)((flags_array[index] & (CLbmOpenClInterface<T>::LBM_FLAG_FLUID | CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE)) > 0);

				load_value(+1, 0, 0, 0);
				load_value(-1, 0, 0, 1);
				load_value(0, +1, 0, 2);
				load_value(0, -1, 0, 3);

				load_value(+1, +1, 0, 4);
				load_value(-1, -1, 0, 5);
				load_value(+1, -1, 0, 6);
				load_value(-1, +1, 0, 7);

				load_value(+1, 0, +1, 8);
				load_value(-1, 0, -1, 9);
				load_value(+1, 0, -1, 10);
				load_value(-1, 0, +1, 11);

				load_value(0, +1, +1, 12);
				load_value(0, -1, -1, 13);
				load_value(0, +1, -1, 14);
				load_value(0, -1, +1, 15);

				load_value(0, 0, +1, 16);
				load_value(0, 0, -1, 17);
				load_value(0, 0, 0, 18);

#undef load_value
				for (int i = 0; i < 19; i++)
					max_dd = CMath::max(CMath::abs(dds[i]), max_dd);

				/**
				 * assume, that a density larger than 2 is invalid (this is really invalid for incomp. flow!!!)
				 */
				if (max_dd > 2)
				{
					std::cout << "dd at (" << x << ", " << y << ", " << z << ") exceeds valid value: (";

					for (int i = 0; i < 19; i++)
						std::cout << dds[i] << " ";

					std::cout << ") " << "  timestep: " << simulation_step_counter << std::endl;
					validation_failed = true;
				}
			}
		}

		return validation_failed;
	}


	/**
	 * check that the interface is closed (no fluid cell may be next to a gas cell)
	 */
	bool validateClosedInterface()
	{
		bool validation_failed = false;

		for (int a = 0; a < (int)domain_cells_count; a++)
		{
			if (flags_array[a] == CLbmOpenClInterface<T>::LBM_FLAG_FLUID)
			{
				int x = a % params.domain_cells[0];
				int y = (a / params.domain_cells[0]) % params.domain_cells[1];
				int z = a / (params.domain_cells[0]*params.domain_cells[1]);

				int index;

/**
 * load a value or set it to 0 if it is not valid (e.g. from a gas cell)
 */
#define check_for_gas(dx, dy, dz)										\
	index = (x+dx) + (y+dy)*params.domain_cells[0] + (z+dz)*params.domain_cells[0]*params.domain_cells[1];	\
	if (index >= 0 && index < (int)domain_cells_count)					\
		if (flags_array[index] == CLbmOpenClInterface<T>::LBM_FLAG_GAS)	\
		{																\
			std::cout << "INTERFACE LAYER BROKEN AT (" << x << ", " << y << ", " << z << ") in direction (" << dx << ", " << dy << ", " << dz << ")" << "  timestep: " << simulation_step_counter << std::endl;	\
			validation_failed = true;									\
		}

				check_for_gas(+1, 0, 0);
				check_for_gas(-1, 0, 0);
				check_for_gas(0, +1, 0);
				check_for_gas(0, -1, 0);

				check_for_gas(+1, +1, 0);
				check_for_gas(-1, -1, 0);
				check_for_gas(+1, -1, 0);
				check_for_gas(-1, +1, 0);

				check_for_gas(+1, 0, +1);
				check_for_gas(-1, 0, -1);
				check_for_gas(+1, 0, -1);
				check_for_gas(-1, 0, +1);

				check_for_gas(0, +1, +1);
				check_for_gas(0, -1, -1);
				check_for_gas(0, +1, -1);
				check_for_gas(0, -1, +1);

				check_for_gas(0, 0, +1);
				check_for_gas(0, 0, -1);
				check_for_gas(0, 0, 0);
#undef check_for_gas

			}
		}

		return validation_failed;
	}

	inline char getFlagChar(int flag)
	{
		switch(flag)
		{
		case CLbmOpenClInterface<T>::LBM_FLAG_FLUID:
			return 'F';

		case CLbmOpenClInterface<T>::LBM_FLAG_GAS:
			return '.';

		case CLbmOpenClInterface<T>::LBM_FLAG_OBSTACLE:
			return 'X';

		case CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE:
			return 'I';


		case CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE_TO_FLUID:
			return 'U';

		case CLbmOpenClInterface<T>::LBM_FLAG_INTERFACE_TO_GAS:
			return 'D';

		case CLbmOpenClInterface<T>::LBM_FLAG_GAS_TO_INTERFACE:
			return 'R';

		default:
			return 'E';
		}
	}

	void outputDebugData()
	{

#define get_index(x,y,z)	(x + y*params.domain_cells[0] + z*params.domain_cells[0]*params.domain_cells[1])
#define get_flag(x,y,z)		getFlagChar(flags_array[get_index(x,y,z)])
#define get_velocity(x,y,z)	velocity_array[get_index(x,y,z)]
#define get_density(x,y,z)	density_array[get_index(x,y,z)]
#define get_density_distribution(x,y,z,ddid)	dd_array[get_index(x,y,z) + ddid*domain_cells_count]

//		std::cout << std::setprecision(3);

		int y_start = 6;
		int y_end = 0;

		int x_start = 0;
		int x_end = 10;

		int z_start = 10;
		int z_end = 21;

		if (simulation_step_counter >= 40 && 0)
		{
			std::cout << std::setprecision(3);

			std::cout << std::endl;
			std::cout << "---------------- TIME STEP " << simulation_step_counter << "----------------" << std::endl;

			z_start = 15;
			z_end = 16;

			std::cout << "FLAGS: " << std::endl;

			for (int y = y_start; y >= y_end; y--)
			{
				for (int z = z_start; z < z_end; z++)
				{
					for (int x = x_start; x < x_end; x++)
					{
						std::cout << get_flag(x, y, z) << " ";
					}
					std::cout << "\t";
				}
				std::cout << std::endl;
			}
#if 1
			std::cout << "VELOCITY: " << std::endl;

			for (int y = y_start; y >= y_end; y--)
			{
				for (int z = z_start; z < z_end; z++)
				{
					for (int x = x_start; x < x_end; x++)
					{
						T v = get_velocity(x, y, z);
						if (CMath::abs(v) > 2.0)	std::cout << "___I___";
						else						std::cout << v;
						std::cout << "\t";
					}
					std::cout << "\t";
				}
				std::cout << std::endl;
			}
#endif
#if 1
			std::cout << "DENSITY: " << std::endl;

			for (int y = y_start; y >= y_end; y--)
			{
				for (int z = z_start; z < z_end; z++)
				{
					for (int x = x_start; x < x_end; x++)
					{
						T d = get_density(x, y, z);
						if (CMath::abs(d) > 2.0)	std::cout << "___I___";
						else							std::cout << d;
						std::cout << "\t";
					}
					std::cout << "\t";
				}
				std::cout << std::endl;
			}
#endif
#if 1
			for (int dd_id = 0; dd_id < 19; dd_id++)
			{
				std::cout << "DENSITY DISTRIBUTION #" << dd_id << ": " << std::endl;

				for (int y = y_start; y >= y_end; y--)
				{
					for (int z = z_start; z < z_end; z++)
					{
						for (int x = x_start; x < x_end; x++)
						{
							float dd = get_density_distribution(x, y, z, dd_id);

							if (CMath::abs(dd) > 2.0)	std::cout << "___I___";
							else							std::cout << dd;

							std::cout << "\t";
						}
						std::cout << "\t";
					}
					std::cout << std::endl;
				}
			}
#endif

			std::cout << std::endl;
			std::cout << "----------------------------------------------------------------" << std::endl;
			std::cout << std::endl;
		}
	}


	bool validationChecks()
	{
		bool validation_failed = false;

		validate_LoadDatasets();

		outputDebugData();
		validation_failed |= validateDensity();
		validation_failed |= validateVelocity();
		validation_failed |= validateMass();
		validation_failed |= validateFluidFraction();
		validation_failed |= validateFlags();
//		validation_failed |= validateDensityDistributions();
		validation_failed |= validateClosedInterface();

		validate_FreeDatasets();

		return validation_failed;
	}
#endif
};


#define CLBM_LOAD_PROGRAM(ccl_program, ccl_kernel_work_group_size, ccl_kernel_max_registers, filename)	\
			cl::Program ccl_program;																	\
			loadProgram(ccl_program, ccl_kernel_work_group_size, ccl_kernel_max_registers, this->cl_interface_program_defines.str(), filename, test_local_work_group_size);




#define CLBM_CREATE_KERNEL_Header(ccl_kernel, ccl_program, function_name)				\
																				\
		if (this->verbose)														\
			std::cout << "creating kernel " << function_name << std::endl << "\t\t\t\t\t(work_group_size=" << ccl_kernel##_WorkGroupSize[0] << ", max_registers=" << ccl_kernel##_MaxRegisters << ")" << std::endl;		\
																				\
		ccl_kernel = cl::Kernel(ccl_program, function_name);							\
/*		if (ccl_kernel.error())													\
		{																		\
			this->error << "ERROR: " << function_name << std::endl;				\
			this->error << ccl_kernel.error.getString();						\
			return;																\
		}	\
*/

#define CLBM_CREATE_KERNEL_Footer(ccl_kernel, ccl_program, function_name)				\
/*		if (ccl_kernel.error())													\
		{																		\
			this->error << "ERROR: " << function_name << std::endl;				\
			this->error << ccl_kernel.error.getString();						\
			return;																\
		}																		\
*/																				\
		if (ccl_kernel##_WorkGroupSize == 0)									\
		{																		\
			ccl_kernel.getWorkGroupInfo(this->cl.cDevice, CL_KERNEL_WORK_GROUP_SIZE, &ccl_kernel##_WorkGroupSize);	\
																				\
			if (this->verbose)														\
				std::cout << "detected maximum work group size: " << ccl_kernel##_WorkGroupSize << std::endl;	\
		}

#define CLBM_CREATE_KERNEL_(ccl_kernel, ccl_program, function_name)			\
		CLBM_CREATE_KERNEL_Header(ccl_kernel, ccl_program, function_name)		\
		CLBM_CREATE_KERNEL_Footer(ccl_kernel, ccl_program, function_name)

#define CLBM_CREATE_KERNEL_2(ccl_kernel, ccl_program, function_name, a, b)		\
		CLBM_CREATE_KERNEL_Header(ccl_kernel, ccl_program, function_name)		\
																		\
		ccl_kernel.setArg(0, a);										\
		ccl_kernel.setArg(1, b);										\
																		\
		CLBM_CREATE_KERNEL_Footer(ccl_kernel, ccl_program, function_name)

#define CLBM_CREATE_KERNEL_3(ccl_kernel, ccl_program, function_name, a, b, c)	\
		CLBM_CREATE_KERNEL_Header(ccl_kernel, ccl_program, function_name)		\
																		\
		ccl_kernel.setArg(0, a);										\
		ccl_kernel.setArg(1, b);										\
		ccl_kernel.setArg(2, c);										\
																		\
		CLBM_CREATE_KERNEL_Footer(ccl_kernel, ccl_program, function_name)



#define CLBM_CREATE_KERNEL_7(ccl_kernel, ccl_program, function_name, a, b, c, d, e, f, g)	\
		CLBM_CREATE_KERNEL_Header(ccl_kernel, ccl_program, function_name)		\
																		\
		ccl_kernel.setArg(0, a);										\
		ccl_kernel.setArg(1, b);										\
		ccl_kernel.setArg(2, c);										\
		ccl_kernel.setArg(3, d);										\
		ccl_kernel.setArg(4, e);										\
		ccl_kernel.setArg(5, f);										\
		ccl_kernel.setArg(6, g);										\
																		\
		CLBM_CREATE_KERNEL_Footer(ccl_kernel, ccl_program, function_name)


#define CLBM_CREATE_KERNEL_13(ccl_kernel, ccl_program, function_name, a, b, c, d, e, f, g, h, i, j, k, l, m)	\
		CLBM_CREATE_KERNEL_Header(ccl_kernel, ccl_program, function_name)	\
																\
		ccl_kernel.setArg(0, a);								\
		ccl_kernel.setArg(1, b);								\
		ccl_kernel.setArg(2, c);								\
		ccl_kernel.setArg(3, d);								\
		ccl_kernel.setArg(4, e);								\
		ccl_kernel.setArg(5, f);								\
		ccl_kernel.setArg(6, g);								\
		ccl_kernel.setArg(7, h);								\
		ccl_kernel.setArg(8, i);								\
		ccl_kernel.setArg(9, j);								\
		ccl_kernel.setArg(10, k);								\
		ccl_kernel.setArg(11, l);								\
		ccl_kernel.setArg(12, m);								\
																\
		CLBM_CREATE_KERNEL_Footer(ccl_kernel, ccl_program, function_name)


#define CLBM_CREATE_KERNEL_14(ccl_kernel, ccl_program, function_name, a, b, c, d, e, f, g, h, i, j, k, l, m, n)	\
		CLBM_CREATE_KERNEL_Header(ccl_kernel, ccl_program, function_name)	\
																\
		ccl_kernel.setArg(0, a);								\
		ccl_kernel.setArg(1, b);								\
		ccl_kernel.setArg(2, c);								\
		ccl_kernel.setArg(3, d);								\
		ccl_kernel.setArg(4, e);								\
		ccl_kernel.setArg(5, f);								\
		ccl_kernel.setArg(6, g);								\
		ccl_kernel.setArg(7, h);								\
		ccl_kernel.setArg(8, i);								\
		ccl_kernel.setArg(9, j);								\
		ccl_kernel.setArg(10, k);								\
		ccl_kernel.setArg(11, l);								\
		ccl_kernel.setArg(12, m);								\
		ccl_kernel.setArg(13, n);								\
																\
		CLBM_CREATE_KERNEL_Footer(ccl_kernel, ccl_program, function_name)




#define CLBM_CREATE_KERNEL_15(ccl_kernel, ccl_program, function_name, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)	\
		CLBM_CREATE_KERNEL_Header(ccl_kernel, ccl_program, function_name)	\
																\
		ccl_kernel.setArg(0, a);								\
		ccl_kernel.setArg(1, b);								\
		ccl_kernel.setArg(2, c);								\
		ccl_kernel.setArg(3, d);								\
		ccl_kernel.setArg(4, e);								\
		ccl_kernel.setArg(5, f);								\
		ccl_kernel.setArg(6, g);								\
		ccl_kernel.setArg(7, h);								\
		ccl_kernel.setArg(8, i);								\
		ccl_kernel.setArg(9, j);								\
		ccl_kernel.setArg(10, k);								\
		ccl_kernel.setArg(11, l);								\
		ccl_kernel.setArg(12, m);								\
		ccl_kernel.setArg(13, n);								\
		ccl_kernel.setArg(14, o);								\
																\
		CLBM_CREATE_KERNEL_Footer(ccl_kernel, ccl_program, function_name)


#endif
