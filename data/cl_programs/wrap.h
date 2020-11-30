/**
 * \in	DOMAIN_CELLS	cell counter within domain
 */
#ifndef DOMAIN_WRAP_CLH
#define DOMAIN_WRAP_CLH

#define CHECK_POW2(A, SHIFT)	((A) == (1 << (SHIFT)))


/**********************************
 * check for POW2 domain size
 **********************************/

#if CHECK_POW2(DOMAIN_CELLS, 0)
	#define DOMAIN_CELLS_POW2	(0)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 1)
	#define DOMAIN_CELLS_POW2	(1)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 2)
	#define DOMAIN_CELLS_POW2	(2)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 3)
	#define DOMAIN_CELLS_POW2	(3)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 4)
	#define DOMAIN_CELLS_POW2	(4)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 5)
	#define DOMAIN_CELLS_POW2	(5)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 6)
	#define DOMAIN_CELLS_POW2	(6)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 7)
	#define DOMAIN_CELLS_POW2	(7)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 8)
	#define DOMAIN_CELLS_POW2	(8)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 9)
	#define DOMAIN_CELLS_POW2	(9)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 10)
	#define DOMAIN_CELLS_POW2	(10)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 11)
	#define DOMAIN_CELLS_POW2	(11)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 12)
	#define DOMAIN_CELLS_POW2	(12)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 13)
	#define DOMAIN_CELLS_POW2	(13)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 14)
	#define DOMAIN_CELLS_POW2	(14)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 15)
	#define DOMAIN_CELLS_POW2	(15)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 16)
	#define DOMAIN_CELLS_POW2	(16)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 17)
	#define DOMAIN_CELLS_POW2	(17)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 18)
	#define DOMAIN_CELLS_POW2	(18)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 19)
	#define DOMAIN_CELLS_POW2	(19)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 20)
	#define DOMAIN_CELLS_POW2	(20)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 21)
	#define DOMAIN_CELLS_POW2	(21)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 22)
	#define DOMAIN_CELLS_POW2	(22)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 23)
	#define DOMAIN_CELLS_POW2	(23)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 24)
	#define DOMAIN_CELLS_POW2	(24)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 25)
	#define DOMAIN_CELLS_POW2	(25)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 26)
	#define DOMAIN_CELLS_POW2	(26)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 27)
	#define DOMAIN_CELLS_POW2	(27)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 28)
	#define DOMAIN_CELLS_POW2	(28)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 29)
	#define DOMAIN_CELLS_POW2	(29)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 30)
	#define DOMAIN_CELLS_POW2	(30)
#endif
#if CHECK_POW2(DOMAIN_CELLS, 31)
	#define DOMAIN_CELLS_POW2	(31)
#endif


#ifdef DOMAIN_CELLS_POW2
	/**
	 * it's REALLY IMPORTANT (!!!) that A is a positive number!!!
	 */
	inline int DOMAIN_WRAP(size_t A)
	{
		return (A) & (DOMAIN_CELLS-1);
	}
#else
//	#warning "domain size has to be a power of 2 to gain a speedup"
//	#error "domain size has to be a power of 2 for a speedup using bit operations!!!"

	inline int DOMAIN_WRAP(size_t A)
	{
		return (A) % (DOMAIN_CELLS);
	}
#endif



/**********************************
 * check for POW2 thread size
 **********************************/

#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 0)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(0)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 1)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(1)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 2)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(2)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 3)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(3)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 4)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(4)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 5)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(5)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 6)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(6)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 7)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(7)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 8)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(8)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 9)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(9)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 10)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(10)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 11)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(11)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 12)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(12)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 13)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(13)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 14)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(14)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 15)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(15)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 16)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(16)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 17)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(17)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 18)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(18)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 19)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(19)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 20)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(20)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 21)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(21)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 22)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(22)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 23)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(23)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 24)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(24)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 25)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(25)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 26)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(26)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 27)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(27)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 28)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(28)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 29)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(29)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 30)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(30)
#endif
#if CHECK_POW2(LOCAL_WORK_GROUP_SIZE, 31)
	#define LOCAL_WORK_GROUP_SIZE_POW2	(31)
#endif


#ifdef LOCAL_WORK_GROUP_SIZE_POW2
	/**
	 * it's REALLY IMPORTANT (!!!) that A is a positive number!!!
	 */
	inline size_t LOCAL_WORK_GROUP_WRAP(size_t A)
	{
		return (A) & (LOCAL_WORK_GROUP_SIZE-1);
	}
#else
//	#warning "domain size has to be a power of 2 to gain a speedup"
//	#error "domain size has to be a power of 2 to gain a speedup"

	inline size_t LOCAL_WORK_GROUP_WRAP(size_t A)
	{
		return (A) % (LOCAL_WORK_GROUP_SIZE);
	}
#endif



#endif
