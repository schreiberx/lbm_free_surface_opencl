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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <list>


#ifdef GL_INTEROP
	// define GL_INTEROP to load OpenGL INTEROP support
	#define LBM_OPENCL_GL_INTEROP
#endif

#include "CMainVisualization.hpp"

extern "C" {
	#include "malloc.h"
}

#include "lbm/CLbmOpenClAA.hpp"
#include "lbm/CLbmOpenClAB_1.hpp"
#include "lbm/CLbmOpenClAB_2.hpp"
#include "lbm/CLbmOpenClAB_1_shared_memory.hpp"

#include "libopencl/CCLSkeleton.hpp"
#include "lib/CStopwatch.hpp"

#include "libmath/CVector.hpp"

#include "CWiiBalanceBoard.hpp"

#if 1
	typedef float	T;
#else
	// double types not tested! (shouldn't work with opengl context)
	//typedef double	T;
#endif

void extract_comma_separated_integers(std::list<int> &int_list, std::string &int_string)
{
	size_t start_pos = 0;

	size_t comma_pos;

	comma_pos = int_string.find_first_of(',', start_pos);

	while (comma_pos != std::string::npos)
	{
		std::string substring = int_string.substr(start_pos, comma_pos-start_pos);

		int number;

		if (substring.empty())	number = 0;
		else					number = atoi(substring.c_str());

		int_list.push_back(number);
		start_pos = comma_pos+1;
		comma_pos = int_string.find_first_of(',', start_pos);
	}
	int_list.push_back(atoi(int_string.substr(start_pos).c_str()));
}


int run(int argc, char *argv[])
{
	bool verbose = false;

	/**
	 * DEFAULT SIMULATION PARAMETERS
	 */
#if 0
	CVector<3,int> domain_cells(32,32,32);	///< resolution of simulation domain
	T domain_length_x = 0.030;				///< length of of domain in x direction - used to compute the cell size
	T viscosity = 0.000001;					///< viscosity
	CVector<3,T> gravitation(0,-9.81,0);	///< gravitation
	T timestep = -1;						///< default timestep size - default: -1 for automatic computation of timestep size from other parameters
	T max_sim_gravitation_length = 0.15;	///< maximum length of dimensionless gravitation
	T mass_exchange_factor = 1.0;			///< experimental: factor to multiply mass exchange with for accelleration
#elif 1
	CVector<3,int> domain_cells(64,64,32);	///< resolution of simulation domain
	T domain_length_x = 0.3;				///< length of of domain in x direction - used to compute the cell size
	T viscosity = 0.0001;					///< viscosity
	CVector<3,T> gravitation(0,-9.81,0);	///< gravitation
	T timestep = -1;						///< default timestep size - default: -1 for automatic computation of timestep size from other parameters
	T max_sim_gravitation_length = 1.0;		///< maximum length of dimensionless gravitation
	T mass_exchange_factor = 1.0;			///< experimental: factor to multiply mass exchange with for accelleration
#else
	CVector<3,int> domain_cells(32,32,32);	///< resolution of simulation domain
	T domain_length_x = 0.030;				///< length of of domain in x direction - used to compute the cell size
	T viscosity = 0.0001;					///< viscosity
	CVector<3,T> gravitation(0,-9.81,0);	///< gravitation
	T timestep = -1;						///< default timestep size - default: -1 for automatic computation of timestep size from other parameters

	// when setting this value above 0.2, you have to limit the velocity in the simulation!
	T max_sim_gravitation_length = 0.8;	///< maximum length of dimensionless gravitation
	T mass_exchange_factor = 20.0;			///< experimental: factor to multiply mass exchange with for accelleration
#endif

	/**
	 * MODULE FLAGS
	 */
	bool load_lbm_simulation = false;
	bool load_opencl = false;
	bool load_gui = false;

	int lbm_implementation_nr = 0;

//	bool pause = false;

	int init_flag = -1;

	size_t computation_kernel_count = 0;

	int device_nr = -1;
	int platform_nr = -1;

	std::string number_of_registers_string;	///< string storing the number of registers for opencl threads separated with comma
	std::string number_of_threads_string;		///< string storing the number of threads for opencl separated with comma

	char *balance_board_addr = NULL;	// bluetooth mac address of balance board

	int simulation_loops = 100;

	char optchar;
	while ((optchar = getopt(argc, argv, "a:d:x:y:z:D:vr:q:k:G:pt:sP:l:u:ncgX:m:R:T:i:b:")) > 0)
	{
		switch(optchar)
		{
			case 'b':
				balance_board_addr = optarg;
				break;

			case 'a':
				lbm_implementation_nr = atoi(optarg);
				break;

			case 'R':
				number_of_registers_string = optarg;
				break;

			case 'T':
				number_of_threads_string = optarg;
				break;

#if !WIN32
			case 'd':
				setenv("DISPLAY", optarg, 1);
				break;
#endif
			case 'D':
				device_nr = atoi(optarg);
				break;

			case 'P':
				platform_nr = atoi(optarg);
				break;

			case 'v':
				verbose = true;
				break;
/*
			case 'p':
				pause = true;
				break;
*/
			case 'l':
				simulation_loops = atoi(optarg);
				break;

			case 'x':
				domain_cells[0] = atoi(optarg);
				break;

			case 'X':
				domain_cells[0] = atoi(optarg);
				domain_cells[1] = domain_cells[0];
				domain_cells[2] = domain_cells[0];
				break;

			case 'y':
				domain_cells[1] = atoi(optarg);
				break;

			case 'k':
				computation_kernel_count = atoi(optarg);
				break;

			case 'z':
				domain_cells[2] = atoi(optarg);
				break;

			case 'r':
				viscosity = atof(optarg);
				break;

			case 'u':
				domain_length_x = atof(optarg);
				break;

			case 'q':
				max_sim_gravitation_length = atof(optarg);
				break;

			case 'm':
				mass_exchange_factor = atof(optarg);
				break;

			case 'G':
				gravitation[1] = atof(optarg);
				break;

			case 't':
				timestep = atof(optarg);
				break;

			case 'g':
				load_gui = true;
				break;

			case 'i':
				init_flag = atoi(optarg);
				break;

			case 'n':
				load_lbm_simulation = true;
				break;

			case 'c':
				load_opencl = true;
				break;

			default:
				goto parameter_error;
		}
	}

	goto parameter_error_ok;
parameter_error:
	std::cout << "usage: " << argv[0] << std::endl;
	std::cout << "		[-x resolution_x, default: 32]" << std::endl;
	std::cout << "		[-y resolution_y, default: 32]" << std::endl;
	std::cout << "		[-z resolution_z, default: 32]" << std::endl;
	std::cout << "		[-X resolution_ALL, default: 32]" << std::endl;
	std::cout << "		[-g gravitation in bottom direction, default: -9.81]" << std::endl;
	std::cout << "		[-t 'time step']	(default: -1 for automatic time step detection)" << std::endl;
	std::cout << std::endl;
	std::cout << "		[-r 'viscosity, default: 0.005']" << std::endl;
	std::cout << "		[-q 'maximum gravitation length to limit time step size, default: 0.0001']" << std::endl;
	std::cout << "		[-u 'length of domain in x direction, default: 1 meter']" << std::endl;
	std::cout << "		[-v]	(verbose mode on, be verbose)" << std::endl;
	std::cout << "		[-k 'number of kernels to run' ]	(default: 0 - auto detect maximum)" << std::endl;
	std::cout << "		[-p]	(pause simulation at start, default:disabled)" << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << "		[-P platform_num]	(-1: list available platforms, or select platform)" << std::endl;
	std::cout << "		[-D device_num]	(-1: list available devices, or select device)" << std::endl;
	std::cout << std::endl;
	std::cout << "		[-l simulation_loops]	(value: number of simulation loops)" << std::endl;
	std::cout << "		[-d display] (default: DISPLAY env. variable)" << std::endl;
	std::cout << "		[-m mass_exchange] (multiplier for mass exchange, default:40)" << std::endl;
	std::cout << "		[-a lbm_implementation_nr] (select implementation number:)" << std::endl;
	std::cout << "		                           (0: A-A pattern)" << std::endl;
	std::cout << "		                           (1: A-B pattern ver. 1)" << std::endl;
	std::cout << "		                           (2: A-B pattern ver. 2)" << std::endl;
	std::cout << "		                           (3: A-B pattern ver. 1 and shared memory utilization)" << std::endl;
	std::cout << std::endl;
	std::cout << "		[-R registers for OpenCL]	(comma separated list of number of registers per threads in OpenCL)" << std::endl;
	std::cout << "		[-T work group threads]		(comma separated list of number of threads in OpenCL)" << std::endl;
	std::cout << std::endl;
	std::cout << "		[-c]	(enable loading OpenCL, default:disabled)" << std::endl;
	std::cout << "		[-n]	(enable LBM simulation, default:disabled)" << std::endl;
	std::cout << "		[-g]	(enable GUI, default:disabled)" << std::endl;
	std::cout << "		[-i initflag]	(iniit flag: default:0)" << std::endl;
	std::cout << std::endl;
	std::cout << "		[-b balance_board_mad, default: 0 for auto detection]" << std::endl;
	return -1;

parameter_error_ok:

	/***************
	 * BALANCE BOARD
	 ***************/

	CWiiBalanceBoard cWiiBalanceBoard(verbose);

	if (balance_board_addr != NULL)
	{

		if (strncmp(balance_board_addr, "0", 2) == 0)
		{
			balance_board_addr = NULL;
			if (verbose)	std::cout << "Balance board (search mode, press red sync button at battery storage) ..." << std::flush;
		}
		else
		{
			if (verbose)	std::cout << "Balance board (" << balance_board_addr << ") ..." << std::flush;
		}

		cWiiBalanceBoard.connect(balance_board_addr);
		if (cWiiBalanceBoard.error())
		{
			std::cerr << cWiiBalanceBoard.error.getString();
			return -1;
		}

		if (verbose)	std::cout << " [OK]" << std::endl;
#if 0
		while(1)
		{
			if (cWiiBalanceBoard.updateVectorAndWeight())
			{
				std::cout << cWiiBalanceBoard.weight << "        (" << cWiiBalanceBoard.vector_x << ", " << cWiiBalanceBoard.vector_y << ")" << std::endl;

				std::cout << cWiiBalanceBoard.left_bottom << " " << cWiiBalanceBoard.left_top << " " << cWiiBalanceBoard.right_bottom << " " << cWiiBalanceBoard.right_top << std::endl;
			}
		}
#endif
	}

	/***************
	 * GUI
	 ***************/
	CMainVisualization<T> *cMainVisualization = NULL;
	if (load_gui)
	{
		std::cout << "Loading Gui" << std::endl;
		cMainVisualization = new CMainVisualization<T>(cWiiBalanceBoard, verbose);

		if (cMainVisualization->error())
		{
			std::cerr << "Error: " << cMainVisualization->error.getString();
			return -1;
		}
	}

	/**************************
	 * OPENCL
	 **************************/
	std::list<int> lbm_opencl_number_of_registers_list;
	std::list<int> lbm_opencl_number_of_threads_list;

	CCLSkeleton *cCLSkeleton = NULL;
	if (load_opencl)
	{
		std::cout << "Loading OpenCL" << std::endl;
		cCLSkeleton = new CCLSkeleton(verbose);

#ifdef GL_INTEROP
		if (load_gui)
		{
			cCLSkeleton->initCLFromGLContext(device_nr, platform_nr);
		}
		else
		{
			cCLSkeleton->initCL(device_nr, platform_nr);
		}
#else
		cCLSkeleton->initCL(device_nr, platform_nr);
#endif
		if (cCLSkeleton->error())
		{
			std::cerr << "Error: " << cCLSkeleton->error.getString();
			return -1;
		}
	}

	/**************************
	 * LBM SIMULATION
	 **************************/
	CLbmOpenClInterface<T> *cLbmOpenCl = NULL;

	if (load_lbm_simulation && load_opencl)
	{
		std::cout << "Loading LBM OpenCL" << std::endl;

		if (!number_of_threads_string.empty())
			extract_comma_separated_integers(lbm_opencl_number_of_threads_list, number_of_threads_string);

		if (!number_of_registers_string.empty())
			extract_comma_separated_integers(lbm_opencl_number_of_registers_list, number_of_registers_string);

		switch(lbm_implementation_nr)
		{
			case 1:
				// standard lbm layout version 1
				cLbmOpenCl = new CLbmOpenClAB_1<T>(*cCLSkeleton, verbose);
				break;

			case 2:
				// standard lbm layout version 2
				cLbmOpenCl = new CLbmOpenClAB_2<T>(*cCLSkeleton, verbose);
				break;

			case 3:
				// standard lbm layout version 1 with shared memory utilization
				cLbmOpenCl = new CLbmOpenClAB_1_shared_memory<T>(*cCLSkeleton, verbose);
				break;

			default:
				// alpha-beta kernel
				cLbmOpenCl = new CLbmOpenClAA<T>(*cCLSkeleton, verbose);
		}

		if (init_flag < 0)
		{
			init_flag = 0;

			init_flag |= CLbmOpenClInterface<T>::INIT_CREATE_BREAKING_DAM;
		}

		if (verbose)
			std::cout << "init flag: " << init_flag << std::endl;

		if (computation_kernel_count == 0)
		{
			int default_work_group_size;
			cCLSkeleton->cDevice.getInfo(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, &default_work_group_size);

			computation_kernel_count = default_work_group_size;
		}

		cLbmOpenCl->init(
						domain_cells,
						domain_length_x,
						viscosity,
						gravitation,
						max_sim_gravitation_length,
						timestep,
						mass_exchange_factor,

						computation_kernel_count,

						init_flag,

						lbm_opencl_number_of_threads_list,
						lbm_opencl_number_of_registers_list
					);

		if (cLbmOpenCl->error())
		{
			std::cerr << "Error: " << cLbmOpenCl->error.getString();
			return -1;
		}
	}

	if (load_gui)
	{
		std::cout << "Initializing graphics" << std::endl;
		cMainVisualization->init(cLbmOpenCl, cCLSkeleton, domain_cells, gravitation);

		if (cMainVisualization->error())
		{
			std::cerr << "Main Viz Error: " << cMainVisualization->error.getString();
			return -1;
		}

		std::cout << "Running render loop" << std::endl;
		cMainVisualization->renderLoop();

		if (cMainVisualization->error())
		{
			std::cerr << "Main Viz Error: [" << cMainVisualization->error.getString() << "]";
			delete cMainVisualization;
			return -1;
		}
	}
	else
	{
		if (load_lbm_simulation && load_opencl)
		{
			CStopwatch cStopwatch;
			cStopwatch.start();

			for (int i = 0; i < simulation_loops; i++)
			{
				if ((i&15) == 0)
					std::cout << "." << std::flush;
				cLbmOpenCl->simulationStep();

				/**
				 * bunch of validation checks
				 *
				 * COMMENT OUT if not necessary!!!
				 */
#if 0
				static bool validation_output = false;
//				if (!validation_output)
				{
					 validation_output = cLbmOpenCl->validationChecks();
				}
#endif
			}
			cLbmOpenCl->wait();
			std::cout << std::endl;

			cStopwatch.stop();
			double fps = (double)simulation_loops/cStopwatch();
			std::cout << "FPS: " << fps << std::endl;
			std::cout << "MLUPS: " << fps*((double)domain_cells.elements()*(double)0.000001) << std::endl;

			if (verbose)
			{
				std::cout << "velocity checksum: " << cLbmOpenCl->getVelocityChecksum() << std::endl;
				std::cout << "mass checksum: " << cLbmOpenCl->getMassReduction() << std::endl;
				std::cout << "density checksum: " << cLbmOpenCl->getDensityChecksum() << std::endl;
			}

		}
	}


	if (cLbmOpenCl != NULL)
	{
		std::cout << "Cleaning up CLbmOpenCl..." << std::endl;
		delete cLbmOpenCl;
	}

	if (cCLSkeleton != NULL)
	{
		if (cMainVisualization != NULL)
		{
			std::cout << "Cleaning up Open CL GUI parts..." << std::endl;
			cMainVisualization->shutdownOpenCLParts();
		}

		std::cout << "Cleaning up CCLSkeleton..." << std::endl;
		delete cCLSkeleton;
	}

	if (cMainVisualization != NULL)
	{
		std::cout << "Cleaning up GUI..." << std::endl;
		delete cMainVisualization;
	}

	std::cout << "EXIT" << std::endl;
	return 0;
}

#if !WIN32
void print_mallinfo()
{
	struct mallinfo mallinfo_s = mallinfo();

	// http://www.gnu.org/s/libc/manual/html_node/Statistics-of-Malloc.html
	std::cout << "---------- MALLINFO ----------" << std::endl;
	std::cout << "arena: " << mallinfo_s.arena << std::endl;
	std::cout << "ordblks: " << mallinfo_s.ordblks << std::endl;
	std::cout << "smblks: " << mallinfo_s.smblks << std::endl;
	std::cout << "hblks: " << mallinfo_s.hblks << std::endl;
	std::cout << "hblkhd: " << mallinfo_s.hblkhd << std::endl;
	std::cout << "usmblks: " << mallinfo_s.usmblks << std::endl;
	std::cout << "fsmblks: " << mallinfo_s.fsmblks << std::endl;
	std::cout << "uordblks: " << mallinfo_s.uordblks << std::endl;
	std::cout << "fordblks: " << mallinfo_s.fordblks << std::endl;
	std::cout << "keepcost: " << mallinfo_s.keepcost << std::endl;
	std::cout << "---------- MALLINFO ----------" << std::endl;
}
#endif

#include "lib/CObjFile.hpp"

int main(int argc, char *argv[])
{
#if 0
	print_mallinfo();
	int retval = run(argc, argv);
	print_mallinfo();

	return retval;
#else
	return run(argc, argv);
#endif
}
