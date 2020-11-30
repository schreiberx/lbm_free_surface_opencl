/*
 * the pre kernel cares about the mass which is going out of the cell
 *
 * this kernel cannot be combined with the alpha collision kernel because there
 * exist race conditions for density distribution values
 */


// this option does not work anymore!
#define CACHED_ACCESS	0

#include "data/cl_programs/lbm_inc_header.h"


__kernel void kernel_lbm_alpha_pre(
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
	const size_t lid = get_local_id(0);

	int flag = flag_array[gid];

	__local T dd_buf[4][LOCAL_WORK_GROUP_SIZE];
	__local T* dd_buf_lid;

//	__local T buf[LOCAL_WORK_GROUP_SIZE];


// TODO: DO NOT UNCOMMENT THE FOLLOWING LINE!!!
// EVEN GAS THREADS HAVE TO LOAD DATA FOR ADJACENT THREADS
//	if (flag & (FLAG_INTERFACE | FLAG_FLUID))
	{
#if (LOCAL_WORK_GROUP_SIZE/DOMAIN_CELLS_X)*DOMAIN_CELLS_X == LOCAL_WORK_GROUP_SIZE
		/*
		 * handle domain x-sizes specially if LOCAL_WORK_GROUP_SIZE is a multiple of DOMAIN_CELLS_X
		 * in this case, we dont have to read any unaligned data!!!
		 */
		int pos_x_wrap = LOCAL_WORK_GROUP_WRAP(lid + 1);
		int neg_x_wrap = LOCAL_WORK_GROUP_WRAP(lid + (LOCAL_WORK_GROUP_SIZE - 1));

		// index to density distribution array in global memory to read from
		int read_delta_neg_x = gid;
		int read_delta_pos_x = gid;
#else
		int pos_x_wrap = LOCAL_WORK_GROUP_WRAP(lid + 1);
		int neg_x_wrap = LOCAL_WORK_GROUP_WRAP(lid + (LOCAL_WORK_GROUP_SIZE - 1));

		// index to density distribution array in global memory to read from
		int read_delta_neg_x = DOMAIN_WRAP((int)gid - (int)lid + pos_x_wrap + DELTA_NEG_X);
		int read_delta_pos_x = DOMAIN_WRAP((int)gid - (int)lid + neg_x_wrap + DELTA_POS_X);
#endif

		T velocity_x, velocity_y, velocity_z;
		T count;
		size_t dd_index;

		T ffx;
		T ddx;
		int neighbor_flag;

		T fluid_fraction = fluid_fraction_array[gid];

		/*
		 * +++++++++++
		 * +++ DD0 +++
		 * +++++++++++
		 *
		 * dd0: f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0)
		 * negative displacement
		 * preload to alignment buffer
		 */

		__global T *current_dds = global_dd;
		dd_buf_lid = &dd_buf[0][lid];

		/*
		 * pointer to current dd buf entry with index lid
		 */

		*dd_buf_lid = current_dds[dd_write_delta_position_0];		current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
		*dd_buf_lid = current_dds[dd_write_delta_position_1];	current_dds += DOMAIN_CELLS;
		barrier(CLK_LOCAL_MEM_FENCE);

		ddx = dd_buf[0][pos_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		T fluid_mass = -ddx*ffx;

		ddx = dd_buf[1][neg_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = current_dds[dd_write_delta_position_2];		current_dds += DOMAIN_CELLS;
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = current_dds[dd_write_delta_position_3];		current_dds += DOMAIN_CELLS;
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;


		/* +++++++++++
		 * +++ DD1 +++
		 * +++++++++++
		 *
		 * ddx: f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0)
		 */
		dd_buf_lid = &dd_buf[0][lid];

		*dd_buf_lid = current_dds[dd_write_delta_position_4];	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
		*dd_buf_lid = current_dds[dd_write_delta_position_5];	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
		*dd_buf_lid = current_dds[dd_write_delta_position_6];	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
		*dd_buf_lid = current_dds[dd_write_delta_position_7];	current_dds += DOMAIN_CELLS;
		barrier(CLK_LOCAL_MEM_FENCE);

		ddx = dd_buf[0][pos_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = dd_buf[1][neg_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = dd_buf[2][pos_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = dd_buf[3][neg_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;


		/* +++++++++++
		 * +++ DD2 +++
		 * +++++++++++
		 *
		 * ddx: f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1)
		 */
		dd_buf_lid = &dd_buf[0][lid];

		*dd_buf_lid = current_dds[dd_write_delta_position_8];	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
		*dd_buf_lid = current_dds[dd_write_delta_position_9];	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
		*dd_buf_lid = current_dds[dd_write_delta_position_10];	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
		*dd_buf_lid = current_dds[dd_write_delta_position_11];	current_dds += DOMAIN_CELLS;
		barrier(CLK_LOCAL_MEM_FENCE);

		ddx = dd_buf[0][pos_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = dd_buf[1][neg_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = dd_buf[2][pos_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = dd_buf[3][neg_x_wrap];
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;


		/*
		 * +++++++++++
		 * +++ DD3 +++
		 * +++++++++++
		 *
		 * dd3: f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1)
		 */

		ddx = current_dds[dd_write_delta_position_12];	current_dds += DOMAIN_CELLS;
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = current_dds[dd_write_delta_position_13];	current_dds += DOMAIN_CELLS;
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = current_dds[dd_write_delta_position_14];	current_dds += DOMAIN_CELLS;
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = current_dds[dd_write_delta_position_15];	current_dds += DOMAIN_CELLS;
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;


		/*
		 * +++++++++++
		 * +++ DD4 +++
		 * +++++++++++
		 *
		 * dd4: f(0,0,1), f(0,0,-1),  f(0,0,0),  (not used)
		 */
		ddx = current_dds[dd_write_delta_position_16];	current_dds += DOMAIN_CELLS;
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		ddx = current_dds[dd_write_delta_position_17];	current_dds += DOMAIN_CELLS;
		ffx = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];
		neighbor_flag = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];
		GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ffx, neighbor_flag);
		fluid_mass -= ddx*ffx;

		// compute new fluid mass
#if ACTIVATE_INCREASED_MASS_EXCHANGE
		fluid_mass_array[gid] = fluid_mass_array[gid] + mass_exchange_factor*fluid_mass;
#else
		fluid_mass_array[gid] = fluid_mass_array[gid] + fluid_mass;
#endif

	}
}
