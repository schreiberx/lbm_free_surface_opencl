/*
 * lbm_collision_operator.h
 *
 *  Created on: Jan 1, 2010
 *      Author: martin
 */

#ifndef LBM_COLLISION_OPERATOR_CLH_
#define LBM_COLLISION_OPERATOR_CLH_

#if LIMIT_VELOCITY
	T a = velocity_x*velocity_x + velocity_y*velocity_y + velocity_z*velocity_z;
	if (a > LIMIT_VELOCITY_SPEED*LIMIT_VELOCITY_SPEED)
	{
		a = LIMIT_VELOCITY_SPEED/sqrt(a);
		velocity_x *= a;
		velocity_y *= a;
		velocity_z *= a;
	}
	barrier(CLK_LOCAL_MEM_FENCE);
#endif

if (flag & (FLAG_INTERFACE | FLAG_FLUID))
{
	vel2 = velocity_x*velocity_x + velocity_y*velocity_y + velocity_z*velocity_z;
#if COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION
	dd_param = (T)(3.0/2.0)*(vel2);
#else
	dd_param = rho - (T)(3.0f/2.0f)*(vel2);
#endif

#define vela_velb_2	vela2

#if TWO_RELAXATION_TIME_COLLISION
	T eq_a, eq_b;
	T eq_even, eq_odd;
	T dd_even, dd_odd;

	/***********************
	 * DD0
	 ***********************/
	vela2 = velocity_x*velocity_x;
	eq_a = eq_dd0(velocity_x, vela2, dd_param, rho);
	eq_b = eq_dd1(velocity_x, vela2, dd_param, rho);
	eq_even = eq_a + eq_b;	eq_odd = eq_a - eq_b;
	dd_even = dd0 + dd1;	dd_odd = dd0 - dd1;
	dd0 += (T)0.5f * (inv_tau*(eq_even - dd_even) + inv_trt_tau*(eq_odd - dd_odd));
	dd1 += (T)0.5f * (inv_tau*(eq_even - dd_even) - inv_trt_tau*(eq_odd - dd_odd));

	vela2 = velocity_y*velocity_y;
	eq_a = eq_dd0(velocity_y, vela2, dd_param, rho);
	eq_b = eq_dd1(velocity_y, vela2, dd_param, rho);
	eq_even = eq_a + eq_b;	eq_odd = eq_a - eq_b;
	dd_even = dd2 + dd3;	dd_odd = dd2 - dd3;
	dd2 += (T)0.5f * (inv_tau*(eq_even - dd_even) + inv_trt_tau*(eq_odd - dd_odd));
	dd3 += (T)0.5f * (inv_tau*(eq_even - dd_even) - inv_trt_tau*(eq_odd - dd_odd));

	/***********************
	 * DD1
	 ***********************/
	vela_velb = velocity_x+velocity_y;
	vela_velb_2 = vela_velb*vela_velb;
	eq_a = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	eq_b = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	eq_even = eq_a + eq_b;	eq_odd = eq_a - eq_b;
	dd_even = dd4 + dd5;	dd_odd = dd4 - dd5;
	dd4 += (T)0.5f * (inv_tau*(eq_even - dd_even) + inv_trt_tau*(eq_odd - dd_odd));
	dd5 += (T)0.5f * (inv_tau*(eq_even - dd_even) - inv_trt_tau*(eq_odd - dd_odd));

	vela_velb = velocity_x-velocity_y;
	vela_velb_2 = vela_velb*vela_velb;
	eq_a = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	eq_b = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	eq_even = eq_a + eq_b;	eq_odd = eq_a - eq_b;
	dd_even = dd6 + dd7;	dd_odd = dd6 - dd7;
	dd6 += (T)0.5f * (inv_tau*(eq_even - dd_even) + inv_trt_tau*(eq_odd - dd_odd));
	dd7 += (T)0.5f * (inv_tau*(eq_even - dd_even) - inv_trt_tau*(eq_odd - dd_odd));

	/***********************
	 * DD2
	 ***********************/
	vela_velb = velocity_x+velocity_z;
	vela_velb_2 = vela_velb*vela_velb;
	eq_a = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	eq_b = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	eq_even = eq_a + eq_b;	eq_odd = eq_a - eq_b;
	dd_even = dd8 + dd9;	dd_odd = dd8 - dd9;
	dd8 += (T)0.5f * (inv_tau*(eq_even - dd_even) + inv_trt_tau*(eq_odd - dd_odd));
	dd9 += (T)0.5f * (inv_tau*(eq_even - dd_even) - inv_trt_tau*(eq_odd - dd_odd));

	vela_velb = velocity_x-velocity_z;
	vela_velb_2 = vela_velb*vela_velb;
	eq_a = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	eq_b = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	eq_even = eq_a + eq_b;	eq_odd = eq_a - eq_b;
	dd_even = dd10 + dd11;	dd_odd = dd10 - dd11;
	dd10 += (T)0.5f * (inv_tau*(eq_even - dd_even) + inv_trt_tau*(eq_odd - dd_odd));
	dd11 += (T)0.5f * (inv_tau*(eq_even - dd_even) - inv_trt_tau*(eq_odd - dd_odd));

	/***********************
	 * DD3
	 ***********************/
	vela_velb = velocity_y+velocity_z;
	vela_velb_2 = vela_velb*vela_velb;
	eq_a = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	eq_b = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	eq_even = eq_a + eq_b;	eq_odd = eq_a - eq_b;
	dd_even = dd12 + dd13;	dd_odd = dd12 - dd13;
	dd12 += (T)0.5f * (inv_tau*(eq_even - dd_even) + inv_trt_tau*(eq_odd - dd_odd));
	dd13 += (T)0.5f * (inv_tau*(eq_even - dd_even) - inv_trt_tau*(eq_odd - dd_odd));

	vela_velb = velocity_y-velocity_z;
	vela_velb_2 = vela_velb*vela_velb;
	eq_a = eq_dd4(vela_velb, vela_velb_2, dd_param, rho);
	eq_b = eq_dd5(vela_velb, vela_velb_2, dd_param, rho);
	eq_even = eq_a + eq_b;	eq_odd = eq_a - eq_b;
	dd_even = dd14 + dd15;	dd_odd = dd14 - dd15;
	dd14 += (T)0.5f * (inv_tau*(eq_even - dd_even) + inv_trt_tau*(eq_odd - dd_odd));
	dd15 += (T)0.5f * (inv_tau*(eq_even - dd_even) - inv_trt_tau*(eq_odd - dd_odd));

	/***********************
	 * DD4
	 ***********************/
	vela2 = velocity_z*velocity_z;
	eq_a = eq_dd0(velocity_z, vela2, dd_param, rho);
	eq_b = eq_dd1(velocity_z, vela2, dd_param, rho);
	eq_even = eq_a + eq_b;	eq_odd = eq_a - eq_b;
	dd_even = dd16 + dd17;	dd_odd = dd16 - dd17;
	dd16 += (T)0.5f * (inv_tau*(eq_even - dd_even) + inv_trt_tau*(eq_odd - dd_odd));
	dd17 += (T)0.5f * (inv_tau*(eq_even - dd_even) - inv_trt_tau*(eq_odd - dd_odd));

	dd18 += inv_tau*(eq_dd18(dd_param, rho) - dd18);

#else

	/***********************
	 * DD0
	 ***********************/
	vela2 = velocity_x*velocity_x;
	dd0 += inv_tau*(eq_dd0(velocity_x, vela2, dd_param, rho) - dd0);
	dd1 += inv_tau*(eq_dd1(velocity_x, vela2, dd_param, rho) - dd1);

	vela2 = velocity_y*velocity_y;
	dd2 += inv_tau*(eq_dd0(velocity_y, vela2, dd_param, rho) - dd2);
	dd3 += inv_tau*(eq_dd1(velocity_y, vela2, dd_param, rho) - dd3);

	/***********************
	 * DD1
	 ***********************/
	vela_velb = velocity_x+velocity_y;
	vela_velb_2 = vela_velb*vela_velb;
	dd4 += inv_tau*(eq_dd4(vela_velb, vela_velb_2, dd_param, rho) - dd4);
	dd5 += inv_tau*(eq_dd5(vela_velb, vela_velb_2, dd_param, rho) - dd5);

	vela_velb = velocity_x-velocity_y;
	vela_velb_2 = vela_velb*vela_velb;
	dd6 += inv_tau*(eq_dd4(vela_velb, vela_velb_2, dd_param, rho) - dd6);
	dd7 += inv_tau*(eq_dd5(vela_velb, vela_velb_2, dd_param, rho) - dd7);

	/***********************
	 * DD2
	 ***********************/
	vela_velb = velocity_x+velocity_z;
	vela_velb_2 = vela_velb*vela_velb;
	dd8 += inv_tau*(eq_dd4(vela_velb, vela_velb_2, dd_param, rho) - dd8);
	dd9 += inv_tau*(eq_dd5(vela_velb, vela_velb_2, dd_param, rho) - dd9);

	vela_velb = velocity_x-velocity_z;
	vela_velb_2 = vela_velb*vela_velb;
	dd10 += inv_tau*(eq_dd4(vela_velb, vela_velb_2, dd_param, rho) - dd10);
	dd11 += inv_tau*(eq_dd5(vela_velb, vela_velb_2, dd_param, rho) - dd11);

	/***********************
	 * DD3
	 ***********************/
	vela_velb = velocity_y+velocity_z;
	vela_velb_2 = vela_velb*vela_velb;
	dd12 += inv_tau*(eq_dd4(vela_velb, vela_velb_2, dd_param, rho) - dd12);
	dd13 += inv_tau*(eq_dd5(vela_velb, vela_velb_2, dd_param, rho) - dd13);

	vela_velb = velocity_y-velocity_z;
	vela_velb_2 = vela_velb*vela_velb;
	dd14 += inv_tau*(eq_dd4(vela_velb, vela_velb_2, dd_param, rho) - dd14);
	dd15 += inv_tau*(eq_dd5(vela_velb, vela_velb_2, dd_param, rho) - dd15);

	/***********************
	 * DD4
	 ***********************/
	vela2 = velocity_z*velocity_z;
	dd16 += inv_tau*(eq_dd0(velocity_z, vela2, dd_param, rho) - dd16);
	dd17 += inv_tau*(eq_dd1(velocity_z, vela2, dd_param, rho) - dd17);

	dd18 += inv_tau*(eq_dd18(dd_param, rho) - dd18);
#endif
#undef vela_velb_2

#if GRAVITATION
	/*
	 * only computate gravitation (and therefore modify the density distribution values)
	 * for fluid and interface cells
	 */
	if (flag == FLAG_OBSTACLE)
		rho = 1.0f;

	// 8*(1/36) + 2*(1/18)
#if 1
	velocity_x += (T)(1.0f/3.0f)*gravitation0;
	velocity_y += (T)(1.0f/3.0f)*gravitation1;
	velocity_z += (T)(1.0f/3.0f)*gravitation2;
#endif
	T rho_ff = rho*fluid_fraction;
	tmp = gravitation0*(T)(1.0f/18.0f)*rho_ff;
	dd0 += tmp;
	dd1 -= tmp;
	tmp = gravitation1*(T)(1.0f/18.0f)*rho_ff;
	dd2 += tmp;
	dd3 -= tmp;

	tmp = (gravitation0 + gravitation1)*(T)(1.0f/36.0f)*rho_ff;
	dd4 += tmp;
	dd5 -= tmp;
	tmp = (gravitation0 - gravitation1)*(T)(1.0f/36.0f)*rho_ff;
	dd6 += tmp;
	dd7 -= tmp;

	tmp = (gravitation0 + gravitation2)*(T)(1.0f/36.0f)*rho_ff;
	dd8 += tmp;
	dd9 -= tmp;
	tmp = (gravitation0 - gravitation2)*(T)(1.0f/36.0f)*rho_ff;
	dd10 += tmp;
	dd11 -= tmp;

	tmp = (gravitation1 + gravitation2)*(T)(1.0f/36.0f)*rho_ff;
	dd12 += tmp;
	dd13 -= tmp;
	tmp = (gravitation1 - gravitation2)*(T)(1.0f/36.0f)*rho_ff;
	dd14 += tmp;
	dd15 -= tmp;

	tmp = gravitation2*(T)(1.0f/18.0f)*rho_ff;
	dd16 += tmp;
	dd17 -= tmp;
#endif
}
else if (flag == FLAG_OBSTACLE)
{
	// in case of an obstacle, we bounce back the values
	// we dont change the fluid mass on obstacles
	// set to zero velocity and no fluid density
	/*
	velocity_x = 0.0;
	velocity_y = 0.0;
	velocity_z = 0.0;
	*/

	// use simple bounce back
	vela2 = dd1;	dd1 = dd0;		dd0 = vela2;
	vela2 = dd3;	dd3 = dd2;		dd2 = vela2;
	vela2 = dd5;	dd5 = dd4;		dd4 = vela2;
	vela2 = dd7;	dd7 = dd6;		dd6 = vela2;
	vela2 = dd9;	dd9 = dd8;		dd8 = vela2;
	vela2 = dd11;	dd11 = dd10;	dd10 = vela2;
	vela2 = dd13;	dd13 = dd12;	dd12 = vela2;
	vela2 = dd15;	dd15 = dd14;	dd14 = vela2;
	vela2 = dd17;	dd17 = dd16;	dd16 = vela2;
}

#endif /* LBM_COLLISION_OPERATOR_CLH_ */
