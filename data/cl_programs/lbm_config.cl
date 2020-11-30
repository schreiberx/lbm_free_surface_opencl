/*
 * ACTIVATE INCREASED MASS EXCHANGE
 */
#define ACTIVATE_INCREASED_MASS_EXCHANGE	1


/*
 * Gravitation active
 */
#define GRAVITATION			1

/*
 * allow interface cells to change their state
 */
#define INTERFACE_CHANGE	1

/*
 * use two relaxation time collision operator
 */
#define TWO_RELAXATION_TIME_COLLISION	0

/*
 * use the compressible or incompressible equilibrium distribution function
 * this option may not be activated with TWO_RELAXATION_TIME_COLLISION !!!
 */
#define COMPRESSIBLE_EQUILIBRIUM_DISTRIBUTION	1

/*
 * remove interfaces not surrounded by any fluid cell
 *
 * TODO: there are still unresolved problems: the interface remover is !!!!! NOT DETERMINISTIC !!!!!
 * but this does not influence the requirement of a gapless interface
 *
 * enabling this option is necessary e.g. for falling drop simulation. otherwise some loneley
 * interface cells reside in the fluid
 */
// !!!!! NOT DETERMINISTIC !!!!!
#define LONELEY_INTERFACE_REMOVER		1

/*
 * spread missing mass of new gas cells to neighbored fluid and interface cells
 */
// !!!!! NOT DETERMINISTIC !!!!!
#define DISTRIBUTE_MASS_OF_GAS_CELLS	0

#if ACTIVATE_INCREASED_MASS_EXCHANGE
	// TODO: EXTRA_OFFSET has to be set with respect to MASS_EXCHANGE_FACTOR
	//#define EXTRA_OFFSET	min(0.01, 0.001*MASS_EXCHANGE_FACTOR)
	//#define EXTRA_OFFSET	0.001
	#define EXTRA_OFFSET	(mass_exchange_factor*0.001f)
#else
	#define EXTRA_OFFSET	(0.001f)
#endif

/*
 * limit velocity - set to 0 to disable
 */
#define LIMIT_VELOCITY	1
#define LIMIT_VELOCITY_SPEED	0.25f

/*
 * This feature has been implemented only for A-B pattern:
 *
 * For A-B Pattern, version 1, it is possible to use the density distribution values
 * streamed from obstacle cells to interface cells by initializing the according dds of
 * obstacle cells when a gas cell is converted to an interface cell
 */
#define DD_FROM_OBSTACLES_TO_INTERFACE	0

/*
 * debug option to initialize the gas with large velocity values.
 * if such a velocity value is streamed to a fluid, something wrong happened...
 */
#define SETUP_LARGE_VELOCITY	1

#define GAS_PRESSURE	1.0f
