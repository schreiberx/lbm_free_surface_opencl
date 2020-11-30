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

/*
 * a small math lib with some useful functions and a c++ abstraction layer
 */
#ifndef CMATH_HPP__
#define CMATH_HPP__

#include <iostream>
#include <limits>

#ifdef _MATH_H
#error "math.h already included!"
#endif

	#include <math.h>

#include <stdlib.h>

#ifndef fabsf
#define	m_fabsf(x)	(fabs(x))
#else
#define	m_fabsf(x)	(fabsf(x))
#endif

#define	m_fabs(x)	(fabs(x))

#ifndef floorf
#define	m_floorf(x)	(floor(x))
#else
#define	m_floorf(x)	(floorf(x))
#endif

#ifndef ceilf
#define	m_ceilf(x)	(ceil(x))
#else
#define	m_ceilf(x)	(ceilf(x))
#endif

#ifndef sqrtf
#define	m_sqrtf(x)	(sqrtf(x))
#else
#define	m_sqrtf(x)	(sqrtf(x))
#endif

#ifndef sqrtl
#define	m_sqrtl(x)	((unsigned long)sqrt((double)x))
#else
#define	m_sqrtl(x)	((unsigned long)sqrtl((unsigned long)x))
#endif


#ifndef roundf
#define m_roundf(x)	((float)(int)(x+0.5f))
#else
#define m_roundf(x)	(roundf(x))
#endif

#ifndef round
#define m_round(x)	((double)(int)(x+0.5))
#else
#define m_round(x)	(round(x))
#endif



/**
 * \brief	math handler to use same function names for different types
 */
class CMath
{
public:
	/// return PI
	template <typename T>
	static inline T PI();

	/// return absolute value
	template <typename T>
	static inline T abs(const T& a);
	/// return power of value
	template <typename T>
	static inline T pow(const T& base, const T& exp);
	/// return floor value
	template <typename T>
	static inline T floor(const T& a);
	/// return ceil value
	template <typename T>
	static inline T ceil(const T& a);

	/// return ceiled value in binary system
	template <typename T>
	static inline T ceil2(const T& a);
	/// return sqrt value
	template <typename T>
	static inline T sqrt(const T& a);

	template <typename T>
	static inline T sqrt2();	// sqrt(2)

	template <typename T>
	static inline T sqrt1_2();	// 1/sqrt(2)

	/// return rounded value
	template <typename T>
	static inline T round(const T& a);

	/// return digits of a in binary system
	template <typename T>
	static inline T digits2(const T& a);

	/// return exp value
	template <typename T>
	static inline T exp(const T& a);
	/// return log value
	template <typename T>
	static inline T log(const T& a);

	/// return sin value
	template <typename T>
	static inline T sin(const T& a);
	/// return cos value
	template <typename T>
	static inline T cos(const T& a);
	/// return tan value
	template <typename T>
	static inline T tan(const T& a);

	/// return sin value
	template <typename T>
	static inline T asin(const T& a);
	/// return cos value
	template <typename T>
	static inline T acos(const T& a);
	/// return tan value
	template <typename T>
	static inline T atan(const T& a);
/*
	/// return max of both values
	static inline T max(T a, T b)		{	return (a < b ? b : a);	}
	/// return min of both values
	static inline T min(T a, T b)		{	return (a > b ? b : a);	}
*/
	/// return value of type T for string s
	template <typename T>
	static inline T aton(const char *s);

	/// return maximum available finite number
	template <typename T>
	static inline T numeric_max();
	/// return minimum available finite number (positive number closest to 0)
	template <typename T>
	static inline T numeric_min();
	/// return the value for infinity
	template <typename T>
	static inline T numeric_inf();

	template <typename T>
	static inline T max(const T& a, const T& b)
	{
		return std::max(a, b);
	}

	template <typename T>
	static inline T min(const T& a, const T& b)
	{
		return std::min(a, b);
	}

	/// return true if the value is nan
	template <typename T>
	static inline bool isNan(const T& a)		{	return a != a;	}

	/**
	 * greatest common divisor
	 * http://en.wikipedia.org/wiki/Euclidean_algorithm
	 */
	template <typename T>
	static inline T gcd(
			const T& a,
			const T& b)
	{
		if (a == 0)	return b;
		while (b != 0)
		{
			if (a > b)
				a -= b;
			else
				b -= a;
		}
		return a;
	}

	/// return the sign of a number (-1 or +1)
	template <typename T>
	static inline T	sign(const T& a)
	{
		return (a < (T)0 ? (T)-1 : (T)+1);
	}
};



/** return PI */
template <>
inline float	CMath::PI<float>()
{
	return M_PI;
}

/** return PI of type double */
template <>	inline double	CMath::PI<double>()				{	return M_PI;		}

/** return absolute value of a */
template <>	inline float	CMath::abs<float>(const float& a)		{	return (float)m_fabsf(a);	}
/** return absolute value of a */
template <>	inline double	CMath::abs<double>(const double& a)	{	return (double)m_fabs(a);	}
/** return absolute value of a */
template <>	inline int		CMath::abs<int>(const int& a)			{	return ::abs(a);	}

/** return the power of 'base' to 'exp' */
template <>	inline float	CMath::pow<float>(const float& base, const float& exp)	{	return powf(base, exp);	}
/** return the power of 'base' to 'exp' */
template <>	inline double	CMath::pow<double>(const double& base, const double& exp)	{	return pow(base, exp);	}
/** return the power of 'base' to 'exp' */
template <>	inline int		CMath::pow<int>(const int& base, const int& exp)			{	return pow<double>((double)base, (double)exp);	}

/** return floored value of a */
template <>	inline float	CMath::floor<float>(const float& a)	{	return (float)m_floorf(a);	}
/** return floored value of a */
template <>	inline double	CMath::floor<double>(const double& a)	{	return (double)floor(a);	}
/** return floored value of a */
template <>	inline int		CMath::floor<int>(const int& a)		{	return a;			}

/** return ceiled value of a */
template <>	inline float	CMath::ceil<float>(const float& a)		{	return (float)m_ceilf(a);	}
/** return ceiled value of a */
template <>	inline double	CMath::ceil<double>(const double& a)	{	return (double)ceil(a);	}
/** return ceiled value of a */
template <>	inline int		CMath::ceil<int>(const int& a)			{	return a;			}


/** return maximum finite value of a */
template <>	inline float	CMath::numeric_max<float>()				{	return std::numeric_limits<float>::max();		}
/** return maximum finite value of a */
template <>	inline double	CMath::numeric_max<double>()			{	return std::numeric_limits<double>::max();		}
/** return maximum finite value of a */
template <>	inline int		CMath::numeric_max<int>()				{	return std::numeric_limits<int>::max();			}

/** return minimum available finite number (positive number closest to 0) */
template <>	inline float	CMath::numeric_min<float>()				{	return std::numeric_limits<float>::min();		}
/** return minimum available finite number (positive number closest to 0) */
template <>	inline double	CMath::numeric_min<double>()			{	return std::numeric_limits<double>::min();		}
/** return minimum available finite number (positive number closest to 0) */
template <>	inline int		CMath::numeric_min<int>()				{	return std::numeric_limits<int>::min();			}

/** return minimum available finite number (positive number closest to 0) */
template <>	inline float	CMath::numeric_inf<float>()				{	return std::numeric_limits<float>::infinity();		}
/** return minimum available finite number (positive number closest to 0) */
template <>	inline double	CMath::numeric_inf<double>()			{	return std::numeric_limits<double>::infinity();		}


/**
 * return ceiled value of a in binary system
 *
 * safety check only valid for unsigned data
 */
template <typename T>
inline T CMath::ceil2(const T& a)
		{
			if (a > ((T)1<<(sizeof(T)*8-2)))	return 0;

			T r = 1;
			while (r < a)
			{
				r <<= 1;
			}
			return r;
		}

/**
 * return digits of a in binary system
 *
 * a must not be larger than 2^30!!!
 */
template <>
inline int CMath::digits2<int>(const int& a)
{
	if (a > 0x40000000)	return 0;

	if (a == 0)		return 0;

	int r = 1;
	int c = 1;

	while (r < a)
	{
		r <<= 1;
		c++;
	}
	return c;
}


/** return square root of a */
template <>	inline float	CMath::sqrt<float>(const float& a)		{	return (float)m_sqrtf(a);	}
/** return square root of a */
template <>	inline double	CMath::sqrt<double>(const double& a)	{	return (double)::sqrt(a);	}
/** return square root of a */
template <>	inline int		CMath::sqrt<int>(const int& a)			{	return m_sqrtl((unsigned long)a);	}


template <>	inline float	CMath::sqrt2<float>()	{	return (float)(M_SQRT2);	}
template <>	inline double	CMath::sqrt2<double>()	{	return (double)(M_SQRT2);	}

template <>	inline float	CMath::sqrt1_2<float>()	{	return (float)(M_SQRT1_2);	}
template <>	inline double	CMath::sqrt1_2<double>()	{	return (double)(M_SQRT1_2);	}




/** return rounded value of a */
template <>	inline float	CMath::round<float>(const float& a)	{	return (float)m_roundf(a);	}
/** return rounded value of a */
template <>	inline double	CMath::round<double>(const double &a)	{	return (double)m_round(a);	}
/** return rounded value of a */
template <>	inline int		CMath::round<int>(const int &a)		{	return a;			}

/** convert string to type float */
template <>	inline float	CMath::aton(const char *s)	{	return (float)::atof(s);	}
/** convert string to type double */
template <>	inline double	CMath::aton(const char *s)	{	return (double)::atof(s);	}
/** convert string to type int */
template <>	inline int		CMath::aton(const char *s)		{	return ::atoi(s);	}

/** sinus of s */
template <>	inline float	CMath::sin(const float &s)	{	return sinf(s);	}
/** cosinus of s */
template <>	inline float	CMath::cos(const float &s)	{	return cosf(s);	}
/** tangens of s */
template <>	inline float	CMath::tan(const float &s)	{	return tanf(s);	}

/** inverse sinus of s */
template <>	inline float	CMath::asin(const float &s)	{	return asinf(s);	}
/** inverse cosinus of s */
template <>	inline float	CMath::acos(const float &s)	{	return acosf(s);	}
/** inverse tangens of s */
template <>	inline float	CMath::atan(const float &s)	{	return atanf(s);	}

/** exponential of s */
template <>	inline float	CMath::exp(const float &s)	{	return expf(s);	}

/** exponential of s */
template <>	inline float	CMath::log(const float &s)	{	return logf(s);	}
/** exponential of s */
template <>	inline double	CMath::log(const double &s)	{	return log(s);	}

/*
// ICC definition
template <typename T>
::std::ostream&
operator<<(::std::ostream& os, const CMath<T>& lm);


template <typename T>
::std::ostream&
operator<<(::std::ostream& os, const CMath<T>& lm)
{
	return os << lm.value;
}
*/


#endif
