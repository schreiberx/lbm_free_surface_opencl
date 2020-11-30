#include "data/cl_programs/lbm_inc_header.h"


__kernel void kernel_lbm_mass_scale(
		__global T fluid_mass_array[DOMAIN_CELLS],		// 6) fluid mass
		T mass_scale_factor
)
{
	const size_t gid = get_global_id(0);

	fluid_mass_array[gid] *= mass_scale_factor;
}
