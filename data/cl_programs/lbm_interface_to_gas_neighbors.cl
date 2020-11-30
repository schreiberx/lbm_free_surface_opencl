#include "data/cl_programs/lbm_inc_header.h"

/*
 * this kernel handles interface cells which are converted to gas cells
 *
 * if a neighbor cell is a fluid cell, it's converted to an interface cell to avoid interface gaps
 *
 * if DISTRIBUTE_MASS_OF_GAS_CELLS is enabled, the number of fluid and interface cells is counted
 * and the mass is divided by that value. this enables the neighbored cells to read out and add the
 * distributed mass directly to it's own mass
 */
__kernel void kernel_interface_to_gas_neighbors(
			__global T *x_global_dd,			// 0) density distributions
			__global int *flag_array,			// 1) flags
			__global T *x_velocity_array,		// 2) velocities
			__global T *x_density_array,			// 3) densities

			__global T *fluid_mass_array,		// 4) fluid mass
			__global T *x_fluid_fraction_array,	// 5) fluid fraction
			__global T *distribute_fluid_mass	// 6) fluid mass to be distributed
)
{
	const size_t gid = get_global_id(0);

	int flag = flag_array[gid];
	int neighbor_flag;

	size_t dd_index;

#if DISTRIBUTE_MASS_OF_GAS_CELLS
	float neighbored_fluid_interface_cells = 0.0;
#endif

	if (flag == FLAG_INTERFACE_TO_GAS)
	{
#if DISTRIBUTE_MASS_OF_GAS_CELLS
	#define STD_STUFF												\
		neighbor_flag = flag_array[dd_index];						\
		if (neighbor_flag == FLAG_FLUID)							\
			flag_array[dd_index] = FLAG_INTERFACE;					\
		if (neighbor_flag & (FLAG_FLUID | FLAG_INTERFACE))			\
			neighbored_fluid_interface_cells += 1.0;
#else
	#define STD_STUFF										\
		if (flag_array[dd_index] == FLAG_FLUID)				\
			flag_array[dd_index] = FLAG_INTERFACE;
#endif
		// we leave the fluid mass and fluid fraction unchanged cz. those should be already around 1.0

		// check neighbored cells if they are fluid cells and convert them to interface cells
		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X);			STD_STUFF
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
	#undef STD_STUFF

		// convert to gas cell
		flag_array[gid] = FLAG_GAS;

#if DISTRIBUTE_MASS_OF_GAS_CELLS
		if (neighbored_fluid_interface_cells != 0)
			neighbored_fluid_interface_cells = fluid_mass_array[gid]/neighbored_fluid_interface_cells;
#endif
	}

#if DISTRIBUTE_MASS_OF_GAS_CELLS
	/*
	 * amount of mass which has to be spread to each cell
	 * if neighbored_fluid_interface_cells == 0, there's usually an error because
	 * the mass cannot be distributed to neighbored cells, but we ignore this silently
	 */
	distribute_fluid_mass[gid] = neighbored_fluid_interface_cells;
#endif

#include "lbm_inc_loneley_gas_interface_remover.h"
}
