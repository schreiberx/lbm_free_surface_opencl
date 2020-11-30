/**
 * AB kernel implementation (version 1) with utilization of shared memory!
 */

#include "data/cl_programs/lbm_inc_header.h"

/**
 * this kernel is the faster version of the AB collision and propagation kernel
 *
 * the first operation is loading the density distribution values from adjacent cells (PROPAGATION)
 *
 * then the COLLISION is computed and the dds are stored to the local cells storage
 */
__kernel void kernel_lbm_coll_prop(
		__global T *global_dd,				// 0) density distributions
		__global int *flag_array,			// 1) flags
		__global T *velocity_array,			// 2) velocities
		__global T *density_array,			// 3) densities
		__const T inv_tau,					// 4) "weight" for collision function
		__const T inv_trt_tau,				// 5) "weight" for TRT collision model

		__global T *fluid_mass_array,		// 6) fluid mass
		__global T *fluid_fraction_array,	// 7) fluid fraction

		__global int *new_flag_array,		// 8) NEW flags
		__global T *new_fluid_fraction_array,	// 9) NEW fluid fraction

		__const T gravitation0,				// 10) gravitation
		__const T gravitation1,
		__const T gravitation2,

		__const T mass_exchange_factor,		// 13) mass exchange factor

		__global T *out_global_dd			// 14) density distributions
)
{
	const size_t gid = get_global_id(0);
	const size_t lid = get_local_id(0);

	// load cell type flag
	int flag = flag_array[gid];

	// load fluid fraction
	T fluid_fraction = fluid_fraction_array[gid];
/*
	if (flag == FLAG_GAS)
	{
		// do nothing if cell is of type gas
		// just copy data
		new_flag_array[gid] = FLAG_GAS;
		new_fluid_fraction_array[gid] = 0.0f;
		return;
	}
*/

	__local T local_buf_float[4][LOCAL_WORK_GROUP_SIZE];
	__local T local_buf_int[2][LOCAL_WORK_GROUP_SIZE];

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


	// density distributions
	T dd0, dd1, dd2, dd3, dd4, dd5, dd6, dd7, dd8, dd9, dd10, dd11, dd12, dd13, dd14, dd15, dd16, dd17, dd18;

	// fluid fractions (already averaged with fluid fraction of current cell)
	T ff0, ff1;

	T out_mass0, out_mass1;		// outgoing mass
	int index0, index1;			// array indices
	int neighbor_flag0, neighbor_flag1;

	T tmp;

	// helper variables
	T vel2;			// vel*vel
	T vela2;
	T vela_velb;

	T dd_param;		// modified rho as temporary variable

	// old velocity of cell to reconstruct incoming density distributions
	T old_velocity_x, old_velocity_y, old_velocity_z;
#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
	T dd_rho;
#endif

	if (flag == FLAG_INTERFACE)
	{
		// read velocity to reconstruct density distributions
		old_velocity_x = velocity_array[gid];
		old_velocity_y = velocity_array[DOMAIN_CELLS+gid];
		old_velocity_z = velocity_array[2*DOMAIN_CELLS+gid];

		vel2 = old_velocity_x*old_velocity_x + old_velocity_y*old_velocity_y + old_velocity_z*old_velocity_z;
#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
		dd_rho = (T)GAS_PRESSURE;
		dd_param = (T)(3.0f/2.0f)*(vel2);
#else
		// use 1.0f as density for gas pressure
		dd_param = (T)GAS_PRESSURE - (T)(3.0f/2.0f)*(vel2);
#endif
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	/*
	 * pointer to density distributions
	 * we use a pointer instead of accessing the array directly
	 * - first this reduces the number of used registers (according to profiling information)
	 * - secondly the program runs faster
	 */
	__global T *current_dds = global_dd;

#define LOAD_DD_FF(dda, ffa, ddb, ffb)			\
	out_mass1 = current_dds[gid];				\
	dda = current_dds[index0];					\
	current_dds += DOMAIN_CELLS;				\
	ffa = fluid_fraction_array[index0];			\
	neighbor_flag0 = flag_array[index0];		\
												\
	out_mass0 = current_dds[gid];				\
	ddb = current_dds[index1];					\
	current_dds += DOMAIN_CELLS;				\
	ffb = fluid_fraction_array[index1];			\
	neighbor_flag1 = flag_array[index1];

	/*
	 * dd 0-3: f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0)
	 */
#if 0
	index0 = DOMAIN_WRAP(gid + DELTA_NEG_X);	// index for dd at adjacent cell (-1,0,0)
	index1 = DOMAIN_WRAP(gid + DELTA_POS_X);	// index for dd at adjacent cell (1,0,0)

	LOAD_DD_FF(dd0, ff0, dd1, ff1);
#else
	// READ FROM GLOBAL MEMORY TO LOCAL MEMORY
	// read outgoing density distribution
	out_mass1 = current_dds[gid];
	// read incoming distribution
	local_buf_float[0][lid] = current_dds[dd_write_delta_position_1];			current_dds += DOMAIN_CELLS;
	local_buf_float[1][lid] = fluid_fraction_array[dd_write_delta_position_1];	// read fluid fraction
	local_buf_int[0][lid] = flag_array[dd_write_delta_position_1];				// read flag

	out_mass0 = current_dds[gid];
	local_buf_float[2][lid] = current_dds[dd_write_delta_position_0];			current_dds += DOMAIN_CELLS;
	local_buf_float[3][lid] = fluid_fraction_array[dd_write_delta_position_0];
	local_buf_int[1][lid] = flag_array[dd_write_delta_position_0];

	// SYNC
	barrier(CLK_LOCAL_MEM_FENCE);

	// READ FROM LOCAL MEMORY TO REGISTERS
	dd0 = local_buf_float[0][neg_x_wrap];
	ff0 = local_buf_float[1][neg_x_wrap];
	neighbor_flag0 = local_buf_int[0][neg_x_wrap];

	dd1 = local_buf_float[2][pos_x_wrap];
	ff1 = local_buf_float[3][pos_x_wrap];
	neighbor_flag1 = local_buf_int[1][pos_x_wrap];
#endif

	reconstruct_dd_01(flag, fluid_fraction, dd0, neighbor_flag0, ff0, dd1, neighbor_flag1, ff1, old_velocity_x, dd_param, dd_rho);

	T fluid_mass = (dd0 - out_mass0)*ff0;
	T rho = dd0;
	T velocity_x = dd0;

	fluid_mass += (dd1 - out_mass1)*ff1;
	rho += dd1;
	velocity_x -= dd1;

	index0 = DOMAIN_WRAP(gid + DELTA_NEG_Y);
	index1 = DOMAIN_WRAP(gid + DELTA_POS_Y);

#if 1
	LOAD_DD_FF(dd2, ff0, dd3, ff1);
#else
	out_mass1 = current_dds[gid];
	dd2 = current_dds[index0];
	current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[index0];
	neighbor_flag0 = flag_array[index0];

	out_mass0 = current_dds[gid];
	current_dds[gid] = dd0 + dd1 + dd2 + dd3 + out_mass0 + out_mass0 + ff0 + neighbor_flag0;	return;
	dd3 = current_dds[index1];
	current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[index1];
	neighbor_flag1 = flag_array[index1];

#endif
	reconstruct_dd_01(flag, fluid_fraction, dd2, neighbor_flag0, ff0, dd3, neighbor_flag1, ff1, old_velocity_y, dd_param, dd_rho);

	fluid_mass += (dd2 - out_mass0)*ff0;
	rho += dd2;
	T velocity_y = dd2;

	fluid_mass += (dd3 - out_mass1)*ff1;
	rho += dd3;
	velocity_y -= dd3;

	/*
	 * dd 4-7: f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0)
	 */
#if 0
	index0 = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y);
	index1 = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y);

	LOAD_DD_FF(dd4, ff0, dd5, ff1);
#else
	out_mass1 = current_dds[gid];
	local_buf_float[0][lid] = current_dds[dd_write_delta_position_5];			current_dds += DOMAIN_CELLS;
	local_buf_float[1][lid] = fluid_fraction_array[dd_write_delta_position_5];
	local_buf_int[0][lid] = flag_array[dd_write_delta_position_5];

	out_mass0 = current_dds[gid];
	local_buf_float[2][lid] = current_dds[dd_write_delta_position_4];			current_dds += DOMAIN_CELLS;
	local_buf_float[3][lid] = fluid_fraction_array[dd_write_delta_position_4];
	local_buf_int[1][lid] = flag_array[dd_write_delta_position_4];

	// SYNC
	barrier(CLK_LOCAL_MEM_FENCE);

	// READ FROM LOCAL MEMORY TO REGISTERS
	dd4 = local_buf_float[0][neg_x_wrap];
	ff0 = local_buf_float[1][neg_x_wrap];
	neighbor_flag0 = local_buf_int[0][neg_x_wrap];

	dd5 = local_buf_float[2][pos_x_wrap];
	ff1 = local_buf_float[3][pos_x_wrap];
	neighbor_flag1 = local_buf_int[1][pos_x_wrap];
#endif

	tmp = old_velocity_x+old_velocity_y;
	reconstruct_dd_45(flag, fluid_fraction, dd4, neighbor_flag0, ff0, dd5, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += (dd4 - out_mass0)*ff0;
	rho += dd4;
	velocity_x += dd4;
	velocity_y += dd4;

	fluid_mass += (dd5 - out_mass1)*ff1;
	rho += dd5;
	velocity_x -= dd5;
	velocity_y -= dd5;

#if 0
	index0 = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y);
	index1 = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y);

	LOAD_DD_FF(dd6, ff0, dd7, ff1);
#else
	out_mass1 = current_dds[gid];
	local_buf_float[0][lid] = current_dds[dd_write_delta_position_7];			current_dds += DOMAIN_CELLS;
	local_buf_float[1][lid] = fluid_fraction_array[dd_write_delta_position_7];
	local_buf_int[0][lid] = flag_array[dd_write_delta_position_7];

	out_mass0 = current_dds[gid];
	local_buf_float[2][lid] = current_dds[dd_write_delta_position_6];			current_dds += DOMAIN_CELLS;
	local_buf_float[3][lid] = fluid_fraction_array[dd_write_delta_position_6];
	local_buf_int[1][lid] = flag_array[dd_write_delta_position_6];

	// SYNC
	barrier(CLK_LOCAL_MEM_FENCE);

	// READ FROM LOCAL MEMORY TO REGISTERS
	dd6 = local_buf_float[0][neg_x_wrap];
	ff0 = local_buf_float[1][neg_x_wrap];
	neighbor_flag0 = local_buf_int[0][neg_x_wrap];

	dd7 = local_buf_float[2][pos_x_wrap];
	ff1 = local_buf_float[3][pos_x_wrap];
	neighbor_flag1 = local_buf_int[1][pos_x_wrap];
#endif

	tmp = old_velocity_x-old_velocity_y;
	reconstruct_dd_45(flag, fluid_fraction, dd6, neighbor_flag0, ff0, dd7, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += (dd6 - out_mass0)*ff0;
	rho += dd6;
	velocity_x += dd6;
	velocity_y -= dd6;

	fluid_mass += (dd7 - out_mass1)*ff1;
	rho += dd7;
	velocity_x -= dd7;
	velocity_y += dd7;

	/*
	 * dd 8-11: f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1)
	 */
#if 0
	index0 = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z);
	index1 = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z);

	LOAD_DD_FF(dd8, ff0, dd9, ff1);
#else
	out_mass1 = current_dds[gid];
	local_buf_float[0][lid] = current_dds[dd_write_delta_position_9];			current_dds += DOMAIN_CELLS;
	local_buf_float[1][lid] = fluid_fraction_array[dd_write_delta_position_9];
	local_buf_int[0][lid] = flag_array[dd_write_delta_position_9];

	out_mass0 = current_dds[gid];
	local_buf_float[2][lid] = current_dds[dd_write_delta_position_8];			current_dds += DOMAIN_CELLS;
	local_buf_float[3][lid] = fluid_fraction_array[dd_write_delta_position_8];
	local_buf_int[1][lid] = flag_array[dd_write_delta_position_8];

	// SYNC
	barrier(CLK_LOCAL_MEM_FENCE);

	// READ FROM LOCAL MEMORY TO REGISTERS
	dd8 = local_buf_float[0][neg_x_wrap];
	ff0 = local_buf_float[1][neg_x_wrap];
	neighbor_flag0 = local_buf_int[0][neg_x_wrap];

	dd9 = local_buf_float[2][pos_x_wrap];
	ff1 = local_buf_float[3][pos_x_wrap];
	neighbor_flag1 = local_buf_int[1][pos_x_wrap];
#endif
	tmp = old_velocity_x+old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd8, neighbor_flag0, ff0, dd9, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += (dd8 - out_mass0)*ff0;
	rho += dd8;
	velocity_x += dd8;
	T velocity_z = dd8;

	fluid_mass += (dd9 - out_mass1)*ff1;
	rho += dd9;
	velocity_x -= dd9;
	velocity_z -= dd9;

#if 0
	index0 = DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z);
	index1 = DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z);

	LOAD_DD_FF(dd10, ff0, dd11, ff1);
#else
	out_mass1 = current_dds[gid];
	local_buf_float[0][lid] = current_dds[dd_write_delta_position_11];			current_dds += DOMAIN_CELLS;
	local_buf_float[1][lid] = fluid_fraction_array[dd_write_delta_position_11];
	local_buf_int[0][lid] = flag_array[dd_write_delta_position_11];

	out_mass0 = current_dds[gid];
	local_buf_float[2][lid] = current_dds[dd_write_delta_position_10];			current_dds += DOMAIN_CELLS;
	local_buf_float[3][lid] = fluid_fraction_array[dd_write_delta_position_10];
	local_buf_int[1][lid] = flag_array[dd_write_delta_position_10];

	// SYNC
	barrier(CLK_LOCAL_MEM_FENCE);


//	if (flag == FLAG_GAS)
//		return;

	// READ FROM LOCAL MEMORY TO REGISTERS
	dd10 = local_buf_float[0][neg_x_wrap];
	ff0 = local_buf_float[1][neg_x_wrap];
	neighbor_flag0 = local_buf_int[0][neg_x_wrap];

	dd11 = local_buf_float[2][pos_x_wrap];
	ff1 = local_buf_float[3][pos_x_wrap];
	neighbor_flag1 = local_buf_int[1][pos_x_wrap];
#endif

	tmp = old_velocity_x-old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd10, neighbor_flag0, ff0, dd11, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += (dd10 - out_mass0)*ff0;
	rho += dd10;
	velocity_x += dd10;
	velocity_z -= dd10;

	fluid_mass += (dd11 - out_mass1)*ff1;
	rho += dd11;
	velocity_x -= dd11;
	velocity_z += dd11;

	/*
	 * dd 12-15: f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1)
	 */
	index0 = DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z);
	index1 = DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z);

	LOAD_DD_FF(dd12, ff0, dd13, ff1);

	tmp = old_velocity_y+old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd12, neighbor_flag0, ff0, dd13, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += (dd12 - out_mass0)*ff0;
	rho += dd12;
	velocity_y += dd12;
	velocity_z += dd12;

	fluid_mass += (dd13 - out_mass1)*ff1;
	rho += dd13;
	velocity_y -= dd13;
	velocity_z -= dd13;


	index0 = DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z);
	index1 = DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z);

	LOAD_DD_FF(dd14, ff0, dd15, ff1);

	tmp = old_velocity_y-old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd14, neighbor_flag0, ff0, dd15, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += (dd14 - out_mass0)*ff0;
	rho += dd14;
	velocity_y += dd14;
	velocity_z -= dd14;

	fluid_mass += (dd15 - out_mass1)*ff1;
	rho += dd15;
	velocity_y -= dd15;
	velocity_z += dd15;

	/*
	 * dd 16-18: f(0,0,1), f(0,0,-1),  f(0,0,0),  (not used)
	 */
	index0 = DOMAIN_WRAP(gid + DELTA_NEG_Z);
	index1 = DOMAIN_WRAP(gid + DELTA_POS_Z);

	LOAD_DD_FF(dd16, ff0, dd17, ff1);

	reconstruct_dd_01(flag, fluid_fraction, dd16, neighbor_flag0, ff0, dd17, neighbor_flag1, ff1, old_velocity_z, dd_param, dd_rho);

	fluid_mass += (dd16 - out_mass0)*ff0;
	rho += dd16;
	velocity_z += dd16;

	fluid_mass += (dd17 - out_mass1)*ff1;
	rho += dd17;
	velocity_z -= dd17;

	dd18 = current_dds[gid];
	rho += dd18;


#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
	velocity_x /= rho;
	velocity_y /= rho;
	velocity_z /= rho;
#endif

	// compute new fluid mass
#if ACTIVATE_INCREASED_MASS_EXCHANGE
	fluid_mass = mass_exchange_factor*fluid_mass + fluid_mass_array[gid];
#else
	fluid_mass = fluid_mass + fluid_mass_array[gid];
#endif


	/**
	 * collision operator
	 */
	#include "data/cl_programs/lbm_inc_collision_operator.h"

	barrier(CLK_LOCAL_MEM_FENCE);

	current_dds = &out_global_dd[gid];
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
	*current_dds = dd10;	current_dds += DOMAIN_CELLS;
	*current_dds = dd11;	current_dds += DOMAIN_CELLS;

	*current_dds = dd12;	current_dds += DOMAIN_CELLS;
	*current_dds = dd13;	current_dds += DOMAIN_CELLS;
	*current_dds = dd14;	current_dds += DOMAIN_CELLS;
	*current_dds = dd15;	current_dds += DOMAIN_CELLS;

	*current_dds = dd16;	current_dds += DOMAIN_CELLS;
	*current_dds = dd17;	current_dds += DOMAIN_CELLS;
	*current_dds = dd18;



#if INTERFACE_CHANGE
	fluid_fraction = fluid_mass/rho;

	if (flag == FLAG_INTERFACE)
	{
		if (fluid_fraction >= 1.0f + EXTRA_OFFSET)
		{
			// switch to fluid
			flag = FLAG_INTERFACE_TO_FLUID;
			/*
			 * leave fluid mass unchanged because we have to distribute that values to the neighbors
			 */
		}
	}
	else if (flag == FLAG_FLUID)
	{
		fluid_mass = rho;
	}
	else	// gas or obstacle
	{
		fluid_mass = 0.0;
	}

	// limit fluid fraction for further computations
	fluid_fraction = max(min(fluid_fraction, (T)(1.0f+EXTRA_OFFSET)), (T)(0.0f-EXTRA_OFFSET));
#endif

	barrier(CLK_LOCAL_MEM_FENCE);

	/*
	 * store fluid mass
	 */
	fluid_mass_array[gid] = fluid_mass;

	/*
	 * store fluid fractions and flags to new arrays to avoid disturbance of mass conservation
	 * as well as race conditions!!!
	 */

	new_fluid_fraction_array[gid] = fluid_fraction;
	new_flag_array[gid] = flag;

	/*
	 * store velocity
	 */
	current_dds = &velocity_array[gid];
	*current_dds = velocity_x;	current_dds += DOMAIN_CELLS;
	*current_dds = velocity_y;	current_dds += DOMAIN_CELLS;
	*current_dds = velocity_z;

	/*
	 * store density
	 * this is useful for recomputation of dd's from gas cells streaming to interface cells
	 */
	density_array[gid] = rho;
}
