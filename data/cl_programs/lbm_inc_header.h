/***********************************************************************
 * parameters
 ***********************************************************************/

#include "data/cl_programs/lbm_config.cl"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define FLAGS_GAS_OBSTACLE				(FLAG_GAS | FLAG_OBSTACLE)
#define FLAGS_GAS_OBSTACLE_INTERFACE	(FLAG_GAS | FLAG_OBSTACLE | FLAG_INTERFACE)
#define FLAGS_FLUID_INTERFACE			(FLAG_FLUID | FLAG_INTERFACE)

#define DOMAIN_CELLS		(DOMAIN_CELLS_X*DOMAIN_CELLS_Y*DOMAIN_CELLS_Z)
#define DOMAIN_SLICE_CELLS	(DOMAIN_CELLS_X*DOMAIN_CELLS_Y)

/**
 * Next we define the delta values for the neighbor cells.
 *
 * Because we use modulo computations (in reality we use a bitmask in the
 * case that the domain resolutions are pow2), e. g. we dont subtract 1 to access the left
 * neighbor, but use a additive components of (DOMAIN_CELLS-1).
 *
 * Otherwise we would run into more computations due to the modulo computations of negative numbers.
 */
#define DELTA_POS_X		(1)
#define DELTA_NEG_X		(DOMAIN_CELLS-1)

#define DELTA_POS_Y		(DOMAIN_CELLS_X)
#define DELTA_NEG_Y		(DOMAIN_CELLS-DOMAIN_CELLS_X)

#define DELTA_POS_Z		(DOMAIN_SLICE_CELLS)
#define DELTA_NEG_Z		(DOMAIN_CELLS-DOMAIN_SLICE_CELLS)

/**
 * include the wrapping stuff
 */
#include "data/cl_programs/wrap.h"


/***********************************************************************
 * equilibrium distributions f_eq for incompressible fluids (not used in this simulation)
 ***********************************************************************/
/**
 * (for more information, have a look at Thuerey_070313.pdf, page 14)
 * f_eq = w_i * (	\rho + 3.0*(e_i*u) + (9.0/2.0)*(e_i*u)^2 - (3.0/2.0)*(u)^2	)
 * w_i = 1/3	for (0,0,0)
 * w_i = 1/18   for (+-1,0,0), (0,+-1, 0), (0,0,+-1)
 * w_i = 1/36   for (+-1,+-1,+-1)
 */

/*
 * we can reuse the following equilibrium distribution function because of its symmetry
 *
 * eq_dd0, eq_dd1 and eq_dd0_plus_dd1 accounts for the following lattice vectors:
 *
 * f(1,0,0), f(-1,0,0),  f(0,1,0),  f(0,-1,0) f(0,0,1) f(0,0,-1)
 *
 * vela = velocity_x
 * vela2 = velocity_x^2
 * rho_alpha = rho - (T)(3.0/2.0)*(vel2);
 */
#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
	#define eq_dd0(p_vela, p_vela2, p_alpha, p_rho)					\
		((T)(1.0f/18.0f)*p_rho*((T)1.0f + (T)(3.0f)*(p_vela) + (T)(9.0f/2.0f)*(p_vela2) - p_alpha))
	#define eq_dd1(p_vela, p_vela2, p_alpha, p_rho)					\
		((T)(1.0f/18.0f)*p_rho*((T)1.0f + (T)(-3.0f)*(p_vela) + (T)(9.0f/2.0f)*(p_vela2) - p_alpha))
	#define eq_dd0_plus_dd1(p_vela, p_vela2, p_alpha, p_rho)		\
		((T)(2.0f/18.0f)*p_rho*((T)1.0f + (T)(9.0f/2.0f)*(p_vela2) - p_alpha))
#else
	#define eq_dd0(p_vela, p_vela2, p_rho_alpha, p_rho)				\
		((T)(1.0f/18.0f)*((p_rho_alpha) + (T)(3.0f)*(p_vela) + (T)(9.0f/2.0f)*(p_vela2)))
	#define eq_dd1(p_vela, p_vela2, p_rho_alpha, p_rho)				\
		((T)(1.0f/18.0f)*((p_rho_alpha) + (T)(-3.0f)*(p_vela) + (T)(9.0f/2.0f)*(p_vela2)))
	#define eq_dd0_plus_dd1(p_vela, p_vela2, p_rho_alpha, p_rho)	\
		((T)(2.0f/18.0f)*((p_rho_alpha) + (T)(9.0f/2.0f)*(p_vela2)))
#endif

/*
 * we can reuse the following functions because of the symmetry of the density distributions
 *
 * eq_dd4, eq_dd5 and eq_dd4_plus_dd5 accounts for the following lattice vectors:
 *
 * f(1,1,0), f(-1,-1,0), f(1,-1,0), f(-1,1,0)
 * f(1,0,1), f(-1,0,-1), f(1,0,-1), f(-1,0,1)
 * f(0,1,1), f(0,-1,-1), f(0,1,-1), f(0,-1,1)
 *
 * velx_add_vely = velocity_x + velocity_y
 * velx_add_vely_2 = velx_add_vely * velx_add_vely
 * rho_alpha = rho - (T)(3.0f/2.0f)*(velx_add_vely_2);
 */
#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
	#define eq_dd4(p_velx_add_vely, p_velx_add_vely_2, p_alpha, p_rho)				\
			((T)(1.0f/36.0f)*p_rho*((T)1.0f + (T)(3.0f)*(p_velx_add_vely) + (T)(9.0f/2.0f)*(p_velx_add_vely_2) - p_alpha))
	#define eq_dd5(p_velx_add_vely, p_velx_add_vely_2, p_alpha, p_rho)				\
			((T)(1.0f/36.0f)*p_rho*((T)1.0f + (T)(-3.0f)*(p_velx_add_vely) + (T)(9.0f/2.0f)*(p_velx_add_vely_2) - p_alpha))
	#define eq_dd4_plus_dd5(p_velx_add_vely, p_velx_add_vely_2, p_alpha, p_rho)		\
			((T)(2.0f/36.0f)*p_rho*((T)1.0f + (T)(9.0f/2.0f)*(p_velx_add_vely_2) - p_alpha))
#else
	#define eq_dd4(p_velx_add_vely, p_velx_add_vely_2, p_rho_alpha, p_rho)			\
			((T)(1.0f/36.0f)*((p_rho_alpha) + (T)(3.0f)*(p_velx_add_vely) + (T)(9.0f/2.0f)*(p_velx_add_vely_2)))
	#define eq_dd5(p_velx_add_vely, p_velx_add_vely_2, p_rho_alpha, p_rho)			\
			((T)(1.0f/36.0f)*((p_rho_alpha) + (T)(-3.0f)*(p_velx_add_vely) + (T)(9.0f/2.0f)*(p_velx_add_vely_2)))
	#define eq_dd4_plus_dd5(p_velx_add_vely, p_velx_add_vely_2, p_rho_alpha, p_rho)	\
			((T)(2.0f/36.0f)*((p_rho_alpha) + (T)(9.0f/2.0f)*(p_velx_add_vely_2)))
#endif

/*
 * f(0,0,0)
 */
#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
	#define eq_dd18(p_alpha, p_rho)	((T)(1.0f/3.0f)*p_rho*((T)1.0f - p_alpha))
#else
	#define eq_dd18(p_rho_alpha, p_rho)	((T)(1.0f/3.0f)*(p_rho_alpha))
#endif

/***********************************************************************
 * STUFF FOR BETA KERNEL
 * some defines are just for convenience
 ***********************************************************************/

// READ FROM LOCAL STORE
#define dd_read_delta_position_0x	(read_delta_neg_x)
#define dd_read_delta_position_1x	(read_delta_pos_x)
#define dd_read_delta_position_2x	DOMAIN_WRAP(gid + DELTA_NEG_Y)
#define dd_read_delta_position_3x	DOMAIN_WRAP(gid + DELTA_POS_Y)

#define dd_read_delta_position_4x	DOMAIN_WRAP(read_delta_neg_x + DELTA_NEG_Y)
#define dd_read_delta_position_5x	DOMAIN_WRAP(read_delta_pos_x + DELTA_POS_Y)
#define dd_read_delta_position_6x	DOMAIN_WRAP(read_delta_neg_x + DELTA_POS_Y)
#define dd_read_delta_position_7x	DOMAIN_WRAP(read_delta_pos_x + DELTA_NEG_Y)

#define	dd_read_delta_position_8x	DOMAIN_WRAP(read_delta_neg_x + DELTA_NEG_Z)
#define dd_read_delta_position_9x	DOMAIN_WRAP(read_delta_pos_x + DELTA_POS_Z)
#define dd_read_delta_position_10x	DOMAIN_WRAP(read_delta_neg_x + DELTA_POS_Z)
#define dd_read_delta_position_11x	DOMAIN_WRAP(read_delta_pos_x + DELTA_NEG_Z)

#define dd_read_delta_position_12x	DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_NEG_Z)
#define dd_read_delta_position_13x	DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_POS_Z)
#define dd_read_delta_position_14x	DOMAIN_WRAP(gid + DELTA_NEG_Y + DELTA_POS_Z)
#define dd_read_delta_position_15x	DOMAIN_WRAP(gid + DELTA_POS_Y + DELTA_NEG_Z)

#define dd_read_delta_position_16x	DOMAIN_WRAP(gid + DELTA_NEG_Z)
#define dd_read_delta_position_17x	DOMAIN_WRAP(gid + DELTA_POS_Z)

#define dd_read_delta_position_0	dd_read_delta_position_0x
#define dd_read_delta_position_1	dd_read_delta_position_1x


#if !CACHED_ACCESS
// if CACHED_ACCESS is disabled, the delta positions for reading are not stored and
// recomputed for write access (if the compiler doesn't optimize it)
#define dd_read_delta_position_2	dd_read_delta_position_2x
#define dd_read_delta_position_3	dd_read_delta_position_3x
#define dd_read_delta_position_4	dd_read_delta_position_4x
#define dd_read_delta_position_5	dd_read_delta_position_5x
#define dd_read_delta_position_6	dd_read_delta_position_6x
#define dd_read_delta_position_7	dd_read_delta_position_7x
#define dd_read_delta_position_8	dd_read_delta_position_8x
#define dd_read_delta_position_9	dd_read_delta_position_9x
#define dd_read_delta_position_10	dd_read_delta_position_10x
#define dd_read_delta_position_11	dd_read_delta_position_11x
#define dd_read_delta_position_12	dd_read_delta_position_12x
#define dd_read_delta_position_13	dd_read_delta_position_13x
#define dd_read_delta_position_14	dd_read_delta_position_14x
#define dd_read_delta_position_15	dd_read_delta_position_15x
#define dd_read_delta_position_16	dd_read_delta_position_16x
#define dd_read_delta_position_17	dd_read_delta_position_17x
#endif



// WRITE FROM LOCAL STORE
#define dd_write_delta_position_0	dd_read_delta_position_1
#define dd_write_delta_position_1	dd_read_delta_position_0
#define dd_write_delta_position_2	dd_read_delta_position_3
#define dd_write_delta_position_3	dd_read_delta_position_2

#define dd_write_delta_position_4	dd_read_delta_position_5
#define dd_write_delta_position_5	dd_read_delta_position_4
#define dd_write_delta_position_6	dd_read_delta_position_7
#define dd_write_delta_position_7	dd_read_delta_position_6

#define dd_write_delta_position_8	dd_read_delta_position_9
#define dd_write_delta_position_9	dd_read_delta_position_8
#define dd_write_delta_position_10	dd_read_delta_position_11
#define dd_write_delta_position_11	dd_read_delta_position_10

#define dd_write_delta_position_12	dd_read_delta_position_13
#define dd_write_delta_position_13	dd_read_delta_position_12
#define dd_write_delta_position_14	dd_read_delta_position_15
#define dd_write_delta_position_15	dd_read_delta_position_14

#define dd_write_delta_position_16	dd_read_delta_position_17
#define dd_write_delta_position_17	dd_read_delta_position_16

/**
 * return the factor for the mass exchange
 *
 * \param fluid_fraction	fluid fraction of current cell
 * \param flag				flag of current cell
 * \param ff				fluid fraction of adjacent cell
 * \param neighbor_flag		flag of neighbor cell
 */
#if 0
	#define GET_MX_FACTOR_INCOMING(fluid_fraction, flag, ff, neighbor_flag)						\
		if ((flag | neighbor_flag) & FLAGS_GAS_OBSTACLE)										\
		{																						\
			/* one of the cell is either an gas or an obstacle cell */							\
			ff = 0.0;																			\
		}																						\
		else if ((flag | neighbor_flag) == FLAG_INTERFACE)										\
		{																						\
			/* both cells are interface cells */												\
			/* average fluid fraction between 2 interface cells */								\
			ff = (ff + fluid_fraction)*0.5;														\
		}																						\
		else																					\
		{																						\
			/* only one cell is an interface cell, the other one a fluid cell */				\
			/* both cells are fluid cells */													\
			ff = 1.0;																			\
		}
#else
	#define GET_MX_FACTOR_INCOMING(fluid_fraction, flag, ff, neighbor_flag)					\
	{																						\
		ff = ((float)(((flag | neighbor_flag) & FLAGS_GAS_OBSTACLE) == 0)) *				\
			(1.0f - 		/* else part*/														\
				((float)((flag & neighbor_flag) == FLAG_INTERFACE))*(1.0f - (ff + fluid_fraction)*0.5f)	\
		);																					\
	}
#endif


#define GET_MX_FACTOR_OUTGOING(fluid_fraction, flag, ff, neighbor_flag)		\
			GET_MX_FACTOR_INCOMING(fluid_fraction, flag, ff, neighbor_flag)

/********************************************************************
 * reconstruction of dds streaming to interface cells from gas cells
 ********************************************************************/

#if 0
/********************************************************************
 * reconstruction using ZERO velocity
 ********************************************************************/

#define reconstruct_dd_01(															\
							p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho						\
)																					\
{																					\
	p_old_velocity_x = 0.0;															\
	p_dd_param = 1.0;																\
	if (p_flag != FLAG_INTERFACE)													\
	{																				\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
	else if ((p_neighbor_flag0 & FLAGS_GAS_OBSTACLE) && (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE))			\
	{																				\
		/* both cells are gas or obstacle cells */									\
		/* => use equilibrium distribution */										\
		vela2 = 0*0;																\
		p_dd0 = eq_dd0(0, vela2, p_dd_param, p_rho);								\
		p_dd1 = eq_dd1(0, vela2, p_dd_param, p_rho);								\
		p_ff0 = 0.0;																\
		p_ff1 = 0.0;																\
	}																				\
	else if (p_neighbor_flag0 & FLAGS_GAS_OBSTACLE)									\
	{																				\
		/* only cell 0 is gas cell */												\
		vela2 = 0*0;																\
		p_dd0 = eq_dd0(0, vela2, p_dd_param, p_rho);								\
		p_ff0 = 0.0;																\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
	else if (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE)									\
	{																				\
		/* only cell 1 is gas cell */												\
		vela2 = 0*0;																\
		p_dd1 = eq_dd1(0, vela2, p_dd_param, p_rho);								\
		p_ff1 = 0.0;																\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
	}																				\
	else																			\
	{																				\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
}



#define reconstruct_dd_45(															\
	p_flag, p_fluid_fraction,														\
	p_dd0, p_neighbor_flag0, p_ff0,													\
	p_dd1, p_neighbor_flag1, p_ff1,													\
	p_old_velocity_aaa, p_dd_param, p_rho											\
		)																			\
{																					\
	if (p_flag != FLAG_INTERFACE)													\
	{																				\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
	else if ((p_neighbor_flag0 & FLAGS_GAS_OBSTACLE) && (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE))			\
	{																				\
		/* both cells are gas cells */												\
		/* => use equilibrium distribution */										\
		vela2 = 0*0;																\
		p_dd0 = eq_dd4(0, vela2, p_dd_param, p_rho);								\
		p_dd1 = eq_dd5(0, vela2, p_dd_param, p_rho);								\
		p_ff0 = 0.0;																\
		p_ff1 = 0.0;																\
	}																				\
	else if (p_neighbor_flag0 & FLAGS_GAS_OBSTACLE)									\
	{																				\
		/* only cell 0 is gas cell */												\
		vela2 = 0*0;																\
		p_dd0 = eq_dd4(0, vela2, p_dd_param, p_rho);								\
		p_ff0 = 0.0;																\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
	else if (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE)									\
	{																				\
		/* only cell 1 is gas cell */												\
		vela2 = 0*0;																\
		p_dd1 = eq_dd5(0, vela2, p_dd_param, p_rho);								\
		p_ff1 = 0.0;																\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
	}																				\
	else																			\
	{																				\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
}										

#elif 0

/********************************************************************
 * reconstruction using OLD velocity
 ********************************************************************/
#define reconstruct_dd_01(	p_flag, p_fluid_fraction,								\
				p_dd0, p_neighbor_flag0, p_ff0,										\
				p_dd1, p_neighbor_flag1, p_ff1,										\
				p_old_velocity_x, p_dd_param										\
		)																			\
{																					\
	if (p_flag != FLAG_INTERFACE)													\
	{																				\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
	else if ((p_neighbor_flag0 & FLAGS_GAS_OBSTACLE) && (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE))			\
	{																				\
		/* both cells are gas or obstacle cells */									\
		/* => use equilibrium distribution */										\
		vela2 = p_old_velocity_x*p_old_velocity_x;									\
		p_dd0 = eq_dd0(p_old_velocity_x, vela2, p_dd_param);						\
		p_dd1 = eq_dd1(p_old_velocity_x, vela2, p_dd_param);						\
		p_ff0 = 0.0;																\
		p_ff1 = 0.0;																\
	}																				\
	else if (p_neighbor_flag0 & FLAGS_GAS_OBSTACLE)									\
	{																				\
		/* only cell 0 is gas cell */												\
		vela2 = p_old_velocity_x*p_old_velocity_x;									\
		p_dd0 = eq_dd0(p_old_velocity_x, vela2, p_dd_param);						\
		p_ff0 = 0.0;																\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
	else if (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE)									\
	{																				\
		/* only cell 1 is gas cell */												\
		vela2 = p_old_velocity_x*p_old_velocity_x;									\
		p_dd1 = eq_dd1(p_old_velocity_x, vela2, p_dd_param);						\
		p_ff1 = 0.0;																\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
	}																				\
	else																			\
	{																				\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
}



#define reconstruct_dd_45(	p_flag, p_fluid_fraction,								\
				p_dd0, p_neighbor_flag0, p_ff0,										\
				p_dd1, p_neighbor_flag1, p_ff1,										\
				p_old_velocity_aaa, p_dd_param										\
		)																			\
{																					\
	if (p_flag != FLAG_INTERFACE)													\
	{																				\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
	else if ((p_neighbor_flag0 & FLAGS_GAS_OBSTACLE) && (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE))			\
	{																				\
		/* both cells are gas cells */												\
		/* => use equilibrium distribution */										\
		vela2 = p_old_velocity_aaa*p_old_velocity_aaa;								\
		p_dd0 = eq_dd4(p_old_velocity_aaa, vela2, p_dd_param);						\
		p_dd1 = eq_dd5(p_old_velocity_aaa, vela2, p_dd_param);						\
		p_ff0 = 0.0;																\
		p_ff1 = 0.0;																\
	}																				\
	else if (p_neighbor_flag0 & FLAGS_GAS_OBSTACLE)									\
	{																				\
		/* only cell 0 is gas cell */												\
		vela2 = p_old_velocity_aaa*p_old_velocity_aaa;								\
		p_dd0 = eq_dd4(p_old_velocity_aaa, vela2, p_dd_param);						\
		p_ff0 = 0.0;																\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
	else if (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE)									\
	{																				\
		/* only cell 1 is gas cell */												\
		vela2 = p_old_velocity_aaa*p_old_velocity_aaa;								\
		p_dd1 = eq_dd5(p_old_velocity_aaa, vela2, p_dd_param);						\
		p_ff1 = 0.0;																\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
	}																				\
	else																			\
	{																				\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	}																				\
}										

#elif 1

/********************************************************************
 * reconstruct density distribution for interface cells
 *
 * using OLD velocity and opposite DDs
 *
 * this produces the best results and is the preferred method!
 *
 * IMPORTANT: the number behind the 'variables' (e. g. 0 for p_dd0) does not
 * specify the lattice vector! it's just to enumerate the variables.
 *
 *
 * OUTPUT:
 * \param p_dd0:	original or reconstructed density distribution
 * \param p_dd1:	original or reconstructed density distribution
 * \param p_ff0:	fluid fraction of adjacent cell in direction of p_dd0
 * \param p_ff1:	fluid fraction of adjacent cell
 ********************************************************************/

#if !DD_FROM_OBSTACLES_TO_INTERFACE
	#define reconstruct_dd_xxx(	p_flag, p_fluid_fraction,								\
								p_dd0, p_neighbor_flag0, p_ff0,							\
								p_dd1, p_neighbor_flag1, p_ff1,							\
								p_old_velocity_x, p_dd_param, p_rho,					\
								p_dda, p_ddb											\
			)																			\
	{																					\
		if (p_flag != FLAG_INTERFACE)													\
		{																				\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
		}																				\
		else if ((p_neighbor_flag0 & FLAGS_GAS_OBSTACLE) && (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE))	\
		{																				\
			/* both cells are gas or obstacle cells */									\
			/* => use equilibrium distribution */										\
			vela2 = p_old_velocity_x*p_old_velocity_x;									\
			p_dd0 = eq_##p_dda(p_old_velocity_x, vela2, p_dd_param, p_rho);				\
			p_dd1 = eq_##p_ddb(p_old_velocity_x, vela2, p_dd_param, p_rho);				\
			p_ff0 = 0.0f;																\
			p_ff1 = 0.0f;																\
		}																				\
		else if (p_neighbor_flag0 & FLAGS_GAS_OBSTACLE)									\
		{																				\
			/* only cell 0 is gas cell */												\
			vela2 = p_old_velocity_x*p_old_velocity_x;									\
			p_dd0 = eq_##p_dda##_plus_##p_ddb(p_old_velocity_x, vela2, p_dd_param, p_rho) - p_dd1;		\
			p_ff0 = 0.0f;																\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
		}																				\
		else if (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE)									\
		{																				\
			/* only cell 1 is gas cell */												\
			vela2 = p_old_velocity_x*p_old_velocity_x;									\
			p_dd1 = eq_##p_dda##_plus_##p_ddb(p_old_velocity_x, vela2, p_dd_param, p_rho) - p_dd0;	\
			p_ff1 = 0.0f;																\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		}																				\
		else																			\
		{																				\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
		}																				\
		/* leave disabled for better performance results */								\
		barrier(CLK_LOCAL_MEM_FENCE); 													\
	}

#else

	#define reconstruct_dd_xxx(	p_flag, p_fluid_fraction,								\
								p_dd0, p_neighbor_flag0, p_ff0,							\
								p_dd1, p_neighbor_flag1, p_ff1,							\
								p_old_velocity_x, p_dd_param, p_rho,					\
								p_dda, p_ddb											\
			)																			\
	{																					\
		if (p_flag != FLAG_INTERFACE)													\
		{																				\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
		}																				\
		else if ((p_neighbor_flag0 & FLAG_GAS) && (p_neighbor_flag1 & FLAG_GAS))		\
		{																				\
			/* both cells are gas or obstacle cells */									\
			/* => use equilibrium distribution */										\
			vela2 = p_old_velocity_x*p_old_velocity_x;									\
			p_dd0 = eq_##p_dda(p_old_velocity_x, vela2, p_dd_param, p_rho);				\
			p_dd1 = eq_##p_ddb(p_old_velocity_x, vela2, p_dd_param, p_rho);				\
			p_ff0 = 0.0f;																\
			p_ff1 = 0.0f;																\
		}																				\
		else if (p_neighbor_flag0 & FLAG_GAS)											\
		{																				\
			/* only cell 0 is gas cell */												\
			vela2 = p_old_velocity_x*p_old_velocity_x;									\
			p_dd0 = eq_##p_dda##_plus_##p_ddb(p_old_velocity_x, vela2, p_dd_param, p_rho) - p_dd1;		\
			p_ff0 = 0.0f;																\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
		}																				\
		else if (p_neighbor_flag1 & FLAG_GAS)											\
		{																				\
			/* only cell 1 is gas cell */												\
			vela2 = p_old_velocity_x*p_old_velocity_x;									\
			p_dd1 = eq_##p_dda##_plus_##p_ddb(p_old_velocity_x, vela2, p_dd_param, p_rho) - p_dd0;	\
			p_ff1 = 0.0f;																\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
		}																				\
		else																			\
		{																				\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
			GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
		}																				\
		/* leave disabled for better performance results */								\
		barrier(CLK_LOCAL_MEM_FENCE); 													\
	}

#endif

#define	reconstruct_dd_01(	p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho						\
			)																		\
		reconstruct_dd_xxx(	p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho,					\
							dd0, dd1												\
		);


#define	reconstruct_dd_45(	p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho						\
			)																		\
		reconstruct_dd_xxx(	p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho,					\
							dd4, dd5												\
		);

#else

/********************************************************************
 * TEST VERSION
 *
 * computations using linear interpolation
 ********************************************************************/

#define reconstruct_dd_xxx(	p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho,					\
							p_dda, p_ddb											\
		)																			\
{																					\
	float aa0 = (float)(p_flag == FLAG_INTERFACE);	\
	float aa1 = ((p_neighbor_flag0 & FLAGS_GAS_OBSTACLE) && (p_neighbor_flag1 & FLAGS_GAS_OBSTACLE));	\
	float aa2 = ((p_neighbor_flag0 & FLAGS_GAS_OBSTACLE) > 0);	\
	float aa3 = ((p_neighbor_flag1 & FLAGS_GAS_OBSTACLE) > 0);	\
	vela2 = p_old_velocity_x*p_old_velocity_x;									\
	GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff0, p_neighbor_flag0);	\
	GET_MX_FACTOR_INCOMING(p_fluid_fraction, p_flag, p_ff1, p_neighbor_flag1);	\
	p_ff0 *= (1.0-aa2);		\
	p_ff1 *= (1.0-aa3);		\
	if (p_flag == FLAG_INTERFACE)													\
	{																				\
		if (aa1 > 0.0)			\
		{																				\
			/* both cells are gas or obstacle cells */									\
			/* => use equilibrium distribution */										\
			p_dd0 = eq_##p_dda(p_old_velocity_x, vela2, p_dd_param, p_rho);				\
			p_dd1 = eq_##p_ddb(p_old_velocity_x, vela2, p_dd_param, p_rho);				\
		}																				\
		else if (aa2 > 0.0)									\
		{																				\
			/* only cell 0 is gas cell */												\
			p_dd0 = eq_##p_dda##_plus_##p_ddb(p_old_velocity_x, vela2, p_dd_param, p_rho) - p_dd1;	\
		}																				\
		else if (aa3 > 0.0)																			\
		{																				\
			/* only cell 1 is gas cell */												\
			p_dd1 = eq_##p_dda##_plus_##p_ddb(p_old_velocity_x, vela2, p_dd_param, p_rho) - p_dd0;	\
		}																				\
	}																					\
	barrier(CLK_LOCAL_MEM_FENCE);														\
}


#define	reconstruct_dd_01(	p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho						\
			)																		\
		reconstruct_dd_xxx(	p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho,					\
							dd0, dd1												\
		);


#define	reconstruct_dd_45(	p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho						\
			)																		\
		reconstruct_dd_xxx(	p_flag, p_fluid_fraction,								\
							p_dd0, p_neighbor_flag0, p_ff0,							\
							p_dd1, p_neighbor_flag1, p_ff1,							\
							p_old_velocity_x, p_dd_param, p_rho,					\
							dd4, dd5												\
		);

#endif
