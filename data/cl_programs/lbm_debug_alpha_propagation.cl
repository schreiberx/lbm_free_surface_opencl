/**
 * this kernel is just for debugging purposes
 *
 * it replaces the kernels for the alpha timestep and prepares
 * the output data of the beta kernel to be used as
 * input data for the beta kernel
 */


#define CACHED_ACCESS 0
#include "data/cl_programs/lbm_inc_header.h"

__kernel void kernel_debug_alpha_propagation(
			__global T global_dd[19*DOMAIN_CELLS],
			__global T new_global_dd[19*DOMAIN_CELLS]
		)
{
	const size_t gid = get_global_id(0);
	const size_t lid = get_local_id(0);

	__local T dd_buf[12][LOCAL_WORK_GROUP_SIZE];

	// density distributions
	T dd0, dd1, dd2, dd3, dd4, dd5, dd6, dd7, dd8, dd9, dd10, dd11, dd12, dd13, dd14, dd15, dd16, dd17, dd18;

	int pos_x_wrap = LOCAL_WORK_GROUP_WRAP(lid + 1);
	int neg_x_wrap = LOCAL_WORK_GROUP_WRAP(lid + (LOCAL_WORK_GROUP_SIZE - 1));

	/*
	 * +++++++++++
	 * +++ DD0 +++
	 * +++++++++++
	 *
	 * dd0: f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0)
	 * negative displacement
	 * preload to alignment buffer
	 */

	/*
	 * pointer to current dd buf entry with index lid
	 */
	__local T *dd_buf_lid = &dd_buf[1][lid];

	/*
	 * pointer to density distributions
	 */
	__global T *current_dds = &global_dd[0];

	// DD0 STUFF
	dd1 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X)];	current_dds += DOMAIN_CELLS;
	dd0 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X)];	current_dds += DOMAIN_CELLS;
	dd3 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y)];	current_dds += DOMAIN_CELLS;
	dd2 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y)];	current_dds += DOMAIN_CELLS;

	/* +++++++++++
	 * +++ DD1 +++
	 * +++++++++++
	 *
	 * dd1: f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0)
	 */

	dd5 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];	current_dds += DOMAIN_CELLS;
	dd4 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];	current_dds += DOMAIN_CELLS;
	dd7 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];	current_dds += DOMAIN_CELLS;
	dd6 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];	current_dds += DOMAIN_CELLS;

	/* +++++++++++
	 * +++ DD2 +++
	 * +++++++++++
	 *
	 * dd2: f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1)
	 */

	dd9 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];	current_dds += DOMAIN_CELLS;
	dd8 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];	current_dds += DOMAIN_CELLS;
	dd11 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];	current_dds += DOMAIN_CELLS;
	dd10 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];	current_dds += DOMAIN_CELLS;

	// +++++++++++
	// +++ DD3 +++
	// +++++++++++

	// dd3: f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1)

	dd13 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];	current_dds += DOMAIN_CELLS;
	dd12 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];	current_dds += DOMAIN_CELLS;
	dd15 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];	current_dds += DOMAIN_CELLS;
	dd14 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];	current_dds += DOMAIN_CELLS;

	/*
	 * +++++++++++
	 * +++ DD4 +++
	 * +++++++++++
	 *
	 * dd4: f(0,0,1), f(0,0,-1),  f(0,0,0),  (not used)
	 */

	dd17 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_Z)];	current_dds += DOMAIN_CELLS;
	dd16 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Z)];	current_dds += DOMAIN_CELLS;

	dd18 = current_dds[gid];

////////////////////////////////////////////////////////

	current_dds = &new_global_dd[gid];

	*current_dds = dd0;		current_dds += DOMAIN_CELLS;
	*current_dds = dd1;		current_dds += DOMAIN_CELLS;
	*current_dds = dd2;		current_dds += DOMAIN_CELLS;
	*current_dds = dd3;		current_dds += DOMAIN_CELLS;

	*current_dds = dd4;		current_dds += DOMAIN_CELLS;
	*current_dds = dd5;		current_dds += DOMAIN_CELLS;
	*current_dds = dd6;		current_dds += DOMAIN_CELLS;
	*current_dds = dd7;		current_dds += DOMAIN_CELLS;

	*current_dds = dd8;		current_dds += DOMAIN_CELLS;
	*current_dds = dd9;		current_dds += DOMAIN_CELLS;
	*current_dds = dd10;		current_dds += DOMAIN_CELLS;
	*current_dds = dd11;		current_dds += DOMAIN_CELLS;

	*current_dds = dd12;		current_dds += DOMAIN_CELLS;
	*current_dds = dd13;		current_dds += DOMAIN_CELLS;
	*current_dds = dd14;		current_dds += DOMAIN_CELLS;
	*current_dds = dd15;		current_dds += DOMAIN_CELLS;

	*current_dds = dd16;		current_dds += DOMAIN_CELLS;
	*current_dds = dd17;		current_dds += DOMAIN_CELLS;
	*current_dds = dd18;
}
