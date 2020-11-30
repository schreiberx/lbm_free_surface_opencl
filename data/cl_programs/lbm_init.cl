#include "data/cl_programs/lbm_inc_header.h"

// bitmask to determine what object to create"
//__global int init_fluid_flags;


enum
{
	INIT_CREATE_BREAKING_DAM			= (1<<0),
	INIT_CREATE_POOL					= (1<<1),
	INIT_CREATE_SPHERE					= (1<<2),
	INIT_CREATE_OBSTACLE_HALF_SPHERE	= (1<<3),
	INIT_CREATE_OBSTACLE_VERTICAL_BAR	= (1<<4),
	INIT_CREATE_FLUID_WITH_GAS_SPHERE	= (1<<5),
	INIT_CREATE_FILLED_CUBE				= (1<<6)
};

/**
 * return 3D position
 * \input linear_position	linear position in 3D cube (ordering: X,Y,Z)
 * \return	Vector with 3D position in cube
 */
inline int4 getCubePosition(int linear_position)
{
	int4 pos;

	// TODO: use AND operation to speed up
	// (but this function is only used during initialization)
	pos.x = (T)((int)linear_position % (int)DOMAIN_CELLS_X);
	linear_position /= DOMAIN_CELLS_X;

	pos.y = (T)((int)linear_position % (int)DOMAIN_CELLS_Y);
	linear_position /= DOMAIN_CELLS_Y;

	pos.z = linear_position;
	return pos;
}

__constant float gas_sphere_radius = 2.0f/3.0f;

int getFlag(int x, int y, int z, int init_fluid_flags)
{
	int flag = FLAG_GAS;

	if (	x <= 0 || y <= 0 || z <= 0 ||
			x >= DOMAIN_CELLS_X-1 || y >= DOMAIN_CELLS_Y-1 || z >= DOMAIN_CELLS_Z-1
	)
	{
		return FLAG_OBSTACLE;
	}

	if (init_fluid_flags & INIT_CREATE_FLUID_WITH_GAS_SPHERE)
	{
		T radius = ((float)min(min(DOMAIN_CELLS_X, DOMAIN_CELLS_Y), DOMAIN_CELLS_Z))*0.35*gas_sphere_radius;

		T dx = x - DOMAIN_CELLS_X/2;
		T dy = y - radius-2;
		T dz = z - DOMAIN_CELLS_Z/2;

		T dist = sqrt(dx*dx + dy*dy + dz*dz);

		if (dist < radius)
		{
			return FLAG_GAS;
		}
		else
		{
			if (DOMAIN_CELLS_Y*2/3 < y)
				return FLAG_GAS;
			else
				return FLAG_FLUID;
		}
	}

	/*
	 * setup FLUIDs
	 */
	if (init_fluid_flags & INIT_CREATE_BREAKING_DAM)
	{
		if (x >= DOMAIN_CELLS_X*2/3)
			flag = FLAG_FLUID;
	}

	if (init_fluid_flags & INIT_CREATE_POOL)
	{
		// NO interface between fluid and border
		if (y < DOMAIN_CELLS_Y/4)
			flag = FLAG_FLUID;
	}

	if (init_fluid_flags & INIT_CREATE_SPHERE)
	{
		T min = (DOMAIN_CELLS_X < DOMAIN_CELLS_Y ? DOMAIN_CELLS_X : DOMAIN_CELLS_Y);
		min = (min < DOMAIN_CELLS_Z ? min : DOMAIN_CELLS_Z)/2;

		T radius = min*2/3;

		T dx = x - DOMAIN_CELLS_X/2;
		T dy = DOMAIN_CELLS_Y - y - radius-3;
		T dz = z - DOMAIN_CELLS_Z/2;

		T dist = sqrt(dx*dx + dy*dy + dz*dz);

		if (dist < radius-sqrt(3.0f)*0.5f)
			flag = FLAG_FLUID;
	}

	/*
	 * SETUP OBSTACLES
	 */
	if (init_fluid_flags & INIT_CREATE_OBSTACLE_HALF_SPHERE)
	{
		T min = (DOMAIN_CELLS_X < DOMAIN_CELLS_Y ? DOMAIN_CELLS_X : DOMAIN_CELLS_Y);
		min = (min < DOMAIN_CELLS_Z ? min : DOMAIN_CELLS_Z)/2;

		T radius = min*2/3;

		T dx = x - DOMAIN_CELLS_X/2;
		T dy = y;
		T dz = z - DOMAIN_CELLS_Z/2;

		T dist = sqrt(dx*dx + dy*dy + dz*dz);

		if (dist < radius)
		{
			flag = FLAG_OBSTACLE;
		}
	}


	if (init_fluid_flags & INIT_CREATE_OBSTACLE_VERTICAL_BAR)
	{
		T min = (DOMAIN_CELLS_X < DOMAIN_CELLS_Y ? DOMAIN_CELLS_X : DOMAIN_CELLS_Y);
		min = (min < DOMAIN_CELLS_Z ? min : DOMAIN_CELLS_Z)/2;

		T radius = min*1/5;

		T dx = x - DOMAIN_CELLS_X/2;
		T dy = y - DOMAIN_CELLS_Y/2;
		T dz = z - DOMAIN_CELLS_Z/2;

		T dist = sqrt(dx*dx + dz*dz);

		if (dist < radius)
		{
			flag = FLAG_OBSTACLE;
		}
	}

	if (init_fluid_flags & INIT_CREATE_FILLED_CUBE)
	{
		return FLAG_FLUID;
	}

	return flag;
}


float getInterfaceFluidFraction(int x, int y, int z, int init_fluid_flags)
{
	float fluid_fraction = 0.0f;

	for (int pz = -1; pz <= 1; pz++)
		for (int py = -1; py <= 1; py++)
			for (int px = -1; px <= 1; px++)
			{
				int flag = getFlag(px, py, pz, init_fluid_flags);
				if (flag == FLAG_FLUID)
					fluid_fraction += 1.0f/sqrt((float)(px*px + py*py + pz*pz));
			}

	// multiplying with 10.0 makes it somehow smoother
	fluid_fraction *= 10.0f;
//return 0.0;
	return fluid_fraction / (8.0/sqrt(3.0) + 12.0/sqrt(2.0) + 6.0);
}


bool checkNeighborFlag(int x, int y, int z, int flag, int init_fluid_flags)
{
	if (getFlag(x-1, y, z, init_fluid_flags) == flag)	return true;
	if (getFlag(x+1, y, z, init_fluid_flags) == flag)	return true;
	if (getFlag(x, y-1, z, init_fluid_flags) == flag)	return true;
	if (getFlag(x, y+1, z, init_fluid_flags) == flag)	return true;
	if (getFlag(x, y, z-1, init_fluid_flags) == flag)	return true;
	if (getFlag(x, y, z+1, init_fluid_flags) == flag)	return true;

	if (getFlag(x-1, y-1, z, init_fluid_flags) == flag)	return true;
	if (getFlag(x+1, y-1, z, init_fluid_flags) == flag)	return true;
	if (getFlag(x-1, y+1, z, init_fluid_flags) == flag)	return true;
	if (getFlag(x+1, y+1, z, init_fluid_flags) == flag)	return true;

	if (getFlag(x-1, y, z-1, init_fluid_flags) == flag)	return true;
	if (getFlag(x+1, y, z-1, init_fluid_flags) == flag)	return true;
	if (getFlag(x-1, y, z+1, init_fluid_flags) == flag)	return true;
	if (getFlag(x+1, y, z+1, init_fluid_flags) == flag)	return true;

	if (getFlag(x, y-1, z-1, init_fluid_flags) == flag)	return true;
	if (getFlag(x, y+1, z-1, init_fluid_flags) == flag)	return true;
	if (getFlag(x, y-1, z+1, init_fluid_flags) == flag)	return true;
	if (getFlag(x, y+1, z+1, init_fluid_flags) == flag)	return true;

	return false;
}

/**
 * INIT KERNEL
 */
__kernel void kernel_lbm_init(
		__global T *global_dd,			// 0) density distributions
		__global int flag_array[DOMAIN_CELLS],			// 1) flags
		__global T *velocity_array,		// 2) velocities
		__global T density_array[DOMAIN_CELLS],			// 3) densities
		__const T inv_tau,								// 4) "weight" for collision function
		__const T inv_trt_tau,							// 5) "weight" for TRT collision model

		__global T fluid_mass_array[DOMAIN_CELLS],		// 6) fluid mass
		__global T fluid_fraction_array[DOMAIN_CELLS],	// 7) fluid fraction

		__global int new_flag_array[DOMAIN_CELLS],		// 8) NEW flags
		__global T new_fluid_fraction_array[DOMAIN_CELLS],	// 9) NEW fluid fraction

		__const T gravitation0,							// 10) gravitation
		__const T gravitation1,
		__const T gravitation2,

		int init_fluid_flags							// 13) init flags
)
{
//	init_fluid_flags = p_init_fluid_flags;

	const size_t gid = get_global_id(0);


	// initialize flag field
	int4 pos = getCubePosition(gid);
	pos.w = 0;

	int flag = getFlag(pos.x, pos.y, pos.z, init_fluid_flags);

	// density distributions
	T dd0, dd1, dd2, dd3, dd4, dd5, dd6, dd7, dd8, dd9, dd10, dd11, dd12, dd13, dd14, dd15, dd16, dd17, dd18;

	T dd_param;
	T vela2;
	T vela_velb;

	T velocity_x = 0.0f;
	T velocity_y = 0.0f;
	T velocity_z = 0.0f;

	// parameter for fluid cells
	T rho = 1.0f;
	T p_fluid_fraction = 1.0f;
	T p_fluid_mass = 1.0f;

	if (flag == FLAG_GAS)
	{
		if (checkNeighborFlag(pos.x, pos.y, pos.z, FLAG_FLUID, init_fluid_flags))
			flag = FLAG_INTERFACE;
	}

	switch(flag)
	{
		case FLAG_OBSTACLE:
			p_fluid_fraction = 0.0f;
			p_fluid_mass = 0.0f;
			break;

		case FLAG_FLUID:
			p_fluid_fraction = 1.0f;
			p_fluid_mass = 1.0f;
			break;

		case FLAG_GAS:
#if SETUP_LARGE_VELOCITY
			velocity_x = 999.999;
			velocity_y = -999.999;
			velocity_z = 999.999;
#else
			velocity_x = 0.0f;
			velocity_y = 0.0f;
			velocity_z = 0.0f;
#endif
			p_fluid_fraction = 0.0f;
			p_fluid_mass = 0.0f;
			break;

		case FLAG_INTERFACE:
			p_fluid_fraction = getInterfaceFluidFraction(pos.x, pos.y, pos.z, init_fluid_flags);
			p_fluid_mass = p_fluid_fraction;
			break;
	}

	__global T *current_dds = &global_dd[gid];

	// compute and store velocity
#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
	dd_param = (T)(3.0f/2.0f)*(velocity_x*velocity_x+velocity_y*velocity_y+velocity_z*velocity_z);
#else
	dd_param = rho - (T)(3.0f/2.0f)*(velocity_x*velocity_x+velocity_y*velocity_y+velocity_z*velocity_z);
#endif

	vela2 = velocity_x*velocity_x;
	dd0 = eq_dd0(velocity_x, vela2, dd_param, rho);
	*current_dds = dd0;		current_dds += DOMAIN_CELLS;
	dd1 = eq_dd1(velocity_x, vela2, dd_param, rho);
	*current_dds = dd1;		current_dds += DOMAIN_CELLS;

	vela2 = velocity_y*velocity_y;

	dd2 = eq_dd0(velocity_y, vela2, dd_param, rho);
	*current_dds = dd2;		current_dds += DOMAIN_CELLS;
	dd3 = eq_dd1(velocity_y, vela2, dd_param, rho);
	*current_dds = dd3;		current_dds += DOMAIN_CELLS;


#define vela_velb_2	vela2
	/***********************
	 * DD1
	 ***********************/
	vela_velb = velocity_x+velocity_y;
	vela_velb_2 = vela_velb*vela_velb;

	dd4 = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd4;		current_dds += DOMAIN_CELLS;
	dd5 = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd5;		current_dds += DOMAIN_CELLS;

	vela_velb = velocity_x-velocity_y;
	vela_velb_2 = vela_velb*vela_velb;

	dd6 = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd6;		current_dds += DOMAIN_CELLS;
	dd7 = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd7;		current_dds += DOMAIN_CELLS;

	/***********************
	 * DD2
	 ***********************/
	vela_velb = velocity_x+velocity_z;
	vela_velb_2 = vela_velb*vela_velb;

	dd8 = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd8;		current_dds += DOMAIN_CELLS;
	dd9 = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd9;		current_dds += DOMAIN_CELLS;

	vela_velb = velocity_x-velocity_z;
	vela_velb_2 = vela_velb*vela_velb;

	dd10 = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd10;		current_dds += DOMAIN_CELLS;
	dd11 = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd11;		current_dds += DOMAIN_CELLS;

	/***********************
	 * DD3
	 ***********************/
	vela_velb = velocity_y+velocity_z;
	vela_velb_2 = vela_velb*vela_velb;


	dd12 = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd12;		current_dds += DOMAIN_CELLS;
	dd13 = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd13;		current_dds += DOMAIN_CELLS;

	vela_velb = velocity_y-velocity_z;
	vela_velb_2 = vela_velb*vela_velb;

	dd14 = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd14;		current_dds += DOMAIN_CELLS;
	dd15 = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	*current_dds = dd15;		current_dds += DOMAIN_CELLS;


#undef vela_velb_2
	/***********************
	 * DD4
	 ***********************/
	vela2 = velocity_z*velocity_z;

	dd16 = eq_dd0(velocity_z, vela2, dd_param, rho);
	*current_dds = dd16;		current_dds += DOMAIN_CELLS;
	dd17 = eq_dd1(velocity_z, vela2, dd_param, rho);
	*current_dds = dd17;		current_dds += DOMAIN_CELLS;

	dd18 = eq_dd18(dd_param, rho);
	*current_dds = dd18;

	// update flags, fraction and mass
	flag_array[gid] = flag;
	fluid_fraction_array[gid] = p_fluid_fraction;
	fluid_mass_array[gid] = p_fluid_mass;

	new_flag_array[gid] = flag;
	new_fluid_fraction_array[gid] = p_fluid_fraction;

	// store velocity
	velocity_array[gid] = velocity_x;
	velocity_array[DOMAIN_CELLS+gid] = velocity_y;
	velocity_array[DOMAIN_CELLS*2+gid] = velocity_z;

	// store density
	density_array[gid] = rho;
}
