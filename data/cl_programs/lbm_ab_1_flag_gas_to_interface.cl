/**
 * this kernel is responsible to handle cells tagged with 'GAS_TO_INTERFACE'
 *
 * the density distributions are reconstructed and also the density and
 * velocity is innitialized with average data from adjacent fluid cells
 */
#include "data/cl_programs/lbm_inc_header.h"

#define STD_STUFF										\
	if (flag_array[dd_index] == FLAG_FLUID)				\
	{													\
		velocity_x += velocity_array[dd_index];			\
		velocity_y += velocity_array[DOMAIN_CELLS+dd_index];		\
		velocity_z += velocity_array[2*DOMAIN_CELLS+dd_index];		\
		rho += density_array[dd_index];					\
		count+=1.0f;										\
	}													\
	barrier(CLK_LOCAL_MEM_FENCE);

__kernel void kernel_ab_flag_gas_to_interface(
			__global T *global_dd,				// 0: density distributions
			__global int *flag_array,			// 1: flags
			__global T *velocity_array,			// 2: velocities
			__global T *density_array,			// 3: densities

			__global T *fluid_mass_array,		// 4: fluid mass
			__global T *fluid_fraction_array,	// 5: fluid fraction
			__const T mass_exchange_factor		// 6) mass exchange factor
		)
{
	const size_t gid = get_global_id(0);

	int flag = flag_array[gid];

	if (flag == FLAG_GAS_TO_INTERFACE)
	{
		T velocity_x, velocity_y, velocity_z;
		T rho;
		T count;

		size_t dd_index;
		__global T *current_dds;

		T vel2;		// vel*vel
		T vela2;
		T vela_velb;
		T vela_velb_2;
		T dd_param;

		// reconstruct density distribution
		// we leave the fluid mass and fluid fraction unchanged cz. those should be already around 1.0

		// compute average velocity of neighboring cells
		velocity_x = 0.0f;
		velocity_y = 0.0f;
		velocity_z = 0.0f;
		rho = 0.0f;

		count = 0.0f;

		// check neighbored cells if they are fluid cells and convert them to interface cells

		dd_index = DOMAIN_WRAP(gid + DELTA_NEG_X);			STD_STUFF;
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

		if (count > 1.0f)
		{
			T inv_count = 1.0f/count;

			velocity_x *= inv_count;	// average velocity
			velocity_y *= inv_count;
			velocity_z *= inv_count;

			rho *= inv_count;			// average rho
		}
		else if (count < 1.0f)
		{
			rho = 1.0f;
		}
//		rho += (float)(count < 1.0f);

		barrier(CLK_LOCAL_MEM_FENCE);

		vel2 = velocity_x*velocity_x + velocity_y*velocity_y + velocity_z*velocity_z;
#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
		dd_param = (T)(3.0/2.0)*(vel2);
#else
		dd_param = rho - (T)(3.0f/2.0f)*(vel2);
#endif

		current_dds = &global_dd[gid];

		/***********************
		 * DD0
		 ***********************/
		vela2 = velocity_x*velocity_x;
		*current_dds = eq_dd0(velocity_x, vela2, dd_param, rho);				current_dds += DOMAIN_CELLS;
		*current_dds = eq_dd1(velocity_x, vela2, dd_param, rho);				current_dds += DOMAIN_CELLS;

		vela2 = velocity_y*velocity_y;
		*current_dds = eq_dd0(velocity_y, vela2, dd_param, rho);				current_dds += DOMAIN_CELLS;
		*current_dds = eq_dd1(velocity_y, vela2, dd_param, rho);				current_dds += DOMAIN_CELLS;


#define vela_velb_2	vela2
		/***********************
		 * DD1
		 ***********************/
		vela_velb = velocity_x+velocity_y;
		vela_velb_2 = vela_velb*vela_velb;

		*current_dds = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;
		*current_dds = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;

		vela_velb = velocity_x-velocity_y;
		vela_velb_2 = vela_velb*vela_velb;

		*current_dds = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;
		*current_dds = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;

		/***********************
		 * DD2
		 ***********************/
		vela_velb = velocity_x+velocity_z;
		vela_velb_2 = vela_velb*vela_velb;

		*current_dds = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;
		*current_dds = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;

		vela_velb = velocity_x-velocity_z;
		vela_velb_2 = vela_velb*vela_velb;

		*current_dds = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;
		*current_dds = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;

		/***********************
		 * DD3
		 ***********************/
		vela_velb = velocity_y+velocity_z;
		vela_velb_2 = vela_velb*vela_velb;

		*current_dds = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;
		*current_dds = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;

		vela_velb = velocity_y-velocity_z;
		vela_velb_2 = vela_velb*vela_velb;

		*current_dds = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;
		*current_dds = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);		current_dds += DOMAIN_CELLS;
#undef vela_velb_2

		/***********************
		 * DD4
		 ***********************/
		vela2 = velocity_z*velocity_z;
		*current_dds = eq_dd0(velocity_z, vela2, dd_param, rho);			current_dds += DOMAIN_CELLS;
		*current_dds = eq_dd1(velocity_z, vela2, dd_param, rho);			current_dds += DOMAIN_CELLS;

		*current_dds = eq_dd18(dd_param, rho);

#if DD_FROM_OBSTACLES_TO_INTERFACE
		current_dds = &global_dd[0];

		/* f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0) */
		vela2 = velocity_x*velocity_x;
		if (flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X)] = eq_dd1(velocity_x, vela2, dd_param, rho);						current_dds += DOMAIN_CELLS;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_POS_X)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_POS_X)] = eq_dd0(velocity_x, vela2, dd_param, rho);						current_dds += DOMAIN_CELLS;

		vela2 = velocity_y*velocity_y;
		if (flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y)] = eq_dd1(velocity_y, vela2, dd_param, rho);						current_dds += DOMAIN_CELLS;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y)] = eq_dd0(velocity_y, vela2, dd_param, rho);						current_dds += DOMAIN_CELLS;

		/* f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0) */
		vela_velb = velocity_x+velocity_y;
		vela_velb_2 = vela_velb*vela_velb;
		if (flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Y)] = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Y)] = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		vela_velb = velocity_x-velocity_y;
		vela_velb_2 = vela_velb*vela_velb;
		if (flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Y)] = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Y)] = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		/* f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1) */
		vela_velb = velocity_x+velocity_z;
		vela_velb_2 = vela_velb*vela_velb;
		if (flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_NEG_Z)] = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_POS_Z)] = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		vela_velb = velocity_x-velocity_z;
		vela_velb_2 = vela_velb*vela_velb;
		if (flag_array[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_NEG_X + DELTA_POS_Z)] = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_POS_X + DELTA_NEG_Z)] = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		/* f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1) */
		vela_velb = velocity_y+velocity_z;
		vela_velb_2 = vela_velb*vela_velb;
		if (flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)] = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)] = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		vela_velb = velocity_y-velocity_z;
		vela_velb_2 = vela_velb*vela_velb;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)] = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)] = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);	current_dds += DOMAIN_CELLS;

		/***********************
		 * DD4
		 ***********************/
		vela2 = velocity_z*velocity_z;
		if (flag_array[DOMAIN_WRAP(gid + DELTA_NEG_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_NEG_Z)] = eq_dd1(velocity_z, vela2, dd_param, rho);						current_dds += DOMAIN_CELLS;

		if (flag_array[DOMAIN_WRAP(gid + DELTA_POS_Z)] == FLAG_OBSTACLE)
			current_dds[DOMAIN_WRAP(gid + DELTA_POS_Z)] = eq_dd0(velocity_z, vela2, dd_param, rho);						current_dds += DOMAIN_CELLS;
#endif

		current_dds = &velocity_array[gid];
		*current_dds = velocity_x;	current_dds += DOMAIN_CELLS;
		*current_dds = velocity_y;	current_dds += DOMAIN_CELLS;
		*current_dds = velocity_z;

		// GAS_TO_INTERFACE
		flag_array[gid] = FLAG_INTERFACE;
		density_array[gid] = rho;
		fluid_mass_array[gid] = 0.0f;
		fluid_fraction_array[gid] = 0.0f;
	}
}
