#define CACHED_ACCESS	0
//#define CACHED_ACCESS	0

#define USE_SHARED_MEMORY	0

#include "data/cl_programs/lbm_inc_header.h"


__kernel void kernel_beta(
		__global T *global_dd,			// 0) density distributions
		__global int *flag_array,		// 1) flags
		__global T *velocity_array,		// 2) velocities
		__global T *density_array,		// 3) densities
		__const T inv_tau,								// 4) "weight" for collision function
		__const T inv_trt_tau,							// 5) "weight" for TRT collision model

		__global T *fluid_mass_array,		// 6) fluid mass
		__global T *fluid_fraction_array,	// 7) fluid fraction

		__global int *new_flag_array,		// 8) NEW flags
		__global T *new_fluid_fraction_array,	// 9) NEW fluid fraction

		__const T gravitation0,							// 10) gravitation
		__const T gravitation1,
		__const T gravitation2,

		__const T mass_exchange_factor					// 13) mass exchange factor
)
{
	const size_t gid = get_global_id(0);
	const size_t lid = get_local_id(0);

	// load cell type flag
	int flag = flag_array[gid];

	// load fluid fraction
	T fluid_fraction = fluid_fraction_array[gid];

#if !USE_SHARED_MEMORY
//	we must not return for gas cells because those threads may load the data for another thread!!!
	if (flag == FLAG_GAS)
	{
		// do nothing if cell is of type gas
		new_flag_array[gid] = FLAG_GAS;
//		new_fluid_fraction_array[gid] = fluid_fraction;
		new_fluid_fraction_array[gid] = 0.0f;
		return;
	}
#endif

#if USE_SHARED_MEMORY
	__local T dd_buf[12][LOCAL_WORK_GROUP_SIZE];
#endif

	// density distributions
	T dd0, dd1, dd2, dd3, dd4, dd5, dd6, dd7, dd8, dd9, dd10, dd11, dd12, dd13, dd14, dd15, dd16, dd17, dd18;

	// fluid fractions (already averaged with fluid fraction of current cell)
	T ff0, ff1;

	T tmp;

	T vel2;		// vel*vel
	T vela2;
	T vela_velb;
	T vela_velb_2;

	T dd_param;	// modified rho as temporary variable
	T dd_rho;

	// velocity
	T old_velocity_x, old_velocity_y, old_velocity_z;

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
//		dd_param = (T)1.0f - (T)(3.0f/2.0f)*(vel2);
		dd_param = (T)GAS_PRESSURE - (T)(3.0f/2.0f)*(vel2);
#endif
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	int neighbor_flag0, neighbor_flag1;


#if !USE_SHARED_MEMORY

	/*
	 * pointer to density distributions
	 */
	__global T *current_dds = &global_dd[0];

	/*
	 * dd 0-3: f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0)
	 */
	dd1 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X)];	current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X)];

	dd0 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X)];	current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];

	reconstruct_dd_01(flag, fluid_fraction, dd0, neighbor_flag0, ff0, dd1, neighbor_flag1, ff1, old_velocity_x, dd_param, dd_rho);

	T fluid_mass = dd0*ff0;
	T rho = dd0;
	T velocity_x = dd0;

	fluid_mass += dd1*ff1;
	/*
	 * we have to sum the densities up in a specific order.
	 * otherwise it seems that we run into numerical errors.
	 */
	T rhob = dd1;
	velocity_x -= dd1;

	dd3 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y)];		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];

	dd2 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y)];		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];

	reconstruct_dd_01(flag, fluid_fraction, dd2, neighbor_flag0, ff0, dd3, neighbor_flag1, ff1, old_velocity_y, dd_param, dd_rho);

	fluid_mass += dd2*ff0;
	T rhoc = dd2;
	T velocity_y = dd2;

	fluid_mass += dd3*ff1;
	T rhod = dd3;
	velocity_y -= dd3;

	/*
	 * dd 4-7: f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0)
	 */
	barrier(CLK_LOCAL_MEM_FENCE);

	dd5 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];

	dd4 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];

	tmp = old_velocity_x+old_velocity_y;
	reconstruct_dd_45(flag, fluid_fraction, dd4, neighbor_flag0, ff0, dd5, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd4*ff0;
	rho += dd4;
	velocity_x += dd4;
	velocity_y += dd4;

	fluid_mass += dd5*ff1;
	rhob += dd5;
	velocity_x -= dd5;
	velocity_y -= dd5;

	dd7 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];

	dd6 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];

	tmp = old_velocity_x-old_velocity_y;
	reconstruct_dd_45(flag, fluid_fraction, dd6, neighbor_flag0, ff0, dd7, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd6*ff0;
	rhoc += dd6;
	velocity_x += dd6;
	velocity_y -= dd6;

	fluid_mass += dd7*ff1;
	rhod += dd7;
	velocity_x -= dd7;
	velocity_y += dd7;

	/*
	 * dd 8-11: f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1)
	 */
	dd9 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];

	dd8 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];

	tmp = old_velocity_x+old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd8, neighbor_flag0, ff0, dd9, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd8*ff0;
	rho += dd8;
	velocity_x += dd8;
	T velocity_z = dd8;

	fluid_mass += dd9*ff1;
	rhob += dd9;
	velocity_x -= dd9;
	velocity_z -= dd9;

	dd11 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];

	dd10 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];

	tmp = old_velocity_x-old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd10, neighbor_flag0, ff0, dd11, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd10*ff0;
	rhoc += dd10;
	velocity_x += dd10;
	velocity_z -= dd10;

	fluid_mass += dd11*ff1;
	rhod += dd11;
	velocity_x -= dd11;
	velocity_z += dd11;

	/*
	 * dd 12-15: f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1)
	 */
	dd13 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];	current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];

	dd12 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];	current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];

	tmp = old_velocity_y+old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd12, neighbor_flag0, ff0, dd13, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd12*ff0;
	rho += dd12;
	velocity_y += dd12;
	velocity_z += dd12;

	fluid_mass += dd13*ff1;
	rhob += dd13;
	velocity_y -= dd13;
	velocity_z -= dd13;

	dd15 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];	current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];

	dd14 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];	current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];

	tmp = old_velocity_y-old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd14, neighbor_flag0, ff0, dd15, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd14*ff0;
	rhoc += dd14;
	velocity_y += dd14;
	velocity_z -= dd14;

	fluid_mass += dd15*ff1;
	rhod += dd15;
	velocity_y -= dd15;
	velocity_z += dd15;

	/*
	 * dd 16-18: f(0,0,1), f(0,0,-1),  f(0,0,0),  (not used)
	 */
	dd17 = current_dds[DOMAIN_WRAP(gid + DELTA_POS_Z)];		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];

	dd16 = current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Z)];		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];

	reconstruct_dd_01(flag, fluid_fraction, dd16, neighbor_flag0, ff0, dd17, neighbor_flag1, ff1, old_velocity_z, dd_param, dd_rho);

	fluid_mass += dd16*ff0;
	rho += dd16;
	velocity_z += dd16;

	fluid_mass += dd17*ff1;
	rhob += dd17;
	velocity_z -= dd17;

	dd18 = current_dds[gid];
	rhoc += dd18;

#else

#if CACHED_ACCESS
/*
	size_t dd_read_delta_position_0;
	size_t dd_read_delta_position_1;
*/
	size_t dd_read_delta_position_2;
	size_t dd_read_delta_position_3;

	size_t dd_read_delta_position_4;
	size_t dd_read_delta_position_5;
	size_t dd_read_delta_position_6;
	size_t dd_read_delta_position_7;

	size_t dd_read_delta_position_8;
	size_t dd_read_delta_position_9;
	size_t dd_read_delta_position_10;
	size_t dd_read_delta_position_11;

	size_t dd_read_delta_position_12;
	size_t dd_read_delta_position_13;
	size_t dd_read_delta_position_14;
	size_t dd_read_delta_position_15;

	size_t dd_read_delta_position_16;
	size_t dd_read_delta_position_17;
#endif

	/*
	 * We have to handle "misaligned" data with a shift of -1 and +1:
	 *
	 * As an example, we handle the access to the density distributions with a shift of -1:
	 *
	 * We use "(( (lid + 1) mod LOCAL_WORK_GROUP_SIZE) + (DOMAIN_CELLS+1)) mod DOMAIN_CELLS" as the reading index
	 *          shift to right                              shift back
	 *
	 * This allows us to read almost everything (except the first thread) aligned. After that the data is stored
	 * to shared memory, a sync operation is called and finally the tread can read the originally required data which
	 * was previously read by another thread.
	 *
	 * Every float is stored to a local memory array indexed by the thread id.
	 * after the dd's are read from global memory, the local memory is accessed with
	 * "(lid + (LOCAL_WORK_GROUP_SIZE-1)) mod LOCAL_WORK_GROUP_SIZE"
	 */

	/*
	 * pos_x_wrap specifies the position in the local buffer for the dd with the displacement x=+1
	 * this is used to force the thread with the largest number to read the dd at the displacement -1 (see gid_pos below)
	 * pos_x_wrap and gid_pos have the following values for given local thread ids:
	 *
	 * thread_id:   0 1 2 3 4 5 ... 63
	 * pos_x_wrap:  1 2 3 4 5 6 ... 0
	 * gid_pos:     0 1 2 3 4 5 ... -1   <<< !!!
	 */

	int pos_x_wrap = LOCAL_WORK_GROUP_WRAP(lid + 1);
	int neg_x_wrap = LOCAL_WORK_GROUP_WRAP(lid + (LOCAL_WORK_GROUP_SIZE - 1));

#if (LOCAL_WORK_GROUP_SIZE/DOMAIN_CELLS_X)*DOMAIN_CELLS_X == LOCAL_WORK_GROUP_SIZE
/*
 * handle domain x-sizes specially if LOCAL_WORK_GROUP_SIZE is a multiple of DOMAIN_CELLS_X
 * in this case, we dont have to read unaligned data!!!
 */
	int read_delta_neg_x = gid;
	int read_delta_pos_x = gid;
#else
	/*
	 * cache variables for speedup
	 */
	int read_delta_neg_x = DOMAIN_WRAP(gid - lid + pos_x_wrap + DELTA_NEG_X);
	int read_delta_pos_x = DOMAIN_WRAP(gid - lid + neg_x_wrap + DELTA_POS_X);
#endif
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
	 * read negative distribution vector (-1,0,0) from relative cell (-1,0,0) and store it to positive distribution vector (1,0,0)
	 *
	 * in the alpha kernel, the density distribution values have been stored to the oppisite density distribution storage
	 * to avoid the propagation step
	 */

	/*
	 * pointer to current dd buf entry with index lid
	 */
	__local T *dd_buf_lid = &dd_buf[1][lid];

	/*
	 * pointer to density distributions
	 */
	__global T *current_dds = &(global_dd[0][0]);
	// DD0 STUFF
	*dd_buf_lid = current_dds[read_delta_pos_x];		current_dds += DOMAIN_CELLS;	dd_buf_lid -= LOCAL_WORK_GROUP_SIZE;
	*dd_buf_lid = current_dds[read_delta_neg_x];		current_dds += DOMAIN_CELLS;	dd_buf_lid += 5*LOCAL_WORK_GROUP_SIZE;
	barrier(CLK_LOCAL_MEM_FENCE);

	dd0 = dd_buf[0][neg_x_wrap];
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];
//	if (ff0 == -1024.0f)
//		neighbor_flag0 = FLAG_GAS;
//	else
		neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];

	dd1 = dd_buf[1][pos_x_wrap];
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X)];
//	if (ff1 == -1024.0f)
//		neighbor_flag1 = FLAG_GAS;
//	else
		neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X)];

	barrier(CLK_LOCAL_MEM_FENCE);
	reconstruct_dd_01(flag, fluid_fraction, dd0, neighbor_flag0, ff0, dd1, neighbor_flag1, ff1, old_velocity_x, dd_param, dd_rho);

	T fluid_mass = dd0*ff0;
	T rho = dd0;
	T velocity_x = dd0;

	fluid_mass += dd1*ff1;
	/*
	 * we have to sum the densities up in a specific order.
	 * otherwise it seems that we run into numerical errors.
	 */
	T rhob = dd1;
	velocity_x -= dd1;


#if CACHED_ACCESS
	dd_read_delta_position_3 = dd_read_delta_position_3x;
#endif
	dd3 = current_dds[dd_read_delta_position_3];		current_dds += DOMAIN_CELLS;

	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];
//	if (ff1 == -1024.0f)
//		neighbor_flag1 = FLAG_GAS;
//	else
		neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];

#if CACHED_ACCESS
	dd_read_delta_position_2 = dd_read_delta_position_2x;
#endif
	dd2 = current_dds[dd_read_delta_position_2];		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];
//	if (ff0 == -1024.0f)
//		neighbor_flag0 = FLAG_GAS;
//	else
		neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];

	reconstruct_dd_01(flag, fluid_fraction, dd2, neighbor_flag0, ff0, dd3, neighbor_flag1, ff1, old_velocity_y, dd_param, dd_rho);

	fluid_mass += dd2*ff0;
	T rhoc = dd2;
	T velocity_y = dd2;

	fluid_mass += dd3*ff1;
	T rhod = dd3;
	velocity_y -= dd3;

	/*
	 * dd 4-7: f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0)
	 */
#if CACHED_ACCESS
	dd_read_delta_position_5 = dd_read_delta_position_5x;
#endif
	*dd_buf_lid = current_dds[dd_read_delta_position_5];	current_dds += DOMAIN_CELLS;	dd_buf_lid -= LOCAL_WORK_GROUP_SIZE;

#if CACHED_ACCESS
	dd_read_delta_position_4 = dd_read_delta_position_4x;
#endif
	*dd_buf_lid = current_dds[dd_read_delta_position_4];	current_dds += DOMAIN_CELLS;	dd_buf_lid += 3*LOCAL_WORK_GROUP_SIZE;

#if CACHED_ACCESS
	dd_read_delta_position_7 = dd_read_delta_position_7x;
#endif
	*dd_buf_lid = current_dds[dd_read_delta_position_7];	current_dds += DOMAIN_CELLS;	dd_buf_lid -= LOCAL_WORK_GROUP_SIZE;

#if CACHED_ACCESS
	dd_read_delta_position_6 = dd_read_delta_position_6x;
#endif
	*dd_buf_lid = current_dds[dd_read_delta_position_6];	current_dds += DOMAIN_CELLS;	dd_buf_lid += 3*LOCAL_WORK_GROUP_SIZE;

	barrier(CLK_LOCAL_MEM_FENCE);

	dd4 = dd_buf[4][neg_x_wrap];
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];
//	if (ff0 == -1024.0f)
//		neighbor_flag0 = FLAG_GAS;
//	else
		neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];

	dd5 = dd_buf[5][pos_x_wrap];
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];
//	if (ff1 == -1024.0f)
//		neighbor_flag1 = FLAG_GAS;
//	else
		neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];

	tmp = old_velocity_x+old_velocity_y;
	reconstruct_dd_45(flag, fluid_fraction, dd4, neighbor_flag0, ff0, dd5, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd4*ff0;
	rho += dd4;
	velocity_x += dd4;
	velocity_y += dd4;

	fluid_mass += dd5*ff1;
	rhob += dd5;
	velocity_x -= dd5;
	velocity_y -= dd5;

	dd6 = dd_buf[6][neg_x_wrap];
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];
//	if (ff0 == -1024.0f)
//		neighbor_flag0 = FLAG_GAS;
//	else
		neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];

	dd7 = dd_buf[7][pos_x_wrap];
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];
//	if (ff1 == -1024.0f)
//		neighbor_flag1 = FLAG_GAS;
//	else
		neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];

	tmp = old_velocity_x-old_velocity_y;
	reconstruct_dd_45(flag, fluid_fraction, dd6, neighbor_flag0, ff0, dd7, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd6*ff0;
	rhoc += dd6;
	velocity_x += dd6;
	velocity_y -= dd6;

	fluid_mass += dd7*ff1;
	rhod += dd7;
	velocity_x -= dd7;
	velocity_y += dd7;

	/*
	 * dd 8-11: f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1)
	 */

#if CACHED_ACCESS
	dd_read_delta_position_9 = dd_read_delta_position_9x;
#endif
	*dd_buf_lid = current_dds[dd_read_delta_position_9];	current_dds += DOMAIN_CELLS;	dd_buf_lid -= LOCAL_WORK_GROUP_SIZE;

#if CACHED_ACCESS
	dd_read_delta_position_8 = dd_read_delta_position_8x;
#endif
	*dd_buf_lid = current_dds[dd_read_delta_position_8];	current_dds += DOMAIN_CELLS;	dd_buf_lid += 3*LOCAL_WORK_GROUP_SIZE;

#if CACHED_ACCESS
	dd_read_delta_position_11 = dd_read_delta_position_11x;
#endif
	*dd_buf_lid = current_dds[dd_read_delta_position_11];	current_dds += DOMAIN_CELLS;	dd_buf_lid -= LOCAL_WORK_GROUP_SIZE;

#if CACHED_ACCESS
	dd_read_delta_position_10 = dd_read_delta_position_10x;
#endif
	*dd_buf_lid = current_dds[dd_read_delta_position_10];	current_dds += DOMAIN_CELLS;


	barrier(CLK_LOCAL_MEM_FENCE);
	dd8 = dd_buf[8][neg_x_wrap];
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];
//	if (ff0 == -1024.0f)
//		neighbor_flag0 = FLAG_GAS;
//	else
		neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];

	dd9 = dd_buf[9][pos_x_wrap];
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];
//	if (ff1 == -1024.0f)
//		neighbor_flag1 = FLAG_GAS;
//	else
		neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];

	tmp = old_velocity_x+old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd8, neighbor_flag0, ff0, dd9, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd8*ff0;
	rho += dd8;
	velocity_x += dd8;
	T velocity_z = dd8;

	fluid_mass += dd9*ff1;
	rhob += dd9;
	velocity_x -= dd9;
	velocity_z -= dd9;

	dd10 = dd_buf[10][neg_x_wrap];
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];
//	if (ff0 == -1024.0f)
//		neighbor_flag0 = FLAG_GAS;
//	else
		neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];

	dd11 = dd_buf[11][pos_x_wrap];
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];
//	if (ff1 == -1024.0f)
//		neighbor_flag1 = FLAG_GAS;
//	else
		neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];

	tmp = old_velocity_x-old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd10, neighbor_flag0, ff0, dd11, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd10*ff0;
	rhoc += dd10;
	velocity_x += dd10;
	velocity_z -= dd10;

	fluid_mass += dd11*ff1;
	rhod += dd11;
	velocity_x -= dd11;
	velocity_z += dd11;

	// +++++++++++
	// +++ DD3 +++
	// +++++++++++

	// dd3: f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1)

#if CACHED_ACCESS
	dd_read_delta_position_13 = dd_read_delta_position_13x;
#endif
	dd13 = current_dds[dd_read_delta_position_13];	current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];
//	if (ff1 == -1024.0f)
//		neighbor_flag1 = FLAG_GAS;
//	else
		neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];

#if CACHED_ACCESS
	dd_read_delta_position_12 = dd_read_delta_position_12x;
#endif
	dd12 = current_dds[dd_read_delta_position_12];	current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];
//	if (ff0 == -1024.0f)
//		neighbor_flag0 = FLAG_GAS;
//	else
		neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];

	tmp = old_velocity_y+old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd12, neighbor_flag0, ff0, dd13, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd12*ff0;
	rho += dd12;
	velocity_y += dd12;
	velocity_z += dd12;

	fluid_mass += dd13*ff1;
	rhob += dd13;
	velocity_y -= dd13;
	velocity_z -= dd13;


#if CACHED_ACCESS
	dd_read_delta_position_15 = dd_read_delta_position_15x;
#endif
	dd15 = current_dds[dd_read_delta_position_15];	current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];
//	if (ff1 == -1024.0f)
//		neighbor_flag1 = FLAG_GAS;
//	else
		neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];

#if CACHED_ACCESS
	dd_read_delta_position_14 = dd_read_delta_position_14x;
#endif
	dd14 = current_dds[dd_read_delta_position_14];	current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];
//	if (ff0 == -1024.0f)
//		neighbor_flag0 = FLAG_GAS;
//	else
		neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];

	tmp = old_velocity_y-old_velocity_z;
	reconstruct_dd_45(flag, fluid_fraction, dd14, neighbor_flag0, ff0, dd15, neighbor_flag1, ff1, tmp, dd_param, dd_rho);

	fluid_mass += dd14*ff0;
	rhoc += dd14;
	velocity_y += dd14;
	velocity_z -= dd14;

	fluid_mass += dd15*ff1;
	rhod += dd15;
	velocity_y -= dd15;
	velocity_z += dd15;

	/*
	 * dd 16-18: f(0,0,1), f(0,0,-1),  f(0,0,0),  (not used)
	 */
#if CACHED_ACCESS
	dd_read_delta_position_17 = dd_read_delta_position_17x;
#endif
	dd17 = current_dds[dd_read_delta_position_17];	current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];
//	if (ff1 == -1024.0f)
//		neighbor_flag1 = FLAG_GAS;
//	else
		neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];

#if CACHED_ACCESS
	dd_read_delta_position_16 = dd_read_delta_position_16x;
#endif
	dd16 = current_dds[dd_read_delta_position_16];	current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];
//	if (ff0 == -1024.0f)
//		neighbor_flag0 = FLAG_GAS;
//	else
		neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];

	reconstruct_dd_01(flag, fluid_fraction, dd16, neighbor_flag0, ff0, dd17, neighbor_flag1, ff1, old_velocity_z, dd_param, dd_rho);

	fluid_mass += dd16*ff0;
	rho += dd16;
	velocity_z += dd16;

	fluid_mass += dd17*ff1;
	rhob += dd17;
	velocity_z -= dd17;

	dd18 = current_dds[gid];
	rhoc += dd18;

#endif
	// sum up density
	rho += rhob + rhoc + rhod;

	// compute new fluid mass
#if NUMERICAL_IMPROVED_MASS_EXCHANGE
	fluid_mass = fluid_mass_array[gid];
#else
	#if ACTIVATE_INCREASED_MASS_EXCHANGE
		fluid_mass = mass_exchange_factor*fluid_mass + fluid_mass_array[gid];
	#else
		fluid_mass = fluid_mass + fluid_mass_array[gid];
	#endif
#endif

	// fix the mass for strange values
	// !!!!!!!!!
	// we do this, because something is going wrong here and this just happens in the alpha kernel!
	// !!!!!!!!!
//	if (fluid_mass < -1.0f)	fluid_mass = -1.0f;
//	else if (fluid_mass > 2.0f)	fluid_mass = 2.0f;

	// compute fluid fraction
	if (flag == FLAG_INTERFACE)
		fluid_fraction = fluid_mass/rho;
	else if (flag == FLAG_FLUID)
		fluid_fraction = 1.0f;
	else
		fluid_fraction = 0.0f;

/*
#undef rhob
#undef rhoc
#undef rhod
#undef tmp
*/

#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
	velocity_x /= rho;
	velocity_y /= rho;
	velocity_z /= rho;
#endif

	/**
	 * collision operator
	 */
	#include "data/cl_programs/lbm_inc_collision_operator.h"

	barrier(CLK_LOCAL_MEM_FENCE);

	current_dds = &global_dd[0];
#if USE_SHARED_MEMORY
	dd_buf_lid = &dd_buf[0][lid];

	/* f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0) */
	dd_buf[0][pos_x_wrap] = dd0;
	dd_buf[1][neg_x_wrap] = dd1;
	barrier(CLK_LOCAL_MEM_FENCE);

	current_dds[dd_write_delta_position_0] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
	current_dds[dd_write_delta_position_1] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;	dd_buf_lid += 3*LOCAL_WORK_GROUP_SIZE;
	current_dds[dd_write_delta_position_2] = dd2;	current_dds += DOMAIN_CELLS;
	current_dds[dd_write_delta_position_3] = dd3;	current_dds += DOMAIN_CELLS;

	/* f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0) */
	dd_buf_lid = &dd_buf[0][lid];
	dd_buf[0][pos_x_wrap] = dd4;
	dd_buf[1][neg_x_wrap] = dd5;
	dd_buf[2][pos_x_wrap] = dd6;
	dd_buf[3][neg_x_wrap] = dd7;
	barrier(CLK_LOCAL_MEM_FENCE);

	current_dds[dd_write_delta_position_4] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
	current_dds[dd_write_delta_position_5] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
	current_dds[dd_write_delta_position_6] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
	current_dds[dd_write_delta_position_7] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;

	/* f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1) */
	dd_buf_lid = &dd_buf[0][lid];
	dd_buf[0][pos_x_wrap] = dd8;
	dd_buf[1][neg_x_wrap] = dd9;
	dd_buf[2][pos_x_wrap] = dd10;
	dd_buf[3][neg_x_wrap] = dd11;
	barrier(CLK_LOCAL_MEM_FENCE);

	current_dds[dd_write_delta_position_8] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
	current_dds[dd_write_delta_position_9] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
	current_dds[dd_write_delta_position_10] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;	dd_buf_lid += LOCAL_WORK_GROUP_SIZE;
	current_dds[dd_write_delta_position_11] = *dd_buf_lid;	current_dds += DOMAIN_CELLS;

	/* f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1) */
	current_dds[dd_write_delta_position_12] = dd12;	current_dds += DOMAIN_CELLS;
	current_dds[dd_write_delta_position_13] = dd13;	current_dds += DOMAIN_CELLS;
	current_dds[dd_write_delta_position_14] = dd14;	current_dds += DOMAIN_CELLS;
	current_dds[dd_write_delta_position_15] = dd15;	current_dds += DOMAIN_CELLS;

	/* f(0,0,1), f(0,0,-1),  f(0,0,0) */
	current_dds[dd_write_delta_position_16] = dd16;	current_dds += DOMAIN_CELLS;
	current_dds[dd_write_delta_position_17] = dd17;	current_dds += DOMAIN_CELLS;
	current_dds[gid] = dd18;
#else

	/* f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0) */
	current_dds[DOMAIN_WRAP(gid + DELTA_POS_X)] = dd0;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X)] = dd1;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y)] = dd2;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y)] = dd3;	current_dds += DOMAIN_CELLS;

	/* f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0) */
	current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)] = dd4;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)] = dd5;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)] = dd6;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)] = dd7;	current_dds += DOMAIN_CELLS;

	/* f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1) */
	current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)] = dd8;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)] = dd9;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)] = dd10;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)] = dd11;	current_dds += DOMAIN_CELLS;

	/* f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1) */
	current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)] = dd12;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)] = dd13;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)] = dd14;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)] = dd15;	current_dds += DOMAIN_CELLS;

	/* f(0,0,1), f(0,0,-1),  f(0,0,0) */
	current_dds[DOMAIN_WRAP(gid + DELTA_POS_Z)] = dd16;	current_dds += DOMAIN_CELLS;
	current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Z)] = dd17;	current_dds += DOMAIN_CELLS;
	current_dds[gid] = dd18;
#endif


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
