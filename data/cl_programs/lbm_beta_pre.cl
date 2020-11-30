#include "data/cl_programs/lbm_inc_header.h"


__kernel void kernel_beta_pre(
			__global T *global_dd,			// 0) density distributions
			__global int *flag_array,		// 1) flags
			__global T *velocity_array,		// 2) velocities
			__global T *density_array,		// 3) densities

			__global T *fluid_mass_array,	// 4) fluid mass
			__global T *fluid_fraction_array,	// 5) fluid fraction

			__const T mass_exchange_factor
		)
{

	const size_t gid = get_global_id(0);

	int flag = flag_array[gid];

	T velocity_x, velocity_y, velocity_z;
	T count;
	size_t dd_index;
	__global T *current_dds;
	T fluid_fraction;

	T ffx;
	T ddx;
	int neighbor_flag;

	if (flag & (FLAG_GAS | FLAG_OBSTACLE))
		return;

	/**
	 * MASS EXCHANGE (outgoing mass)
	 */
	// pointer to density distributions
	current_dds = &global_dd[gid];

	fluid_fraction = fluid_fraction_array[gid];

	// +++++++++++
	// +++ DD0 +++
	// +++++++++++
	//
	// 0-3: f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0)
	/*
	 * after the alpha step, the dd values are stored in cells of the opposite direction
	 * e.g. dd0 is stored at dd1 which simulates the streaming to the right side
	 * thus to compute the mass flow to the right cell, we load dd1 which is stored at
	 * dd0 and the cell flag of the right cell
	 */
	// dd0
	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	T fluid_mass = -ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	// +++++++++++
	// +++ DD1 +++
	// +++++++++++
	//

	// 4-7: f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0)
	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	// +++++++++++
	// +++ DD2 +++
	// +++++++++++
	//

	// 8-11: f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1)
	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	// +++++++++++
	// +++ DD3 +++
	// +++++++++++


	// dd3: f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1)
	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;


	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	// +++++++++++
	// +++ DD4 +++
	// +++++++++++
	//
	// dd4: f(0,0,1), f(0,0,-1),  f(0,0,0),  (not used)
	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	ddx = *current_dds;		current_dds += DOMAIN_CELLS;
	ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];
	neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];
	GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
	fluid_mass -= ddx*ffx;

	// compute new fluid mass
#if ACTIVATE_INCREASED_MASS_EXCHANGE
	fluid_mass_array[gid] = fluid_mass_array[gid] + mass_exchange_factor*fluid_mass;
#else
	fluid_mass_array[gid] = fluid_mass_array[gid] + fluid_mass;
#endif
}
