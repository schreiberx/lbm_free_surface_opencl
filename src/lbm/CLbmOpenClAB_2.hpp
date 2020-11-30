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

#ifndef CLBM_OPENCL_AB_2_OPEN_CL_HPP
#define CLBM_OPENCL_AB_2_OPEN_CL_HPP

#include "CLbmParameters.hpp"
#include "CLbmOpenClInterface.hpp"
#include "libopencl/CCLSkeleton.hpp"
#include "lib/CError.hpp"
#include <typeinfo>
#include <iomanip>

#include "lib/CStopwatch.hpp"

#define LBM_AB_2_TEST_WITH_AA_1_KERNEL	0

/**
 * OpenCL implementation for lattice boltzmann method using the A-B pattern
 */
template <typename T>
class CLbmOpenClAB_2	:	
	public CLbmOpenClInterface<T>
{
	using CLbmOpenClInterface<T>::initInterface;
	using CLbmOpenClInterface<T>::loadProgram;

private:
	// initialization kernels
	cl::Kernel cKernelLbm_Init;

	// simulation kernels
	cl::Kernel cKernelLbm_Main;
	cl::Kernel cKernelLbm_GasToInterface;

	cl::Kernel cKernelLbm_InterfaceToFluidNeighbors;
	cl::Kernel cKernelLbm_InterfaceToGas;
	cl::Kernel cKernelLbm_InterfaceToGasNeighbors;
	cl::Kernel cKernelLbm_GatherMass;
#if LBM_AB_2_TEST_WITH_AA_1_KERNEL
	cl::Kernel cKernelLbm_Alpha_Pre;
	cl::Kernel cKernelLbm_AA_Helper;
#endif


	cl::Kernel cKernelLbm_MassScale;

	size_t global_work_group_size_a[1];
	size_t max_local_work_group_size;


	/*
	 * work group sizes for opencl kernels
	 */
	cl::NDRange cKernelLbm_Init_WorkGroupSize;

	cl::NDRange cKernelLbm_Main_WorkGroupSize;
	cl::NDRange cKernelLbm_GasToInterface_WorkGroupSize;

	cl::NDRange cKernelLbm_InterfaceToGas_WorkGroupSize;
	cl::NDRange cKernelLbm_InterfaceToGasNeighbors_WorkGroupSize;
	cl::NDRange cKernelLbm_InterfaceToFluidNeighbors_WorkGroupSize;
	cl::NDRange cKernelLbm_GatherMass_WorkGroupSize;

	cl::NDRange cKernelLbm_MassScale_WorkGroupSize;

#if LBM_AB_2_TEST_WITH_AA_1_KERNEL
	cl::NDRange cKernelLbm_Alpha_Pre_WorkGroupSize;
	cl::NDRange cKernelLbm_AA_Helper_WorkGroupSize;
#endif

	/*
	 * thread register limitations for opencl kernels
	 */
	size_t cKernelLbm_Init_MaxRegisters;

	size_t cKernelLbm_Main_MaxRegisters;
	size_t cKernelLbm_GasToInterface_MaxRegisters;

	size_t cKernelLbm_InterfaceToGas_MaxRegisters;
	size_t cKernelLbm_InterfaceToGasNeighbors_MaxRegisters;
	size_t cKernelLbm_InterfaceToFluidNeighbors_MaxRegisters;
	size_t cKernelLbm_GatherMass_MaxRegisters;

	size_t cKernelLbm_MassScale_MaxRegisters;

#if LBM_AB_2_TEST_WITH_AA_1_KERNEL
	size_t cKernelLbm_Alpha_Pre_MaxRegisters;
	size_t cKernelLbm_AA_Helper_MaxRegisters;
#endif

public:

	cl::Buffer cMemNewDensityDistributions;

	/**
	 * Setup class with OpenCL skeleton.
	 *
	 * No simulation kernels are loaded in this constructor!
	 */
	CLbmOpenClAB_2(	const CCLSkeleton &cClSkeleton,
					bool p_verbose = false
	)	:
		CLbmOpenClInterface<T>(cClSkeleton, p_verbose)
	{
	}


	/**
	 * initialize simulation with given parameters
	 */
	void init(	CVector<3,int> &p_domain_cells,			///< domain cells in each dimension
				T p_d_domain_x_length,					///< domain length in x direction
				T p_d_viscosity,						///< viscocity of fluid
				CVector<3,T> &p_d_gravitation,			///< gravitation
				T p_max_sim_gravitation_length,			///< maximum length of gravitation to limit timestep
				T p_d_timestep,							///< timestep for one simulation step - computed automagically
				T p_mass_exchange_factor,				///< mass exchange

				size_t p_max_local_work_group_size,		///< maximum count of computation kernels (threads) used on gpu

				int init_flags,							///< flags for first initialization

				std::list<int> &p_lbm_opencl_number_of_threads_list,		///< list with number of threads for each successively created kernel
				std::list<int> &p_lbm_opencl_number_of_registers_list		///< list with number of registers for each thread threads for each successively created kernel
		)
	{
		max_local_work_group_size = p_max_local_work_group_size;

		initInterface(			p_domain_cells,
								p_d_domain_x_length,
								p_d_viscosity,
								p_d_gravitation,
								p_max_sim_gravitation_length,
								p_d_timestep,
								p_mass_exchange_factor,
								p_lbm_opencl_number_of_threads_list,
								p_lbm_opencl_number_of_registers_list
							);

		reload();

		this->setupInitFlags(init_flags);
		resetFluid();
	}



    void setKernelArguments()
    {
		cKernelLbm_Init.setArg(4, this->params.inv_tau);
		cKernelLbm_Init.setArg(5, this->params.inv_trt_tau);

		cKernelLbm_Init.setArg(10, this->params.gravitation[0]);
		cKernelLbm_Init.setArg(11, this->params.gravitation[1]);
		cKernelLbm_Init.setArg(12, this->params.gravitation[2]);

		cKernelLbm_Main.setArg(4, this->params.inv_tau);
		cKernelLbm_Main.setArg(5, this->params.inv_trt_tau);

		cKernelLbm_Main.setArg(10, this->params.gravitation[0]);
		cKernelLbm_Main.setArg(11, this->params.gravitation[1]);
		cKernelLbm_Main.setArg(12, this->params.gravitation[2]);

		cKernelLbm_Main.setArg(13, this->params.mass_exchange_factor);

#if LBM_AB_2_TEST_WITH_AA_1_KERNEL
		cKernelLbm_Alpha_Pre.setArg(6, this->params.mass_exchange_factor);

		cKernelLbm_InterfaceToFluidNeighbors.setArg(6, this->params.mass_exchange_factor);
		cKernelLbm_InterfaceToGas.setArg(6, this->params.mass_exchange_factor);
		cKernelLbm_GasToInterface.setArg(6, this->params.mass_exchange_factor);
#else
		cKernelLbm_InterfaceToFluidNeighbors.setArg(6, this->params.mass_exchange_factor);
		cKernelLbm_InterfaceToGas.setArg(6, this->params.mass_exchange_factor);
		cKernelLbm_GasToInterface.setArg(6, this->params.mass_exchange_factor);
#endif

		CError_AppendReturnThis(this->params);
    }


	/**
	 * return the maximum work group size which has to be a factor of domain_cells_count
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
			local_work_group_size = CMath::gcd(local_work_group_size, this->domain_cells_count);
			if (output_information)
				std::cerr << "unable to use full local_work_group_size (" << max_local_work_group_size << ") for kernel => reducing to " << local_work_group_size << std::endl;
		}

		return local_work_group_size;
	}

	/**
	 * reload the simulation and kernels
	 */
	void reload()
	{
		this->reloadInterface();

		/*
		 * ALLOCATE BUFFERS
		 */
		cMemNewDensityDistributions = cl::Buffer(this->cl.cContext, CL_MEM_READ_WRITE, sizeof(T)*this->domain_cells_count*this->SIZE_DD_HOST);


		global_work_group_size_a[0] = this->domain_cells_count;

		if (max_local_work_group_size == 0)
		{
			cKernelLbm_Init_WorkGroupSize = 0;

			cKernelLbm_MassScale_WorkGroupSize = 0;

			cKernelLbm_Main_WorkGroupSize = 0;
			cKernelLbm_GasToInterface_WorkGroupSize = 0;

			cKernelLbm_InterfaceToGas_WorkGroupSize = 0;
			cKernelLbm_InterfaceToGasNeighbors_WorkGroupSize = 0;
			cKernelLbm_InterfaceToFluidNeighbors_WorkGroupSize = 0;
			cKernelLbm_GatherMass_WorkGroupSize = 0;
#if LBM_AB_2_TEST_WITH_AA_1_KERNEL
			cKernelLbm_Alpha_Pre_WorkGroupSize = 0;
			cKernelLbm_AA_Helper_WorkGroupSize = 0;
#endif
			if (this->verbose)
				std::cout << "loading kernels with test value for local_work_group_size (highly experimental, most probably wont work)" << std::endl;

			createKernels(true);

			if (this->verbose)
				std::cout << "loading kernels..." << std::endl;

			createKernels(false);
		}
		else
		{
#define to_str(x)	#x

/**
 * initialize the variable (postfixed appropriately with _WorkGroupSize and _MaxRegisters)
 * with either the standard value (max_local_work_group_size) or with the value from the list
 */
#define INIT_WORK_GROUP_SIZE(variable)										\
			if (it != this->lbm_opencl_number_of_threads_list.end())		\
			{																\
				variable##_WorkGroupSize = (*it != 0 ? *it : max_local_work_group_size);	\
				it++;														\
			}																\
			else															\
			{																\
				variable##_WorkGroupSize = max_local_work_group_size;		\
			}																\
																			\
			if (ir != this->lbm_opencl_number_of_registers_list.end())		\
			{																\
				variable##_MaxRegisters = (*ir != 0 ? *ir : 0);				\
				ir++;														\
			}																\
			else															\
			{																\
				variable##_MaxRegisters  = 0;								\
			}

			std::list<int>::iterator it = this->lbm_opencl_number_of_threads_list.begin();
			std::list<int>::iterator ir = this->lbm_opencl_number_of_registers_list.begin();

			INIT_WORK_GROUP_SIZE(cKernelLbm_Init);
			INIT_WORK_GROUP_SIZE(cKernelLbm_Main);
			INIT_WORK_GROUP_SIZE(cKernelLbm_GasToInterface);
			INIT_WORK_GROUP_SIZE(cKernelLbm_InterfaceToGas);
			INIT_WORK_GROUP_SIZE(cKernelLbm_InterfaceToGasNeighbors);
			INIT_WORK_GROUP_SIZE(cKernelLbm_InterfaceToFluidNeighbors);
			INIT_WORK_GROUP_SIZE(cKernelLbm_GatherMass);

#if LBM_AB_2_TEST_WITH_AA_1_KERNEL
			INIT_WORK_GROUP_SIZE(cKernelLbm_Alpha_Pre);
			INIT_WORK_GROUP_SIZE(cKernelLbm_AA_Helper);
#endif

			INIT_WORK_GROUP_SIZE(cKernelLbm_MassScale);
#undef to_str
#undef INIT_WORK_GROUP_SIZE

			createKernels(false);
		}
	}


	void createKernels(bool test_local_work_group_size)
	{
		/*
		 * prepare CL Programs
		 */

		// initialization
		CLBM_LOAD_PROGRAM(cProgram_Init, cKernelLbm_Init_WorkGroupSize, cKernelLbm_Init_MaxRegisters, "data/cl_programs/lbm_init.cl");

		// interface conversions
		CLBM_LOAD_PROGRAM(cProgram_InterfaceToGas,			cKernelLbm_InterfaceToGas_WorkGroupSize,			cKernelLbm_InterfaceToGas_MaxRegisters,				"data/cl_programs/lbm_interface_to_gas.cl");
		CLBM_LOAD_PROGRAM(cProgram_InterfaceToGasNeighbors,	cKernelLbm_InterfaceToGasNeighbors_WorkGroupSize,	cKernelLbm_InterfaceToGasNeighbors_MaxRegisters,	"data/cl_programs/lbm_interface_to_gas_neighbors.cl");
		CLBM_LOAD_PROGRAM(cProgram_InterfaceToFluidNeighbors,	cKernelLbm_InterfaceToFluidNeighbors_WorkGroupSize,	cKernelLbm_InterfaceToFluidNeighbors_MaxRegisters,	"data/cl_programs/lbm_interface_to_fluid_neighbors.cl");
		CLBM_LOAD_PROGRAM(cProgram_GatherMass,				cKernelLbm_GatherMass_WorkGroupSize, 				cKernelLbm_GatherMass_MaxRegisters,					"data/cl_programs/lbm_gather_mass.cl");

		// collision and propagation kernel
#if LBM_AB_2_TEST_WITH_AA_1_KERNEL
		CLBM_LOAD_PROGRAM(cProgram_Alpha_Pre,			cKernelLbm_Alpha_Pre_WorkGroupSize,			cKernelLbm_Alpha_Pre_MaxRegisters,		"data/cl_programs/lbm_alpha_pre.cl");
		CLBM_LOAD_PROGRAM(cProgram_Main,				cKernelLbm_Main_WorkGroupSize,				cKernelLbm_Main_MaxRegisters,			"data/cl_programs/lbm_alpha.cl");
		CLBM_LOAD_PROGRAM(cProgram_AA_Helper,			cKernelLbm_AA_Helper_WorkGroupSize,			cKernelLbm_AA_Helper_MaxRegisters,		"data/cl_programs/lbm_debug_beta_propagation.cl");
		CLBM_LOAD_PROGRAM(cProgram_GasToInterface,	cKernelLbm_GasToInterface_WorkGroupSize, 	cKernelLbm_GasToInterface_MaxRegisters, "data/cl_programs/lbm_alpha_flag_gas_to_interface.cl");
#else
		CLBM_LOAD_PROGRAM(cProgram_Main,				cKernelLbm_Main_WorkGroupSize,				cKernelLbm_Main_MaxRegisters,				"data/cl_programs/lbm_ab_1_coll_prop.cl");
		CLBM_LOAD_PROGRAM(cProgram_GasToInterface,	cKernelLbm_GasToInterface_WorkGroupSize, 	cKernelLbm_GasToInterface_MaxRegisters, 	"data/cl_programs/lbm_ab_1_flag_gas_to_interface.cl");
#endif

		// mass scaling
		CLBM_LOAD_PROGRAM(cProgram_MassScale, cKernelLbm_MassScale_WorkGroupSize, cKernelLbm_MassScale_MaxRegisters,	"data/cl_programs/lbm_mass_scale.cl");


		/**********************************************************
		 * initialization kernel
		 **********************************************************/
		CLBM_CREATE_KERNEL_13(	cKernelLbm_Init, cProgram_Init, "kernel_lbm_init",
						this->cMemDensityDistributions,
						this->cMemCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->params.inv_tau,
						this->params.inv_trt_tau,
						this->cMemFluidMass,
						this->cMemFluidFraction,
						this->cMemNewCellFlags,
						this->cMemNewFluidFraction,

						this->params.gravitation[0],
						this->params.gravitation[1],
						this->params.gravitation[2]
		);


		/**********************************************************
		 * simulation kernels
		 **********************************************************/

		// MAIN
#if LBM_AB_2_TEST_WITH_AA_1_KERNEL
		// PRE ALPHA KERNEL
		CLBM_CREATE_KERNEL_7(	cKernelLbm_Alpha_Pre, cProgram_Alpha_Pre, "kernel_lbm_alpha_pre",
						this->cMemDensityDistributions,
						this->cMemCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemFluidFraction,
						this->params.mass_exchange_factor
		);

		CLBM_CREATE_KERNEL_14(	cKernelLbm_Main, cProgram_Main, "kernel_lbm_alpha",
						this->cMemDensityDistributions,
						this->cMemCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->params.inv_tau,
						this->params.inv_trt_tau,
						this->cMemFluidMass,
						this->cMemFluidFraction,
						this->cMemNewCellFlags,
						this->cMemNewFluidFraction,

						this->params.gravitation[0],
						this->params.gravitation[1],
						this->params.gravitation[2],

						this->params.mass_exchange_factor
		);

		CLBM_CREATE_KERNEL_2(	cKernelLbm_AA_Helper, cProgram_AA_Helper, "kernel_debug_beta_propagation",
						this->cMemDensityDistributions,
						this->cMemNewDensityDistributions
		);

		// INTERFACE TO FLUID NEIGHBORS
		CLBM_CREATE_KERNEL_7(	cKernelLbm_InterfaceToFluidNeighbors, cProgram_InterfaceToFluidNeighbors, "kernel_interface_to_fluid_neighbors",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);

		// INTERFACE TO GAS
		CLBM_CREATE_KERNEL_7(	cKernelLbm_InterfaceToGas, cProgram_InterfaceToGas, "kernel_interface_to_gas",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);

		// INTERFACE TO GAS NEIGHBORS
		CLBM_CREATE_KERNEL_7(	cKernelLbm_InterfaceToGasNeighbors, cProgram_InterfaceToGasNeighbors, "kernel_interface_to_gas_neighbors",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->cMemFluidFraction	// use fluid fraction as temporary buffer
		);

		// GATHER MASS
		CLBM_CREATE_KERNEL_3(	cKernelLbm_GatherMass, cProgram_GatherMass, "kernel_gather_mass",
						this->cMemNewCellFlags,
						this->cMemFluidMass,
						this->cMemFluidFraction
		);

		// ALPHA GAS TO INTERFACE CONVERSION
		CLBM_CREATE_KERNEL_7(	cKernelLbm_GasToInterface, cProgram_GasToInterface, "kernel_lbm_alpha_flag_gas_to_interface",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);

#else
		CLBM_CREATE_KERNEL_15(	cKernelLbm_Main, cProgram_Main, "kernel_lbm_coll_prop",
						this->cMemDensityDistributions,
						this->cMemCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->params.inv_tau,
						this->params.inv_trt_tau,
						this->cMemFluidMass,
						this->cMemFluidFraction,
						this->cMemNewCellFlags,
						this->cMemNewFluidFraction,

						this->params.gravitation[0],
						this->params.gravitation[1],
						this->params.gravitation[2],

						this->params.mass_exchange_factor,
						this->cMemNewDensityDistributions
		);


		// INTERFACE TO FLUID NEIGHBORS
		CLBM_CREATE_KERNEL_7(	cKernelLbm_InterfaceToFluidNeighbors, cProgram_InterfaceToFluidNeighbors, "kernel_interface_to_fluid_neighbors",
						this->cMemNewDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);

		// INTERFACE TO GAS
		CLBM_CREATE_KERNEL_7(	cKernelLbm_InterfaceToGas, cProgram_InterfaceToGas, "kernel_interface_to_gas",
						this->cMemNewDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);

		// INTERFACE TO GAS NEIGHBORS
		CLBM_CREATE_KERNEL_7(	cKernelLbm_InterfaceToGasNeighbors, cProgram_InterfaceToGasNeighbors, "kernel_interface_to_gas_neighbors",
						this->cMemNewDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->cMemFluidFraction	// use fluid fraction as temporary buffer
		);

		// GATHER MASS
		CLBM_CREATE_KERNEL_3(	cKernelLbm_GatherMass, cProgram_GatherMass, "kernel_gather_mass",
						this->cMemNewCellFlags,
						this->cMemFluidMass,
						this->cMemFluidFraction
		);

		// ALPHA GAS TO INTERFACE CONVERSION
		CLBM_CREATE_KERNEL_7(	cKernelLbm_GasToInterface, cProgram_GasToInterface, "kernel_ab_flag_gas_to_interface",
						this->cMemNewDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);
#endif

		/**********************************************************
		 * mass scaling kernel
		 **********************************************************/
		CLBM_CREATE_KERNEL_(	cKernelLbm_MassScale, cProgram_MassScale, "kernel_lbm_mass_scale");
		cKernelLbm_MassScale.setArg(0, this->cMemFluidMass);
	}

	/**
	 * reset the fluid to it's initial state
	 */
	void resetFluid()
	{
		if (this->error())
			return;

		if (this->verbose)
			std::cout << "Init Simulation: " << std::flush;

		cKernelLbm_Init.setArg(13, this->fluid_init_flags);
		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_Init,	// kernel
														cl::NullRange,			// global work offset
														cl::NDRange(global_work_group_size_a[0]),
														cl::NDRange(cKernelLbm_Init_WorkGroupSize)
						);

		this->cl.cCommandQueue.enqueueBarrier();

		// also initialize cMemNewDensityDistributions to avoid infinite velocities from gas cells!
		this->cl.cCommandQueue.enqueueCopyBuffer(
											this->cMemDensityDistributions,
											cMemNewDensityDistributions,
											0,
											0,
											this->domain_cells_count*this->SIZE_DD_HOST_BYTES
                                        );

		this->cl.cCommandQueue.enqueueBarrier();

		this->resetFluid_Interface();

		if (this->verbose)
			std::cout << "OK" << std::endl;
	}


	/**
	 * scale the mass in each cell to stabilize the overall amount of mass in the simulation
	 */
	void scaleMass(T mass_scale_factor)
	{
		cKernelLbm_MassScale.setArg(1, mass_scale_factor);

		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_MassScale,	// kernel
												cl::NullRange,				// global work offset
												cl::NDRange(global_work_group_size_a[0]),
												cl::NDRange(cKernelLbm_MassScale_WorkGroupSize)
						);

		this->cl.cCommandQueue.enqueueBarrier();
	}

	/**
	 * start one simulation step (enqueue kernels)
	 */
#if LBM_AB_2_TEST_WITH_AA_1_KERNEL
	void simulationStep()
	{
		/*
		 * collision kernels are inserted as 1d kernels because they work cell wise without neighboring information
		 */

		this->cl.cCommandQueue.enqueueBarrier();

		/*
		 * PRE
		 */
		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_Alpha_Pre,	// kernel
												cl::NullRange,					// global work offset
												cl::NDRange(global_work_group_size_a[0]),
												cl::NDRange(cKernelLbm_Alpha_Pre_WorkGroupSize)
						);

		this->cl.cCommandQueue.enqueueBarrier();

		/*
		 * MAIN
		 */

		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_Main,	// kernel
												cl::NullRange,						// global work offset
												cl::NDRange(global_work_group_size_a[0]),
												cl::NDRange(cKernelLbm_Main_WorkGroupSize)
						);

		this->cl.cCommandQueue.enqueueBarrier();


		/*
		 * INTERFACE TO FLUID NEIGHBORS
		 */

		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_InterfaceToFluidNeighbors,	// kernel
												cl::NullRange,					// global work offset
												cl::NDRange(global_work_group_size_a[0]),
												cl::NDRange(cKernelLbm_InterfaceToFluidNeighbors_WorkGroupSize)
						);
		this->cl.cCommandQueue.enqueueBarrier();


		/*
		 * INTERFACE TO GAS
		 */
		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_InterfaceToGas,	// kernel
												cl::NullRange,					// global work offset
												cl::NDRange(global_work_group_size_a[0]),
												cl::NDRange(cKernelLbm_InterfaceToGas_WorkGroupSize)
						);
		this->cl.cCommandQueue.enqueueBarrier();


		/*
		 * GATHER MASS
		 */
		cKernelLbm_GatherMass.setArg(0, this->cMemNewCellFlags);

		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_GatherMass,	// kernel
												cl::NullRange,				// global work offset
												cl::NDRange(global_work_group_size_a[0]),
												cl::NDRange(cKernelLbm_GatherMass_WorkGroupSize)
						);

		this->cl.cCommandQueue.enqueueBarrier();


		/*
		 * INTERFACE TO GAS NEIGHBORS
		 */
		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_InterfaceToGasNeighbors,	// kernel
												cl::NullRange,					// global work offset
												cl::NDRange(global_work_group_size_a[0]),
												cl::NDRange(cKernelLbm_InterfaceToGasNeighbors_WorkGroupSize)
						);
		this->cl.cCommandQueue.enqueueBarrier();


		/*
		 * GAS TO INTERFACE
		 */
		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_GasToInterface,	// kernel
												cl::NullRange,					// global work offset
												cl::NDRange(global_work_group_size_a[0]),
												cl::NDRange(cKernelLbm_GasToInterface_WorkGroupSize)
						);

		this->cl.cCommandQueue.enqueueBarrier();

		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_AA_Helper,     // kernel
                                                cl::NullRange,                              // global work offset
                                                cl::NDRange(global_work_group_size_a[0]),
                                                cl::NDRange(cKernelLbm_AA_Helper_WorkGroupSize)
                                        );
		this->cl.cCommandQueue.enqueueBarrier();

		this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemNewDensityDistributions,
												this->cMemDensityDistributions,
												0,
												0,
												this->domain_cells_count*sizeof(T)*19
										);

		this->cl.cCommandQueue.enqueueBarrier();
		this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemNewFluidFraction,
												this->cMemFluidFraction,
												0,
												0,
												this->domain_cells_count*sizeof(T)
										);

		this->cl.cCommandQueue.enqueueBarrier();
		this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemNewCellFlags,
												this->cMemCellFlags,
												0,
												0,
												this->domain_cells_count*sizeof(T)
										);

		this->cl.cCommandQueue.enqueueBarrier();

		this->simulation_step_counter++;
	}
#else
	void simulationStep()
	{
		/*
		 * collision kernels are inserted as 1d kernels because they work cell wise without neighboring information
		 */

		this->cl.cCommandQueue.enqueueBarrier();

		if (this->simulation_step_counter & 1)
		{
			/*
			 * MAIN
			 */
			cKernelLbm_Main.setArg(0, cMemNewDensityDistributions);
			cKernelLbm_Main.setArg(14, this->cMemDensityDistributions);

			cKernelLbm_Main.setArg(1, this->cMemNewCellFlags);
			cKernelLbm_Main.setArg(8, this->cMemCellFlags);

			cKernelLbm_Main.setArg(7, this->cMemNewFluidFraction);
			cKernelLbm_Main.setArg(9, this->cMemFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_Main,	// kernel
													cl::NullRange,						// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_Main_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrier();

			/*
			 * INTERFACE TO FLUID NEIGHBORS
			 */
			cKernelLbm_InterfaceToFluidNeighbors.setArg(1, this->cMemCellFlags);
			cKernelLbm_InterfaceToFluidNeighbors.setArg(5, this->cMemFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_InterfaceToFluidNeighbors,	// kernel
													cl::NullRange,					// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_InterfaceToFluidNeighbors_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrier();

			/*
			 * INTERFACE TO GAS
			 */
			cKernelLbm_InterfaceToGas.setArg(1, this->cMemCellFlags);
			cKernelLbm_InterfaceToGas.setArg(5, this->cMemFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_InterfaceToGas,	// kernel
													cl::NullRange,					// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_InterfaceToGas_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrier();

			/*
			 * GATHER MASS
			 */

			cKernelLbm_GatherMass.setArg(0, this->cMemCellFlags);
			cKernelLbm_GatherMass.setArg(2, this->cMemFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_GatherMass,	// kernel
													cl::NullRange,				// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_GatherMass_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrier();

			/*
			 * INTERFACE TO GAS NEIGHBORS
			 */
			cKernelLbm_InterfaceToGasNeighbors.setArg(1, this->cMemCellFlags);
			cKernelLbm_InterfaceToGasNeighbors.setArg(5, this->cMemFluidFraction);
			cKernelLbm_InterfaceToGasNeighbors.setArg(6, this->cMemNewFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_InterfaceToGasNeighbors,	// kernel
													cl::NullRange,					// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_InterfaceToGasNeighbors_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrier();

			/*
			 * GAS TO INTERFACE
			 */
			cKernelLbm_GasToInterface.setArg(0, this->cMemDensityDistributions);
			cKernelLbm_GasToInterface.setArg(1, this->cMemCellFlags);
			cKernelLbm_GasToInterface.setArg(5, this->cMemFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_GasToInterface,	// kernel
													cl::NullRange,					// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_GasToInterface_WorkGroupSize)
							);

			////////////////////////////////////
			#if 0
			this->cl.cCommandQueue.enqueueBarrier();
					this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemDensityDistributions,
															this->cMemNewDensityDistributions,
			                                                0,
			                                                0,
			                                                this->domain_cells_count*sizeof(T)*19
			                                        );

					this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemFluidFraction,
															this->cMemNewFluidFraction,
			                                                0,
			                                                0,
			                                                this->domain_cells_count*sizeof(T)
			                                        );

					this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemCellFlags,
															this->cMemNewCellFlags,
			                                                0,
			                                                0,
			                                                this->domain_cells_count*sizeof(T)
			                                        );

					this->cl.cCommandQueue.enqueueBarrier();
			#endif
			////////////////////////////////////
		}
		else
		{
			// EVEN TIMESTEP

			/*
			 * MAIN
			 */

			cKernelLbm_Main.setArg(0, this->cMemDensityDistributions);
			cKernelLbm_Main.setArg(14, this->cMemNewDensityDistributions);

			cKernelLbm_Main.setArg(1, this->cMemCellFlags);
			cKernelLbm_Main.setArg(8, this->cMemNewCellFlags);

			cKernelLbm_Main.setArg(7, this->cMemFluidFraction);
			cKernelLbm_Main.setArg(9, this->cMemNewFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_Main,	// kernel
													cl::NullRange,						// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_Main_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrier();

			/*
			 * INTERFACE TO FLUID NEIGHBORS
			 */

			cKernelLbm_InterfaceToFluidNeighbors.setArg(1, this->cMemNewCellFlags);
			cKernelLbm_InterfaceToFluidNeighbors.setArg(5, this->cMemNewFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_InterfaceToFluidNeighbors,	// kernel
													cl::NullRange,					// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_InterfaceToFluidNeighbors_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrier();


			/*
			 * INTERFACE TO GAS
			 */

			cKernelLbm_InterfaceToGas.setArg(1, this->cMemNewCellFlags);
			cKernelLbm_InterfaceToGas.setArg(5, this->cMemNewFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_InterfaceToGas,	// kernel
													cl::NullRange,					// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_InterfaceToGas_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrier();



			/*
			 * GATHER MASS
			 */

			cKernelLbm_GatherMass.setArg(0, this->cMemNewCellFlags);
			cKernelLbm_GatherMass.setArg(2, this->cMemNewFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_GatherMass,	// kernel
													cl::NullRange,				// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_GatherMass_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrier();


			/*
			 * INTERFACE TO GAS NEIGHBORS
			 */

			cKernelLbm_InterfaceToGasNeighbors.setArg(1, this->cMemNewCellFlags);
			cKernelLbm_InterfaceToGasNeighbors.setArg(5, this->cMemNewFluidFraction);
			cKernelLbm_InterfaceToGasNeighbors.setArg(6, this->cMemFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_InterfaceToGasNeighbors,	// kernel
													cl::NullRange,					// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_InterfaceToGasNeighbors_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrier();


			/*
			 * GAS TO INTERFACE
			 */

			cKernelLbm_GasToInterface.setArg(0, this->cMemNewDensityDistributions);
			cKernelLbm_GasToInterface.setArg(1, this->cMemNewCellFlags);
			cKernelLbm_GasToInterface.setArg(5, this->cMemNewFluidFraction);

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbm_GasToInterface,	// kernel
													cl::NullRange,					// global work offset
													cl::NDRange(global_work_group_size_a[0]),
													cl::NDRange(cKernelLbm_GasToInterface_WorkGroupSize)
							);

			////////////////////////////////////
			#if 0
				this->cl.cCommandQueue.enqueueBarrier();
				this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemNewDensityDistributions,
														this->cMemDensityDistributions,
														0,
														0,
														this->domain_cells_count*sizeof(T)*19
												);

				this->cl.cCommandQueue.enqueueBarrier();
				this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemNewFluidFraction,
														this->cMemFluidFraction,
														0,
														0,
														this->domain_cells_count*sizeof(T)
												);

				this->cl.cCommandQueue.enqueueBarrier();
				this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemNewCellFlags,
														this->cMemCellFlags,
														0,
														0,
														this->domain_cells_count*sizeof(int)
												);
			#endif
			////////////////////////////////////
		}

		this->cl.cCommandQueue.enqueueBarrier();

		this->simulation_step_counter++;
	}
#endif
};

#endif
