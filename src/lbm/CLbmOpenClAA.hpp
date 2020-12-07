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

#ifndef CLBM_AA_OPEN_CL_HPP
#define CLBM_AA_OPEN_CL_HPP

#include "CLbmParameters.hpp"
#include "CLbmOpenClInterface.hpp"
#include "libopencl/CCLSkeleton.hpp"
#include "lib/CError.hpp"
#include <typeinfo>
#include <iomanip>

#include "lib/CStopwatch.hpp"

/**
 * set to 1 to test only alpha or beta kernel
 * the opposite kernel is replaced by a "propagation kernel"
 */
#define LBM_AA_ALPHA_KERNEL_AS_PROPAGATION	0
#define LBM_BETA_AA_KERNEL_AS_PROPAGATION	0

#define AA_TIMING	0

/**
 * OpenCL implementation for lattice boltzmann method using the A-A pattern with a single density distribution buffer
 */
template <typename T>
class CLbmOpenClAA	:
	public CLbmOpenClInterface<T>
{
	using CLbmOpenClInterface<T>::initInterface;
	using CLbmOpenClInterface<T>::loadProgram;

public:
#if AA_TIMING
	double cAlphaTimingsCount;				///< performance counter
	double cKernelLbmAlpha_Pre_Timing_Sum;	///< performance counter
	double cKernelLbmAlpha_Main_Timing_Sum;	///< performance counter
	double cKernelLbmAlpha_GasToInterface_Timing_Sum;	///< performance counter

	double cKernelLbmAlpha_InterfaceToGas_Timing_Sum;	///< performance counter
	double cKernelLbmAlpha_InterfaceToGasNeighbors_Timing_Sum;	///< performance counter
	double cKernelLbmAlpha_InterfaceToFluidNeighbors_Timing_Sum;	///< performance counter
	double cKernelLbmAlpha_GatherMass_Timing_Sum;	///< performance counter

	double cBetaTimingsCount;					///< performance counter
	double cKernelLbmBeta_Pre_Timing_Sum;		///< performance counter
	double cKernelLbmBeta_Main_Timing_Sum;		///< performance counter
	double cKernelLbmBeta_GasToInterface_Timing_Sum;	///< performance counter

	double cKernelLbmBeta_InterfaceToGas_Timing_Sum;	///< performance counter
	double cKernelLbmBeta_InterfaceToGasNeighbors_Timing_Sum;	///< performance counter
	double cKernelLbmBeta_InterfaceToFluidNeighbors_Timing_Sum;	///< performance counter
	double cKernelLbmBeta_GatherMass_Timing_Sum;		///< performance counter

	double cKernelLbmAlpha_Pre_Timing;				///< performance counter
	double cKernelLbmAlpha_Main_Timing;				///< performance counter
	double cKernelLbmAlpha_GasToInterface_Timing;	///< performance counter

	double cKernelLbmAlpha_InterfaceToGas_Timing;	///< performance counter
	double cKernelLbmAlpha_InterfaceToGasNeighbors_Timing;		///< performance counter
	double cKernelLbmAlpha_InterfaceToFluidNeighbors_Timing;	///< performance counter
	double cKernelLbmAlpha_GatherMass_Timing;		///< performance counter

	double cKernelLbmBeta_Pre_Timing;				///< performance counter
	double cKernelLbmBeta_Main_Timing;				///< performance counter
	double cKernelLbmBeta_GasToInterface_Timing;	///< performance counter

	double cKernelLbmBeta_InterfaceToGas_Timing;	///< performance counter
	double cKernelLbmBeta_InterfaceToGasNeighbors_Timing;		///< performance counter
	double cKernelLbmBeta_InterfaceToFluidNeighbors_Timing;		///< performance counter
	double cKernelLbmBeta_GatherMass_Timing;		///< performance counter
#endif

private:
	// initialization kernels
	cl::Kernel cKernelLbmInit;

#if LBM_AA_ALPHA_KERNEL_AS_PROPAGATION
	cl::Kernel cKernelLbmAlpha_Propagation;
#else
	// collision and propagation kernels
	cl::Kernel cKernelLbmAlpha_Pre;
	cl::Kernel cKernelLbmAlpha_Main;
	cl::Kernel cKernelLbmAlpha_GasToInterface;

	cl::Kernel cKernelLbmAlpha_InterfaceToFluidNeighbors;
	cl::Kernel cKernelLbmAlpha_InterfaceToGas;
	cl::Kernel cKernelLbmAlpha_InterfaceToGasNeighbors;
	cl::Kernel cKernelLbmAlpha_GatherMass;
#endif

#if LBM_BETA_AA_KERNEL_AS_PROPAGATION
	cl::Kernel cKernelLbmBeta_Propagation;
#else
	cl::Kernel cKernelLbmBeta_Pre;
	cl::Kernel cKernelLbmBeta_Main;
	cl::Kernel cKernelLbmBeta_GasToInterface;

	cl::Kernel cKernelLbmBeta_InterfaceToFluidNeighbors;
	cl::Kernel cKernelLbmBeta_InterfaceToGas;
	cl::Kernel cKernelLbmBeta_InterfaceToGasNeighbors;
	cl::Kernel cKernelLbmBeta_GatherMass;
#endif

	cl::Kernel cKernelLbmMassScale;

	/**
	 * work group sizes
	 */
	cl::NDRange cKernelLbmInit_WorkGroupSize;

	cl::NDRange cKernelLbmMassScale_WorkGroupSize;

	cl::NDRange cKernelLbmAlpha_Pre_WorkGroupSize;
	cl::NDRange cKernelLbmAlpha_Main_WorkGroupSize;
	cl::NDRange cKernelLbmAlpha_GasToInterface_WorkGroupSize;

	cl::NDRange cKernelLbmAlpha_InterfaceToGas_WorkGroupSize;
	cl::NDRange cKernelLbmAlpha_InterfaceToGasNeighbors_WorkGroupSize;
	cl::NDRange cKernelLbmAlpha_InterfaceToFluidNeighbors_WorkGroupSize;
	cl::NDRange cKernelLbmAlpha_GatherMass_WorkGroupSize;

#if LBM_AA_ALPHA_KERNEL_AS_PROPAGATION
	cl::NDRange cKernelLbmAlpha_Propagation_WorkGroupSize;
#endif

	cl::NDRange cKernelLbmBeta_Pre_WorkGroupSize;
	cl::NDRange cKernelLbmBeta_Main_WorkGroupSize;
	cl::NDRange cKernelLbmBeta_GasToInterface_WorkGroupSize;

	cl::NDRange cKernelLbmBeta_InterfaceToGas_WorkGroupSize;
	cl::NDRange cKernelLbmBeta_InterfaceToGasNeighbors_WorkGroupSize;
	cl::NDRange cKernelLbmBeta_InterfaceToFluidNeighbors_WorkGroupSize;
	cl::NDRange cKernelLbmBeta_GatherMass_WorkGroupSize;

#if LBM_BETA_AA_KERNEL_AS_PROPAGATION
	cl::NDRange cKernelLbmBeta_Propagation_WorkGroupSize;
#endif
	/**
	 * maximum registers available in single thread
	 */
	size_t cKernelLbmInit_MaxRegisters;

	size_t cKernelLbmMassScale_MaxRegisters;

	size_t cKernelLbmAlpha_Pre_MaxRegisters;
	size_t cKernelLbmAlpha_Main_MaxRegisters;
	size_t cKernelLbmAlpha_GasToInterface_MaxRegisters;

	size_t cKernelLbmAlpha_InterfaceToGas_MaxRegisters;
	size_t cKernelLbmAlpha_InterfaceToGasNeighbors_MaxRegisters;
	size_t cKernelLbmAlpha_InterfaceToFluidNeighbors_MaxRegisters;
	size_t cKernelLbmAlpha_GatherMass_MaxRegisters;

#if LBM_AA_ALPHA_KERNEL_AS_PROPAGATION
	size_t cKernelLbmAlpha_Propagation_MaxRegisters;
#endif

	size_t cKernelLbmBeta_Pre_MaxRegisters;
	size_t cKernelLbmBeta_Main_MaxRegisters;
	size_t cKernelLbmBeta_GasToInterface_MaxRegisters;

	size_t cKernelLbmBeta_InterfaceToGas_MaxRegisters;
	size_t cKernelLbmBeta_InterfaceToGasNeighbors_MaxRegisters;
	size_t cKernelLbmBeta_InterfaceToFluidNeighbors_MaxRegisters;
	size_t cKernelLbmBeta_GatherMass_MaxRegisters;

#if LBM_BETA_AA_KERNEL_AS_PROPAGATION
	size_t cKernelLbmBeta_Propagation_MaxRegisters;
#endif



	cl::NDRange global_work_group_size;

	size_t max_local_work_group_size;


public:

#if LBM_AA_ALPHA_KERNEL_AS_PROPAGATION	|| LBM_BETA_AA_KERNEL_AS_PROPAGATION
	cl::Buffer cMemNewDensityDistributions;
#endif

	/**
	 * Setup class with OpenCL skeleton.
	 *
	 * No simulation kernels are loaded in this constructor!
	 */
	CLbmOpenClAA(	const CCLSkeleton &cClSkeleton,
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

		initInterface(		p_domain_cells,
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
		CError_AppendReturnThis(this->params);

		CL_CHECK_ERROR(cKernelLbmInit.setArg(4, this->params.inv_tau));
		CL_CHECK_ERROR(cKernelLbmInit.setArg(5, this->params.inv_trt_tau));

		CL_CHECK_ERROR(cKernelLbmInit.setArg(10, this->params.gravitation[0]));
		CL_CHECK_ERROR(cKernelLbmInit.setArg(11, this->params.gravitation[1]));
		CL_CHECK_ERROR(cKernelLbmInit.setArg(12, this->params.gravitation[2]));

//		std::cout << params.inv_tau << " " << params.gravitation << std::endl;

#if !LBM_AA_ALPHA_KERNEL_AS_PROPAGATION
		CL_CHECK_ERROR(cKernelLbmAlpha_Main.setArg(4, this->params.inv_tau));
		CL_CHECK_ERROR(cKernelLbmAlpha_Main.setArg(5, this->params.inv_trt_tau));

		CL_CHECK_ERROR(cKernelLbmAlpha_Main.setArg(10, this->params.gravitation[0]));
		CL_CHECK_ERROR(cKernelLbmAlpha_Main.setArg(11, this->params.gravitation[1]));
		CL_CHECK_ERROR(cKernelLbmAlpha_Main.setArg(12, this->params.gravitation[2]));

		CL_CHECK_ERROR(cKernelLbmAlpha_Pre.setArg(6, this->params.mass_exchange_factor));
		CL_CHECK_ERROR(cKernelLbmAlpha_Main.setArg(13, this->params.mass_exchange_factor));

		CL_CHECK_ERROR(cKernelLbmAlpha_InterfaceToFluidNeighbors.setArg(6, this->params.mass_exchange_factor));
		CL_CHECK_ERROR(cKernelLbmAlpha_InterfaceToGas.setArg(6, this->params.mass_exchange_factor));
//		CL_CHECK_ERROR(cKernelLbmAlpha_InterfaceToGasNeighbors.setArg(6, this->params.mass_exchange_factor));
		CL_CHECK_ERROR(cKernelLbmAlpha_GasToInterface.setArg(6, this->params.mass_exchange_factor));
#endif

#if !LBM_BETA_AA_KERNEL_AS_PROPAGATION
		CL_CHECK_ERROR(cKernelLbmBeta_Main.setArg(4, this->params.inv_tau));
		CL_CHECK_ERROR(cKernelLbmBeta_Main.setArg(5, this->params.inv_trt_tau));

		CL_CHECK_ERROR(cKernelLbmBeta_Main.setArg(10, this->params.gravitation[0]));
		CL_CHECK_ERROR(cKernelLbmBeta_Main.setArg(11, this->params.gravitation[1]));
		CL_CHECK_ERROR(cKernelLbmBeta_Main.setArg(12, this->params.gravitation[2]));

		CL_CHECK_ERROR(cKernelLbmBeta_Pre.setArg(6, this->params.mass_exchange_factor));
		CL_CHECK_ERROR(cKernelLbmBeta_Main.setArg(13, this->params.mass_exchange_factor));

		CL_CHECK_ERROR(cKernelLbmBeta_InterfaceToFluidNeighbors.setArg(6, this->params.mass_exchange_factor));
		CL_CHECK_ERROR(cKernelLbmBeta_InterfaceToGas.setArg(6, this->params.mass_exchange_factor));
//		CL_CHECK_ERROR(cKernelLbmBeta_InterfaceToGasNeighbors.setArg(6, this->params.mass_exchange_factor));
		CL_CHECK_ERROR(cKernelLbmBeta_GasToInterface.setArg(6, this->params.mass_exchange_factor));
#endif
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
#if LBM_AA_ALPHA_KERNEL_AS_PROPAGATION || LBM_BETA_AA_KERNEL_AS_PROPAGATION
		cMemNewDensityDistributions = cl::Buffer(this->cl.cContext, CL_MEM_READ_WRITE, sizeof(T)*this->domain_cells_count*this->SIZE_DD_HOST);
#endif

		global_work_group_size = cl::NDRange(this->domain_cells_count);

		if (max_local_work_group_size == 0)
		{
			cKernelLbmInit_WorkGroupSize = 0;

			cKernelLbmMassScale_WorkGroupSize = 0;

			cKernelLbmAlpha_Pre_WorkGroupSize = 0;
			cKernelLbmAlpha_Main_WorkGroupSize = 0;
			cKernelLbmAlpha_GasToInterface_WorkGroupSize = 0;

			cKernelLbmAlpha_InterfaceToGas_WorkGroupSize = 0;
			cKernelLbmAlpha_InterfaceToGasNeighbors_WorkGroupSize = 0;
			cKernelLbmAlpha_InterfaceToFluidNeighbors_WorkGroupSize = 0;
			cKernelLbmAlpha_GatherMass_WorkGroupSize = 0;

			cKernelLbmBeta_Pre_WorkGroupSize = 0;
			cKernelLbmBeta_Main_WorkGroupSize = 0;
			cKernelLbmBeta_GasToInterface_WorkGroupSize = 0;

			cKernelLbmBeta_InterfaceToGas_WorkGroupSize = 0;
			cKernelLbmBeta_InterfaceToGasNeighbors_WorkGroupSize = 0;
			cKernelLbmBeta_InterfaceToFluidNeighbors_WorkGroupSize = 0;
			cKernelLbmBeta_GatherMass_WorkGroupSize = 0;

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
				variable##_WorkGroupSize = cl::NDRange(*it != 0 ? *it : max_local_work_group_size);	\
				it++;														\
			}																\
			else															\
			{																\
				variable##_WorkGroupSize = cl::NDRange(max_local_work_group_size);		\
			}																\
																			\
			if (ir != this->lbm_opencl_number_of_registers_list.end())		\
			{																\
				variable##_MaxRegisters = *ir != 0 ? *ir : 0;				\
				ir++;														\
			}																\
			else															\
			{																\
				variable##_MaxRegisters  = 0;								\
			}

			std::list<int>::iterator it = this->lbm_opencl_number_of_threads_list.begin();
			std::list<int>::iterator ir = this->lbm_opencl_number_of_registers_list.begin();


			INIT_WORK_GROUP_SIZE(cKernelLbmInit);


			INIT_WORK_GROUP_SIZE(cKernelLbmAlpha_Pre);
			INIT_WORK_GROUP_SIZE(cKernelLbmAlpha_Main);
			INIT_WORK_GROUP_SIZE(cKernelLbmAlpha_GasToInterface);

			INIT_WORK_GROUP_SIZE(cKernelLbmAlpha_InterfaceToGas);
			INIT_WORK_GROUP_SIZE(cKernelLbmAlpha_InterfaceToGasNeighbors);
			INIT_WORK_GROUP_SIZE(cKernelLbmAlpha_InterfaceToFluidNeighbors);
			INIT_WORK_GROUP_SIZE(cKernelLbmAlpha_GatherMass);

			INIT_WORK_GROUP_SIZE(cKernelLbmBeta_Pre);
			INIT_WORK_GROUP_SIZE(cKernelLbmBeta_Main);
			INIT_WORK_GROUP_SIZE(cKernelLbmBeta_GasToInterface);

			INIT_WORK_GROUP_SIZE(cKernelLbmBeta_InterfaceToGas);
			INIT_WORK_GROUP_SIZE(cKernelLbmBeta_InterfaceToGasNeighbors);
			INIT_WORK_GROUP_SIZE(cKernelLbmBeta_InterfaceToFluidNeighbors);
			INIT_WORK_GROUP_SIZE(cKernelLbmBeta_GatherMass);

#if LBM_AA_ALPHA_KERNEL_AS_PROPAGATION
			INIT_WORK_GROUP_SIZE(cKernelLbmAlpha_Propagation);
#endif
#if LBM_BETA_AA_KERNEL_AS_PROPAGATION
			INIT_WORK_GROUP_SIZE(cKernelLbmBeta_Propagation);
#endif

			INIT_WORK_GROUP_SIZE(cKernelLbmMassScale);


#undef to_str
#undef INIT_WORK_GROUP_SIZE
			createKernels(false);
		}

		resetTimings();
	}

	/**
	 * reset the performance counters
	 */
	void resetTimings()
	{
#if AA_TIMING
		cBetaTimingsCount = 0.0;

		cKernelLbmAlpha_Pre_Timing = 0.0;
		cKernelLbmAlpha_Main_Timing = 0.0;
		cKernelLbmAlpha_GasToInterface_Timing = 0.0;

		cKernelLbmAlpha_InterfaceToGas_Timing = 0.0;
		cKernelLbmAlpha_InterfaceToGasNeighbors_Timing = 0.0;
		cKernelLbmAlpha_InterfaceToFluidNeighbors_Timing = 0.0;
		cKernelLbmAlpha_GatherMass_Timing = 0.0;

		cKernelLbmBeta_Pre_Timing = 0.0;
		cKernelLbmBeta_Main_Timing = 0.0;
		cKernelLbmBeta_GasToInterface_Timing = 0.0;

		cKernelLbmBeta_InterfaceToGas_Timing = 0.0;
		cKernelLbmBeta_InterfaceToGasNeighbors_Timing = 0.0;
		cKernelLbmBeta_InterfaceToFluidNeighbors_Timing = 0.0;
		cKernelLbmBeta_GatherMass_Timing = 0.0;


		cAlphaTimingsCount = 0.0;

		cKernelLbmAlpha_Pre_Timing_Sum = 0.0;
		cKernelLbmAlpha_Main_Timing_Sum = 0.0;
		cKernelLbmAlpha_GasToInterface_Timing_Sum = 0.0;

		cKernelLbmAlpha_InterfaceToGas_Timing_Sum = 0.0;
		cKernelLbmAlpha_InterfaceToGasNeighbors_Timing_Sum = 0.0;
		cKernelLbmAlpha_InterfaceToFluidNeighbors_Timing_Sum = 0.0;
		cKernelLbmAlpha_GatherMass_Timing_Sum = 0.0;

		cKernelLbmBeta_Pre_Timing_Sum = 0.0;
		cKernelLbmBeta_Main_Timing_Sum = 0.0;
		cKernelLbmBeta_GasToInterface_Timing_Sum = 0.0;

		cKernelLbmBeta_InterfaceToGas_Timing_Sum = 0.0;
		cKernelLbmBeta_InterfaceToGasNeighbors_Timing_Sum = 0.0;
		cKernelLbmBeta_InterfaceToFluidNeighbors_Timing_Sum = 0.0;
		cKernelLbmBeta_GatherMass_Timing_Sum = 0.0;
#endif
	}

	void createKernels(bool test_local_work_group_size)
	{
		/*
		 * prepare CL Program
		 */

		cl::Program cProgramInit;
		loadProgram(cProgramInit, cKernelLbmInit_WorkGroupSize, cKernelLbmInit_MaxRegisters, this->cl_interface_program_defines.str(), "data/cl_programs/lbm_init.cl", test_local_work_group_size);

		// INTERFACE CONVERSIONS (shared between ALPHA and BETA kernels)
		CLBM_LOAD_PROGRAM(cProgram_InterfaceToGas,			cKernelLbmAlpha_InterfaceToGas_WorkGroupSize,			cKernelLbmAlpha_InterfaceToGas_MaxRegisters,			"data/cl_programs/lbm_interface_to_gas.cl");
		CLBM_LOAD_PROGRAM(cProgram_InterfaceToGasNeighbors,	cKernelLbmAlpha_InterfaceToGasNeighbors_WorkGroupSize,	cKernelLbmAlpha_InterfaceToGasNeighbors_MaxRegisters,	"data/cl_programs/lbm_interface_to_gas_neighbors.cl");
		CLBM_LOAD_PROGRAM(cProgram_InterfaceToFluidNeighbors,	cKernelLbmAlpha_InterfaceToFluidNeighbors_WorkGroupSize,	cKernelLbmAlpha_InterfaceToFluidNeighbors_MaxRegisters,	"data/cl_programs/lbm_interface_to_fluid_neighbors.cl");
		CLBM_LOAD_PROGRAM(cProgram_GatherMass,				cKernelLbmAlpha_GatherMass_WorkGroupSize, 				cKernelLbmAlpha_GatherMass_MaxRegisters,				"data/cl_programs/lbm_gather_mass.cl");

		/****************
		 * ALPHA
		 ****************/
#if LBM_AA_ALPHA_KERNEL_AS_PROPAGATION
		CLBM_LOAD_PROGRAM(cProgramAlpha_Propagation,		cKernelLbmAlpha_Propagation_WorkGroupSize,		cKernelLbmAlpha_Propagation_MaxRegisters,	"data/cl_programs/lbm_debug_alpha_propagation.cl");
#else
		CLBM_LOAD_PROGRAM(cProgramAlpha_Pre,				cKernelLbmAlpha_Pre_WorkGroupSize,				cKernelLbmAlpha_Pre_MaxRegisters,		"data/cl_programs/lbm_alpha_pre.cl");
		CLBM_LOAD_PROGRAM(cProgramAlpha_Main,				cKernelLbmAlpha_Main_WorkGroupSize,				cKernelLbmAlpha_Main_MaxRegisters,		"data/cl_programs/lbm_alpha.cl");
		CLBM_LOAD_PROGRAM(cProgramAlpha_GasToInterface,	cKernelLbmAlpha_GasToInterface_WorkGroupSize, 	cKernelLbmAlpha_GasToInterface_MaxRegisters,	"data/cl_programs/lbm_alpha_flag_gas_to_interface.cl");
#endif
		/****************
		 * BETA
		 ****************/
#if LBM_BETA_AA_KERNEL_AS_PROPAGATION
		CLBM_LOAD_PROGRAM(cProgramBeta_Propagation,		cKernelLbmBeta_Propagation_WorkGroupSize,		cKernelLbmBeta_Propagation_MaxRegisters,	"data/cl_programs/lbm_debug_beta_propagation.cl");
#else
		CLBM_LOAD_PROGRAM(cProgramBeta_Pre,				cKernelLbmBeta_Pre_WorkGroupSize,				cKernelLbmBeta_Pre_MaxRegisters,	"data/cl_programs/lbm_beta_pre.cl");
		CLBM_LOAD_PROGRAM(cProgramBeta_Main,				cKernelLbmBeta_Main_WorkGroupSize,				cKernelLbmBeta_Main_MaxRegisters,	"data/cl_programs/lbm_beta.cl");
		CLBM_LOAD_PROGRAM(cProgramBeta_GasToInterface,	cKernelLbmBeta_GasToInterface_WorkGroupSize,	cKernelLbmBeta_GasToInterface_MaxRegisters,	"data/cl_programs/lbm_beta_flag_gas_to_interface.cl");
#endif

		CLBM_LOAD_PROGRAM(cProgramMassScale, cKernelLbmMassScale_WorkGroupSize, cKernelLbmMassScale_MaxRegisters, "data/cl_programs/lbm_mass_scale.cl");


		/**********************************************************
		 * mass scaling kernel
		 **********************************************************/
		CLBM_CREATE_KERNEL_(	cKernelLbmMassScale, cProgramMassScale, "kernel_lbm_mass_scale");
		cKernelLbmMassScale.setArg(0, this->cMemFluidMass);

		/**********************************************************
		 * initialization kernel
		 **********************************************************/
		CLBM_CREATE_KERNEL_13(	cKernelLbmInit, cProgramInit, "kernel_lbm_init",
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
		 * collision and propagation kernel (ALPHA)
		 **********************************************************/
#if !LBM_AA_ALPHA_KERNEL_AS_PROPAGATION
		// PRE ALPHA KERNEL
		CLBM_CREATE_KERNEL_7(	cKernelLbmAlpha_Pre, cProgramAlpha_Pre, "kernel_lbm_alpha_pre",
						this->cMemDensityDistributions,
						this->cMemCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemFluidFraction,
						this->params.mass_exchange_factor
		);

		// ALPHA MAIN
		CLBM_CREATE_KERNEL_14(	cKernelLbmAlpha_Main, cProgramAlpha_Main, "kernel_lbm_alpha",
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

		// INTERFACE TO FLUID NEIGHBORS
		CLBM_CREATE_KERNEL_7(	cKernelLbmAlpha_InterfaceToFluidNeighbors, cProgram_InterfaceToFluidNeighbors, "kernel_interface_to_fluid_neighbors",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);

		// INTERFACE TO GAS
		CLBM_CREATE_KERNEL_7(	cKernelLbmAlpha_InterfaceToGas, cProgram_InterfaceToGas, "kernel_interface_to_gas",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);

		// INTERFACE TO GAS NEIGHBORS
		CLBM_CREATE_KERNEL_7(	cKernelLbmAlpha_InterfaceToGasNeighbors, cProgram_InterfaceToGasNeighbors, "kernel_interface_to_gas_neighbors",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->cMemFluidFraction	// use fluid fraction as temporary buffer
		);

		// GATHER MASS
		CLBM_CREATE_KERNEL_3(	cKernelLbmAlpha_GatherMass, cProgram_GatherMass, "kernel_gather_mass",
						this->cMemNewCellFlags,
						this->cMemFluidMass,
						this->cMemFluidFraction
		);

		// ALPHA GAS TO INTERFACE CONVERSION
		CLBM_CREATE_KERNEL_7(	cKernelLbmAlpha_GasToInterface, cProgramAlpha_GasToInterface, "kernel_lbm_alpha_flag_gas_to_interface",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);

#else
		/**********************************************************
		 * propagation kernel (ALPHA)
		 **********************************************************/

		CLBM_CREATE_KERNEL_2(	cKernelLbmAlpha_Propagation, cProgramAlpha_Propagation, "kernel_debug_alpha_propagation",
						this->cMemDensityDistributions,
						this->cMemNewDensityDistributions
		);

#endif


		/**********************************************************
		 * collision and propagation kernel (BETA)
		 **********************************************************/

#if !LBM_BETA_AA_KERNEL_AS_PROPAGATION
		// PRE BETA KERNEL

		CLBM_CREATE_KERNEL_7(	cKernelLbmBeta_Pre, cProgramBeta_Pre, "kernel_beta_pre",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->params.mass_exchange_factor
		);

		// BETA MAIN
		CLBM_CREATE_KERNEL_14(	cKernelLbmBeta_Main, cProgramBeta_Main, "kernel_beta",
						this->cMemDensityDistributions,
						this->cMemNewCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->params.inv_tau,
						this->params.inv_trt_tau,
						this->cMemFluidMass,
						this->cMemNewFluidFraction,
						this->cMemCellFlags,
						this->cMemFluidFraction,

						this->params.gravitation[0],
						this->params.gravitation[1],
						this->params.gravitation[2],

						this->params.mass_exchange_factor
		);

		// INTERFACE TO FLUID NEIGHBORS
		CLBM_CREATE_KERNEL_7(	cKernelLbmBeta_InterfaceToFluidNeighbors, cProgram_InterfaceToFluidNeighbors, "kernel_interface_to_fluid_neighbors",
						this->cMemDensityDistributions,
						this->cMemCellFlags,
						this->cMemVelocity,
						this->cMemDensity,
						this->cMemFluidMass,
						this->cMemFluidFraction,
						this->params.mass_exchange_factor
		);

		// INTERFACE TO GAS
		CLBM_CREATE_KERNEL_7(	cKernelLbmBeta_InterfaceToGas, cProgram_InterfaceToGas, "kernel_interface_to_gas",
				this->cMemDensityDistributions,
				this->cMemCellFlags,
				this->cMemVelocity,
				this->cMemDensity,
				this->cMemFluidMass,
				this->cMemFluidFraction,
				this->params.mass_exchange_factor
		);

		// INTERFACE TO GAS NEIGHBORS
		CLBM_CREATE_KERNEL_7(	cKernelLbmBeta_InterfaceToGasNeighbors, cProgram_InterfaceToGasNeighbors, "kernel_interface_to_gas_neighbors",
				this->cMemDensityDistributions,
				this->cMemCellFlags,
				this->cMemVelocity,
				this->cMemDensity,
				this->cMemFluidMass,
				this->cMemFluidFraction,
				this->cMemNewFluidFraction	// use fluid fraction as temporary buffer
		);

		// BETA GAS TO INTERFACE CONVERSION
		CLBM_CREATE_KERNEL_7(	cKernelLbmBeta_GasToInterface, cProgramBeta_GasToInterface, "lbm_beta_flag_gas_to_interface",
				this->cMemDensityDistributions,
				this->cMemCellFlags,
				this->cMemVelocity,
				this->cMemDensity,
				this->cMemFluidMass,
				this->cMemFluidFraction,
				this->params.mass_exchange_factor
		);

		// GATHER MASS
		CLBM_CREATE_KERNEL_3(	cKernelLbmBeta_GatherMass, cProgram_GatherMass, "kernel_gather_mass",
						this->cMemCellFlags,
						this->cMemFluidMass,
						this->cMemNewFluidFraction
		);
#else
		/**********************************************************
		 * propagation kernel (BETA)
		 **********************************************************/

		CLBM_CREATE_KERNEL_2(	cKernelLbmBeta_Propagation, cProgramBeta_Propagation, "kernel_debug_beta_propagation",
						this->cMemDensityDistributions,
						this->cMemNewDensityDistributions
		);
#endif

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

		cKernelLbmInit.setArg(13, this->fluid_init_flags);
		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmInit,	// kernel
														cl::NullRange,			// global work offset
														global_work_group_size,
														cKernelLbmInit_WorkGroupSize
						);

		this->cl.cCommandQueue.enqueueBarrierWithWaitList();

		this->resetFluid_Interface();

		if (this->verbose)
			std::cout << "OK" << std::endl;
	}


	/**
	 * start one simulation step (enqueue kernels)
	 */
	void simulationStepDebugTimings()
	{
#if 0
		/*
		 * collision kernels are inserted as 1d kernels because they work cell wise without neighboring information
		 */

		CStopwatch cStopwatch;

		if (this->simulation_step_counter & 1)
		{
			// BETA KERNEL
#if !LBM_BETA_AA_KERNEL_AS_PROPAGATION
			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_Pre,	// kernel
													cl::NullRange,				// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_Pre_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmBeta_Pre_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_Main,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_Main_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmBeta_Main_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_InterfaceToFluidNeighbors,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_InterfaceToFluidNeighbors_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmBeta_InterfaceToFluidNeighbors_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_InterfaceToGas,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_InterfaceToGas_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmBeta_InterfaceToGas_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_InterfaceToGasNeighbors,	// kernel
													cl::NullRange,									// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_InterfaceToGasNeighbors_WorkGroupSize)
							);

			this->cl.cCommandQueue.finish();
			cKernelLbmBeta_InterfaceToGasNeighbors_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_GasToInterface,	// kernel
													cl::NullRange,							// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_GasToInterface_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmBeta_GasToInterface_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_GatherMass,	// kernel
													cl::NullRange,							// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_GatherMass_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmBeta_GasToInterface_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_GatherMass,	// kernel
													cl::NullRange,							// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_GatherMass_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmBeta_GatherMass_Timing_Sum += cStopwatch.getTime();

			cBetaTimingsCount += 1.0;
			double div = 1.0/cBetaTimingsCount;

			cKernelLbmBeta_Pre_Timing = cKernelLbmBeta_Pre_Timing_Sum*div;
			cKernelLbmBeta_Main_Timing = cKernelLbmBeta_Main_Timing_Sum*div;
			cKernelLbmBeta_GasToInterface_Timing = cKernelLbmBeta_GasToInterface_Timing_Sum*div;

			cKernelLbmBeta_InterfaceToGas_Timing = cKernelLbmBeta_InterfaceToGas_Timing_Sum*div;
			cKernelLbmBeta_InterfaceToGasNeighbors_Timing = cKernelLbmBeta_InterfaceToGasNeighbors_Timing_Sum*div;
			cKernelLbmBeta_InterfaceToFluidNeighbors_Timing = cKernelLbmBeta_InterfaceToFluidNeighbors_Timing_Sum*div;
			cKernelLbmBeta_GatherMass_Timing = cKernelLbmBeta_GatherMass_Timing_Sum*div;
#else
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_Propagation,     // kernel
                                                    0,                              // global work offset

														global_work_group_size,
														cl::NDRange(cKernelLbmBeta_Propagation_WorkGroupSize)
                                            );
			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemNewDensityDistributions,
													this->cMemDensityDistributions,
                                                    0,
                                                    0,
                                                    this->domain_cells_count*sizeof(T)*19
                                            );

			this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemNewFluidFraction,
													this->cMemFluidFraction,
                                                    0,
                                                    0,
                                                    this->domain_cells_count*sizeof(T)
                                            );

			this->cl.cCommandQueue.enqueueCopyBuffer(		this->cMemNewCellFlags,
													this->cMemCellFlags,
                                                    0,
                                                    0,
                                                    this->domain_cells_count*sizeof(T)
                                            );

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

#endif
		}
		else
		{
			// ALPHA KERNEL
#if !LBM_AA_ALPHA_KERNEL_AS_PROPAGATION
			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_Pre,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_Pre_WorkGroupSize)
							);

			this->cl.cCommandQueue.finish();
			cKernelLbmAlpha_Pre_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_Main,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_Main_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmAlpha_Main_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_InterfaceToFluidNeighbors,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_InterfaceToFluidNeighbors_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmAlpha_InterfaceToFluidNeighbors_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_InterfaceToGas,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_InterfaceToGas_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmAlpha_InterfaceToGas_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_InterfaceToGasNeighbors,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_InterfaceToGasNeighbors_WorkGroupSize)
							);

			this->cl.cCommandQueue.finish();
			cKernelLbmAlpha_InterfaceToGasNeighbors_Timing_Sum += cStopwatch.getTime();


			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_GasToInterface,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_GasToInterface_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmAlpha_GasToInterface_Timing_Sum += cStopwatch.getTime();

			cStopwatch.start();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_GatherMass,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_GatherMass_WorkGroupSize)
							);
			this->cl.cCommandQueue.finish();
			cKernelLbmAlpha_GatherMass_Timing_Sum += cStopwatch.getTime();

			cAlphaTimingsCount += 1.0;
			double div = 1.0/cAlphaTimingsCount;

			cKernelLbmAlpha_Pre_Timing = cKernelLbmAlpha_Pre_Timing_Sum*div;
			cKernelLbmAlpha_Main_Timing = cKernelLbmAlpha_Main_Timing_Sum*div;
			cKernelLbmAlpha_GasToInterface_Timing = cKernelLbmAlpha_GasToInterface_Timing_Sum*div;

			cKernelLbmAlpha_InterfaceToGas_Timing = cKernelLbmAlpha_InterfaceToGas_Timing_Sum*div;
			cKernelLbmAlpha_InterfaceToGasNeighbors_Timing = cKernelLbmAlpha_InterfaceToGasNeighbors_Timing_Sum*div;
			cKernelLbmAlpha_InterfaceToFluidNeighbors_Timing = cKernelLbmAlpha_InterfaceToFluidNeighbors_Timing_Sum*div;
			cKernelLbmAlpha_GatherMass_Timing = cKernelLbmAlpha_GatherMass_Timing_Sum*div;
#else
			this->cl.cCommandQueue.enqueueNDRangeKernel(     cKernelLbmAlpha_Propagation,    // kernel
														1,                              // dimensions
														0,                              // global work offset

														global_work_group_size,
														&cKernelLbmAlpha_Propagation_WorkGroupSize
                                            );

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueCopyBuffer(	this->cMemNewDensityDistributions,
												this->cMemDensityDistributions,
												0,
												0,
												this->domain_cells_count*sizeof(T)*19
                                            );

			this->cl.cCommandQueue.enqueueCopyBuffer(	this->cMemFluidFraction,
												this->cMemNewFluidFraction,
												0,
												0,
												this->domain_cells_count*sizeof(T)
                                            );

			this->cl.cCommandQueue.enqueueCopyBuffer(	this->cMemCellFlags,
												this->cMemNewCellFlags,
												0,
												0,
												this->domain_cells_count*sizeof(T)
                                            );

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

#endif
		}
#endif
		this->simulation_step_counter++;
	}


	/**
	 * scale the mass in each cell to stabilize the overall amount of mass in the simulation
	 */
	void scaleMass(T mass_scale_factor)
	{
		cKernelLbmMassScale.setArg(1, mass_scale_factor);

		this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmMassScale,	// kernel
														cl::NullRange,				// global work offset
														global_work_group_size,
														cl::NDRange(cKernelLbmMassScale_WorkGroupSize)
						);

		this->cl.cCommandQueue.enqueueBarrierWithWaitList();
	}

	/**
	 * start one simulation step (enqueue kernels)
	 */
	void simulationStep()
	{
		/*
		 * collision kernels are inserted as 1d kernels because they work cell wise without neighboring information
		 */

		if (this->simulation_step_counter & 1)
		{
#if !LBM_BETA_AA_KERNEL_AS_PROPAGATION
//			std::cout << "beta aa" << std::endl;
			// BETA KERNEL
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_Pre,	// kernel
													cl::NullRange,				// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_Pre_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_Main,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_Main_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_InterfaceToFluidNeighbors,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_InterfaceToFluidNeighbors_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_InterfaceToGas,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_InterfaceToGas_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_InterfaceToGasNeighbors,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_InterfaceToGasNeighbors_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_GatherMass,	// kernel
													cl::NullRange,				// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_GatherMass_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_GasToInterface,	// kernel
													cl::NullRange,				// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmBeta_GasToInterface_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

#else
//			std::cout << "beta propagation" << std::endl;
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmBeta_Propagation,     // kernel
														cl::NullRange,                              // global work offset
														global_work_group_size,
														cl::NDRange(cKernelLbmBeta_Propagation_WorkGroupSize)
                                            );

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueCopyBuffer(	this->cMemNewDensityDistributions,
												this->cMemDensityDistributions,
												0,
												0,
												this->domain_cells_count*sizeof(T)*19
                                            );

			this->cl.cCommandQueue.enqueueCopyBuffer(	this->cMemNewFluidFraction,
												this->cMemFluidFraction,
												0,
												0,
												this->domain_cells_count*sizeof(T)
                                            );

			this->cl.cCommandQueue.enqueueCopyBuffer(	this->cMemNewCellFlags,
												this->cMemCellFlags,
												0,
												0,
												this->domain_cells_count*sizeof(T)
                                            );

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();
#endif
		}
		else
		{
#if !LBM_AA_ALPHA_KERNEL_AS_PROPAGATION
			// ALPHA KERNEL
//			std::cout << "alpha aa" << std::endl;
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_Pre,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_Pre_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_Main,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_Main_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_InterfaceToFluidNeighbors,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_InterfaceToFluidNeighbors_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_InterfaceToGas,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_InterfaceToGas_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_InterfaceToGasNeighbors,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_InterfaceToGasNeighbors_WorkGroupSize)
							);
			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_GatherMass,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_GatherMass_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_GasToInterface,	// kernel
													cl::NullRange,					// global work offset
													global_work_group_size,
													cl::NDRange(cKernelLbmAlpha_GasToInterface_WorkGroupSize)
							);

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

#else
//			std::cout << "alpha propagation" << std::endl;
			this->cl.cCommandQueue.enqueueNDRangeKernel(	cKernelLbmAlpha_Propagation,    // kernel
                                                    cl::NullRange,                              // global work offset
                    								global_work_group_size,
                    								cl::NDRange(cKernelLbmAlpha_Propagation_WorkGroupSize)
                                            );

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

			this->cl.cCommandQueue.enqueueCopyBuffer(	this->cMemNewDensityDistributions,
												this->cMemDensityDistributions,
												0,
												0,
												this->domain_cells_count*sizeof(T)*19
                                            );

			this->cl.cCommandQueue.enqueueCopyBuffer(	this->cMemFluidFraction,
												this->cMemNewFluidFraction,
												0,
												0,
												this->domain_cells_count*sizeof(T)
                                            );

			this->cl.cCommandQueue.enqueueCopyBuffer(	this->cMemCellFlags,
												this->cMemNewCellFlags,
												0,
												0,
												this->domain_cells_count*sizeof(T)
                                            );

			this->cl.cCommandQueue.enqueueBarrierWithWaitList();

#endif
		}

		this->simulation_step_counter++;
	}
};

#endif
