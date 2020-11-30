#include "data/cl_programs/lbm_inc_header.h"

__kernel void kernel_interface_to_fluid_neighbors(
			__global T *x_global_dd,			// 0) density distributions
			__global int *flag_array,			// 1) flags
			__global T *x_velocity_array,		// 2) velocities
			__global T *x_density_array,		// 3) densities

			__global T *x_fluid_mass_array,		// 4) fluid mass
			__global T *x_fluid_fraction_array,	// 5) fluid fraction

			__const T mass_exchange_factor		// 6) mass exchange factor
		)
{
	const size_t gid = get_global_id(0);

	int flag = flag_array[gid];
	size_t dd_index;

	if (flag == FLAG_INTERFACE_TO_FLUID)
	{
#define STD_STUFF	if (flag_array[dd_index] == FLAG_GAS)	flag_array[dd_index] = FLAG_GAS_TO_INTERFACE;

		// check neighbored cells if they are fluid cells and convert them to interface cells
		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X);			STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X);			STD_STUFF;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_Y);			STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_Y);			STD_STUFF;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_Z);			STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_Z);			STD_STUFF;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y);	STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y);	STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y);	STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y);	STD_STUFF;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z);	STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z);	STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z);	STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z);	STD_STUFF;

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z);	STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z);	STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z);	STD_STUFF;
		dd_index = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z);	STD_STUFF;

		// convert to fluid cell
		flag_array[gid] = FLAG_FLUID;
	}
#include "lbm_inc_loneley_fluid_interface_remover.h"
}
