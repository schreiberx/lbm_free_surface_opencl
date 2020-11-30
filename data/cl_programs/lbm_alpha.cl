


#include "data/cl_programs/lbm_inc_header.h"


__kernel void kernel_lbm_alpha(
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

	// load cell type flag
	int flag = flag_array[gid];

	// load fluid fraction
	T fluid_fraction = fluid_fraction_array[gid];

	if (flag == FLAG_GAS)
	{
		// do nothing if cell is of type gas
		new_flag_array[gid] = FLAG_GAS;
		new_fluid_fraction_array[gid] = 0.0f;
		return;
	}

	// density distributions
	T dd0, dd1, dd2, dd3, dd4, dd5, dd6, dd7, dd8, dd9, dd10, dd11, dd12, dd13, dd14, dd15, dd16, dd17, dd18;

	// fluid fractions
	T ff0, ff1;

	T tmp;

	// helper variables
	T vel2;			// vel*vel
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
		// use 1.0 as density for gas pressure
//		dd_param = (T)1.0 - (T)(3.0/2.0)*(vel2);
		dd_param = (T)GAS_PRESSURE - (T)(3.0f/2.0f)*(vel2);
#endif
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	int neighbor_flag0, neighbor_flag1;

	/*
	 * pointer to density distributions
	 * we use a pointer instead of accessing the array directly
	 * - first this reduces the number of used registers (according to profiling information)
	 * - secondly the program runs faster and
	 */
	__global T *current_dds = &global_dd[gid];

	/*
	 * dd 0-3: f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0)
	 */
	dd0 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X)];

	dd1 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_X)];

	reconstruct_dd_01(flag, fluid_fraction, dd0, neighbor_flag0, ff0, dd1, neighbor_flag1, ff1, old_velocity_x, dd_param, dd_rho);

	T fluid_mass = dd0*ff0;
	T rho = dd0;
	T velocity_x = dd0;

	fluid_mass += dd1*ff1;
	T rhob = dd1;
	velocity_x -= dd1;

	dd2 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)];

	dd3 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y)];

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
	dd4 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)];

	dd5 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)];
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

	dd6 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)];

	dd7 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)];
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
	dd8 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)];

	dd9 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)];
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

	dd10 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)];

	dd11 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)];
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

	/*
	 * dd 12-15: f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1)
	 */
	dd12 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)];

	dd13 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)];

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

	dd14 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)];

	dd15 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)];

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
	dd16 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff0 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];
	neighbor_flag0 = flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)];

	dd17 = *current_dds;		current_dds += DOMAIN_CELLS;
	ff1 = fluid_fraction_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];
	neighbor_flag1 = flag_array[DOMAIN_WRAP(gid + DELTA_POS_Z)];

	reconstruct_dd_01(flag, fluid_fraction, dd16, neighbor_flag0, ff0, dd17, neighbor_flag1, ff1, old_velocity_z, dd_param, dd_rho);

	fluid_mass += dd16*ff0;
	rho += dd16;
	velocity_z += dd16;

	fluid_mass += dd17*ff1;
	rhob += dd17;
	velocity_z -= dd17;

	dd18 = *current_dds;
	rhoc += dd18;

	// sum up density
	rho += rhob + rhoc + rhod;


#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
	velocity_x /= rho;
	velocity_y /= rho;
	velocity_z /= rho;
#endif

	// compute new fluid mass
#if NUMERICAL_IMPROVED_MASS_EXCHANGE
	fluid_mass = fluid_mass_array[gid];
#else
		// compute new fluid mass
	#if ACTIVATE_INCREASED_MASS_EXCHANGE
		fluid_mass = mass_exchange_factor*fluid_mass + fluid_mass_array[gid];
	#else
		fluid_mass = fluid_mass + fluid_mass_array[gid];
	#endif
#endif

	// fix the mass for strange values
//	if (fluid_mass < -1.0f)	fluid_mass = -1.0f;
//opengop	else if (fluid_mass > 2.0f)	fluid_mass = 2.0f;

	// fix rho for strange values
//	if (rho < 0.9)	rho = 0.9;
//	else if (rho > 1.1)	rho = 1.1;

	// compute fluid fraction
	if (flag == FLAG_INTERFACE)
		fluid_fraction = fluid_mass/rho;
	else if (flag == FLAG_FLUID)
		fluid_fraction = 1.0f;
	else
		fluid_fraction = 0.0f;

	barrier(CLK_LOCAL_MEM_FENCE);

	/**
	 * collision operator
	 */
	#include "data/cl_programs/lbm_inc_collision_operator.h"

	barrier(CLK_LOCAL_MEM_FENCE);

	current_dds = &(global_dd[gid]);

	*current_dds = dd1;		current_dds += DOMAIN_CELLS;
	*current_dds = dd0;		current_dds += DOMAIN_CELLS;
	*current_dds = dd3;		current_dds += DOMAIN_CELLS;
	*current_dds = dd2;		current_dds += DOMAIN_CELLS;

	*current_dds = dd5;		current_dds += DOMAIN_CELLS;
	*current_dds = dd4;		current_dds += DOMAIN_CELLS;
	*current_dds = dd7;		current_dds += DOMAIN_CELLS;
	*current_dds = dd6;		current_dds += DOMAIN_CELLS;

	*current_dds = dd9;		current_dds += DOMAIN_CELLS;
	*current_dds = dd8;		current_dds += DOMAIN_CELLS;
	*current_dds = dd11;	current_dds += DOMAIN_CELLS;
	*current_dds = dd10;	current_dds += DOMAIN_CELLS;

	*current_dds = dd13;	current_dds += DOMAIN_CELLS;
	*current_dds = dd12;	current_dds += DOMAIN_CELLS;
	*current_dds = dd15;	current_dds += DOMAIN_CELLS;
	*current_dds = dd14;	current_dds += DOMAIN_CELLS;

	*current_dds = dd17;	current_dds += DOMAIN_CELLS;
	*current_dds = dd16;	current_dds += DOMAIN_CELLS;
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
