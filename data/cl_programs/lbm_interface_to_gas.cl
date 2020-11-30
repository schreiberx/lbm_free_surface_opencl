#include "data/cl_programs/lbm_inc_header.h"

__kernel void kernel_interface_to_gas(
			__global T *x_global_dd,		// 0) density distributions
			__global int *flag_array,		// 1) flags
			__global T *x_velocity_array,	// 2) velocities
			__global T *x_density_array,		// 3) densities

			__global T *fluid_mass_array,	// 4) fluid mass
			__global T *fluid_fraction_array,	// 5) fluid fraction

			__const T mass_exchange_factor	// 6) mass exchange factor
		)
{
#if !INTERFACE_CHANGE
	return;
#endif

	const size_t gid = get_global_id(0);

	// load cell type flag
	const int flag = flag_array[gid];

	if (flag == FLAG_INTERFACE)
	{
		// load fluid fraction
		T fluid_fraction = fluid_fraction_array[gid];

		if (fluid_fraction <= -EXTRA_OFFSET)
		{
			// switch to gas
			flag_array[gid] = FLAG_INTERFACE_TO_GAS;
			//fluid_fraction_array[gid] = -1024.0;
			fluid_fraction_array[gid] = 0.0;
		}
	}
}
