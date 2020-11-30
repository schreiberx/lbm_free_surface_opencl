/*
 * Copyright 2010 Martin Schreiber
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CLBMSKELETON_HPP
#define CLBMSKELETON_HPP

/**
 *
 * \brief Lattice Boltzmann parameter handler and parametrization class
 * \author Martin Schreiber
 * \version 0.1
 * \date 2009-05-20
 *
 */

#include "libmath/CMath.hpp"
#include "libmath/CVector.hpp"
#include "lib/CError.hpp"

/**
 * \brief skeleton for lattice boltzmann simulation to handle parameters and to do the parametrization
 */
template <typename T>
class CLbmParameters
{
public:
	CError error;		///< error hanlder
	bool debug;			///< true, if debug mode is on

	CVector<3,int> domain_cells;	///< available simulation cells in each dimension
	int domain_cells_count;			///< overall number of simulation cells

//protected:
	// input values
	T d_domain_x_length;		/**< domain length in x direction (meters)
								 * the length in y and z direction can be computated by domain_cells because the cells have unit lengths
								 * maybe it's possible to specify the domain length in each dimension by a modification of the
								 * equilibrium and collision operators
								 * (with dimension)
								 */

	CVector<3,T> d_gravitation;	///< gravitation vector (with dimension)
	T d_viscosity;				///< viscosity of fluid (with dimension)
	T mass_exchange_factor;		///< accelleration for mass exchange

	// computed values
	T d_cell_length;			///< length of cell
	T d_timestep;				///< timestep
	bool compute_timestep;		///< true, if the timestep should be computes

	// simulation values (parametrized dimensionless)
	T viscosity;				///< parametrized viscosity of fluid
	T tau;						///< dimensionless tau for collision operator
	T inv_tau;					///< inverse tau
	T inv_trt_tau;				///< inverse two time relaxation model tau

	CVector<3,T> gravitation;	///< gravitation vector

	T max_sim_gravitation_length;	///< maximum length of dimensionless gravitation vector to restrict maximum force acting on fluid

	T max_sim_gravitation_length_scaled;	///< maximum length of dimensionless gravitation vector scaled by inverse of domain_size in x direction

	/**
	 * update the values for lbm simulation (parametrization)
	 */
#if 0
	// WITH MASS EXCHANGE FACTOR
	void updateValues(bool info_output = false)
	{
		if (d_timestep == -1.0)
			d_timestep = (d_cell_length*d_cell_length)*((T)2.0*tau - (T)1.0)/((T)6.0 * d_viscosity * CMath<T>::sqrt(mass_exchange_factor));

		gravitation = d_gravitation*((d_timestep*d_timestep)/d_cell_length);
/*
		std::cout << "=================================" << std::endl;
		std::cout << "tau: " << tau << std::endl;
		std::cout << "gravitation: " << d_gravitation << std::endl;
		std::cout << "=================================" << std::endl;
*/
		/*
		 * limit the gravitation parameter for the simulation
		 * to avoid large velocities and thus an unstable simulation
		 */

		if (gravitation.getLength() >= max_sim_gravitation_length)
		{
			if (info_output)
				std::cout << "limiting timestep (gravitation: " << gravitation << ", gravitation length " << gravitation.getLength() << " >= " << max_sim_gravitation_length << ")" << std::endl;

			d_timestep = CMath<T>::sqrt((max_sim_gravitation_length * d_cell_length) / d_gravitation.getLength());
			gravitation = d_gravitation*((d_timestep*d_timestep)/d_cell_length);
			tau = (T)0.5*(d_timestep*d_viscosity*CMath<T>::sqrt(mass_exchange_factor)*(T)6.0)/(d_cell_length*d_cell_length)+(T)0.5;
		}
		else
		{
			if (info_output)
				std::cout << "using computed timestep" << std::endl;
		}

		if (d_viscosity != -1.0)
		{
			viscosity = (d_viscosity*d_timestep) / (d_cell_length*d_cell_length);
			tau = (6.0*viscosity + 1.0)*0.5;
		}
/*
		float tau_min = 0.505;
		float tau_max = 2.0;
		if (tau < tau_min || tau > tau_max)
		{
			error << "tau has to be within the boundary [" << tau_min << "; " << tau_max << "]" << std::endl;
			error << "otherwise the simulation becomes unstable! current value: " << tau << std::endl;
		}
*/
		inv_tau = (T)1.0/tau;
		inv_trt_tau = (T)1.0/((T)0.5 + (T)3.0/((T)16.0*tau - (T)8.0));

		// multiply the timestep by mass exchange factor
		d_timestep *= CMath<float>::sqrt(mass_exchange_factor);
	}
#else
	void computeParametrization(bool info_output = false)
	{
		if (compute_timestep)
		{
			T d_gravitation_length = d_gravitation.getLength();

			if (d_gravitation_length < 0.000000000001)
			{
				// handle zero gravitation with small timestep
				d_timestep = 0.01;
			}
			else
			{
				d_timestep = CMath::sqrt<T>((max_sim_gravitation_length_scaled * d_cell_length) / d_gravitation_length);
			}
		}
		else
		{
			gravitation = d_gravitation*((d_timestep*d_timestep)/d_cell_length);

			/*
			 * limit the gravitation parameter for the simulation
			 * to avoid large velocities and thus an unstable simulation
			 */

//			std::cout << gravitation.getLength() << std::endl;
//			std::cout << max_sim_gravitation_length_scaled << std::endl;
			if (gravitation.getLength() >= max_sim_gravitation_length_scaled)
			{
				if (info_output)
					std::cout << "LBM Skeleton INFO: limiting timestep (gravitation: " << gravitation << ", gravitation length " << gravitation.getLength() << " >= " << max_sim_gravitation_length_scaled << ")" << std::endl;

				d_timestep = CMath::sqrt<T>((max_sim_gravitation_length_scaled * d_cell_length) / d_gravitation.getLength());
			}
		}

		gravitation = d_gravitation*((d_timestep*d_timestep)/d_cell_length);
		viscosity = (d_viscosity*d_timestep)/(d_cell_length*d_cell_length);
		tau = viscosity*(T)3.0+(T)0.5;
		inv_tau = (T)1.0/tau;
/*
		float inv_tau_min = 0.01;
		float inv_tau_max = 1.9;

		if (inv_tau < inv_tau_min || inv_tau > inv_tau_max)
			error << "inv_tau (" << inv_tau << ") has to be within the boundary [" << inv_tau_min << "; " << inv_tau_max << "]" << CError::endl;
*/
		inv_trt_tau = (T)1.0/((T)0.5 + (T)3.0/((T)16.0*tau - (T)8.0));

		d_timestep *= mass_exchange_factor;
	}
#endif

	/**
	 * set new gravitation force
	 *
	 * after this function, computeParametrization() has to be called!
	 */
	void setGravitation(CVector<3,T> p_d_gravitation)
	{
		d_gravitation = p_d_gravitation;
	}

	/**
	 * set new viscosity
	 */
	void setViscosity(T p_d_viscosity)
	{
		d_viscosity = p_d_viscosity;
	}

	/**
	 * set mass exchange factors
	 */
	void setMassExchangeFactor(T p_mass_exchange_factor)
	{
		mass_exchange_factor = p_mass_exchange_factor;
	}


	/**
	 * set new domain size
	 */
	void setDomainXLength(T p_d_domain_x_length)
	{
		d_domain_x_length = p_d_domain_x_length;

		// compute sidelength of a lattice cell
		d_cell_length = d_domain_x_length / (T)domain_cells[0];
	}

	/**
	 * initialize skeleton
	 */
	void initSkeleton(	CVector<3,int> &p_domain_cells,			///< domain cells in each dimension
						T p_d_domain_x_length,					///< domain length in x direction
						T p_d_viscosity,						///< viscocity of fluid
						CVector<3,T> &p_d_gravitation,			///< gravitation
						T p_max_sim_gravitation_length,			///< maximum length of gravitation to limit timestep
						T p_d_timestep = -1,					///< timestep for one simulation step - computed automagically
						T p_mass_exchange_factor = 1.0			///< mass exchange
	)
	{
		domain_cells = p_domain_cells;
		d_domain_x_length = p_d_domain_x_length;

		d_gravitation = p_d_gravitation;

		domain_cells_count = domain_cells.elements();

		d_viscosity = p_d_viscosity;
		mass_exchange_factor = p_mass_exchange_factor;

		// compute sidelength of a lattice cell
		d_cell_length = d_domain_x_length / (T)p_domain_cells[0];

		d_timestep = p_d_timestep;
		compute_timestep = (p_d_timestep < 0);

		max_sim_gravitation_length = p_max_sim_gravitation_length;
		max_sim_gravitation_length_scaled = max_sim_gravitation_length/(T)domain_cells.max();

		computeParametrization(true);

		if (debug)
		{
			std::cout << "-------------------------------------------" << std::endl;
			std::cout << "dim domain cells: " << domain_cells << std::endl;
			std::cout << "dim domain cells max: " << domain_cells.max() << std::endl;
			std::cout << "dim domain x length: " << d_domain_x_length << std::endl;
			std::cout << "dim cell length: " << d_cell_length << std::endl;
			std::cout << "mass exchange factor: " << mass_exchange_factor << std::endl;
			std::cout << "max_sim_gravitation_length: " << max_sim_gravitation_length << std::endl;
			std::cout << "max_sim_gravitation_length_scaled: " << max_sim_gravitation_length_scaled << std::endl;
			std::cout << "-------------------------------------------" << std::endl;
			std::cout << "dim viscocity: " << d_viscosity << std::endl;
			std::cout << "dim gravitation: " << d_gravitation << std::endl;
			std::cout << "dim timestep: " << d_timestep << std::endl;

			std::cout << "-------------------------------------------" << std::endl;

//			std::cout << "viscocity: " << viscosity << std::endl;
			std::cout << "tau: " << tau << std::endl;
			std::cout << "inv_trt_tau: " << inv_trt_tau << std::endl;
			std::cout << "inv_tau: " << inv_tau << std::endl;
			std::cout << "gravitation: " << gravitation << std::endl;
			std::cout << "-------------------------------------------" << std::endl;
		}
	}

	/**
	 * default constructor: initialize only debug variable, use init(...) for further initialization
	 */
	CLbmParameters(bool p_debug = false)
	{
		debug = p_debug;
	}
};

#endif
