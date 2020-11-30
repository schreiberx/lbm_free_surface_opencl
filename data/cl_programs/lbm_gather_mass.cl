#include "data/cl_programs/lbm_inc_header.h"

__kernel void kernel_gather_mass(
			__global int fluid_flag_array[DOMAIN_CELLS],
			__global T fluid_mass_array[DOMAIN_CELLS],
		    __global T gather_mass_array[DOMAIN_CELLS]
)
{
#if !DISTRIBUTE_MASS_OF_GAS_CELLS
	return;
#endif

	const size_t gid = get_global_id(0);

	// only gather mass for cells which are fluid or interface cells
	if (!(fluid_flag_array[gid] & (FLAG_INTERFACE | FLAG_FLUID)))
		return;

	T gathered_mass = 0.0;

	size_t dd_index;

#define STD_STUFF(i)	gathered_mass += gather_mass_array[i];

	// check neighbored cells if they are fluid cells and convert them to interface cells
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_NEG_X));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_POS_X));

	STD_STUFF(DOMAIN_WRAP(gid + DELTA_NEG_Y));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_POS_Y));

	STD_STUFF(DOMAIN_WRAP(gid + DELTA_NEG_Z));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_POS_Z));

	STD_STUFF(DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y));

	STD_STUFF(DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z));

	STD_STUFF(DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z));
	STD_STUFF(DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z));
#undef STD_STUFF

	if (gathered_mass != 0.0)
		fluid_mass_array[gid] += gathered_mass;//*1000000000000000.0;
}
