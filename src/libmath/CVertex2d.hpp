/*
 * Copyright (C) 2011 Technische Universitaet Muenchen
 * This file is part of the Sierpi project. For conditions of distribution and
 * use, please see the copyright notice in the file 'copyright.txt' at the root
 * directory of this package and the copyright notice at http://www5.in.tum.de/sierpi
 */

#ifndef CVERTEX_2D_HPP__
#define CVERTEX_2D_HPP__

#include <iostream>
#include "libmath/CMath.hpp"

/**
 * \brief	2D Vector handler
 */
template <typename T>
class CVertex2d
{
public:
	T data[2];	///< vector data

	/**
	 * default constructor
	 */
	inline CVertex2d()
	{
		setZero();
	}

	/**
	 * initialize vector with (x0, x1)
	 */
	inline CVertex2d(const T x0, const T x1)
	{
		data[0] = x0;
		data[1] = x1;
	}

	/**
	 * initialize all vector components with the scalar value 'x'
	 */
	inline CVertex2d(const T x)
	{
		data[0] = x;
		data[1] = x;
	}

	/**
	 * return normalized (length=1) vector
	 */
	inline CVertex2d getNormal()
	{
		CVertex2d<T> v = *this;
		T inv_length = 1.0f/CMath::sqrt(v[0]*v[0] + v[1]*v[1]);
		v[0] *= inv_length;
		v[1] *= inv_length;
		return v;
	}

	inline T getDotProd(CVertex2d<T> &a)
	{
		return data[0]*a.data[0]+data[1]*a.data[1];
	}


	inline T getLength()
	{
		return CMath::sqrt(data[0]*data[0]+data[1]*data[1]);
	}


	inline T getLength2()
	{
		return data[0]*data[0]+data[1]*data[1];
	}

	/**
	 * initialize vector components with the array 'v'
	 */
	inline CVertex2d(const T v[2])
	{
		data[0] = v[0];
		data[1] = v[1];
	}

	/**
	 * set all components of vector to 0
	 */
	inline void setZero()
	{
		data[0] = T(0);
		data[1] = T(0);
	}

	/*******************
	 * OPERATORS
	 *******************/
	/// assign values of a[2] to this vector and return reference to this vector
	inline CVertex2d<T>&	operator=(const T a[2])	{	data[0] = a[0]; data[1] = a[1]; return *this;	};
	/// assign values of vector a to this vector and return reference to this vector
	inline CVertex2d<T>&	operator=(CVertex2d<T> const& a)	{	data[0] = a.data[0]; data[1] = a.data[1]; return *this;	};

	/// return negative of this vector
	inline CVertex2d<T>	operator-()	{	return CVertex2d<T>(-data[0], -data[1]);	}

	/// return new vector (this+a)
	inline CVertex2d<T>	operator+(const T a)	{	return CVertex2d<T>(data[0]+a, data[1]+a);	}
	/// return new vector (this-a)
	inline CVertex2d<T>	operator-(const T a)	{	return CVertex2d<T>(data[0]-a, data[1]-a);	}
	/// return new vector with component wise (this*a)
	inline CVertex2d<T>	operator*(const T a)	{	return CVertex2d<T>(data[0]*a, data[1]*a);	}
	/// return new vector with component wise (this/a)
	inline CVertex2d<T>	operator/(const T a)	{	return CVertex2d<T>(data[0]/a, data[1]/a);	}
	/// add a to this vector and return reference to this vector
	inline CVertex2d<T>&	operator+=(const T a)	{	data[0] += a; data[1] += a;	return *this;	}
	/// subtract a from this vector and return reference to this vector
	inline CVertex2d<T>&	operator-=(const T a)	{	data[0] -= a; data[1] -= a;	return *this;	}
	/// multiply each component of this vector with scalar a and return reference to this vector
	inline CVertex2d<T>&	operator*=(const T a)	{	data[0] *= a; data[1] *= a;	return *this;	}
	/// divide each component of this vector by scalar a and return reference to this vector
	inline CVertex2d<T>&	operator/=(const T a)	{	data[0] /= a; data[1] /= a;	return *this;	}

// CVector
	/// return new vector with sum of this vector and v
	inline CVertex2d<T>	operator+(const CVertex2d<T> &v)	const {	return CVertex2d<T>(data[0]+v.data[0], data[1]+v.data[1]);	}
	/// return new vector with subtraction of vector v from this vector
	inline CVertex2d<T>	operator-(const CVertex2d<T> &v)	const {	return CVertex2d<T>(data[0]-v.data[0], data[1]-v.data[1]);	}
	/// return new vector with values of this vector multiplied component wise with vector v
	inline CVertex2d<T>	operator*(const CVertex2d<T> &v)	const {	return CVertex2d<T>(data[0]*v.data[0], data[1]*v.data[1]);	}
	/// return new vector with values of this vector divided component wise by components of vector v
	inline CVertex2d<T>	operator/(const CVertex2d<T> &v)	const {	return CVertex2d<T>(data[0]/v.data[0], data[1]/v.data[1]);	}

	/// return this vector after adding v
	inline CVertex2d<T>&	operator+=(const CVertex2d<T> &v)	{	data[0] += v.data[0]; data[1] += v.data[1]; 	return *this;	}
	/// return this vector after subtracting v
	inline CVertex2d<T>&	operator-=(const CVertex2d<T> &v)	{	data[0] -= v.data[0]; data[1] -= v.data[1]; 	return *this;	}

	/// return true, if each component of the vector is equal to the corresponding component of vector v
	inline bool	operator==(const CVertex2d<T> &v)	{	return bool(data[0] == v.data[0] && data[1] == v.data[1]);	}
	/// return true, if at lease component of the vector is not equal to the corresponding component of vector v
	inline bool	operator!=(const CVertex2d<T> &v)	{	return bool(data[0] != v.data[0] || data[1] != v.data[1]);	}

	/**
	 * access element i
	 */
	inline T& operator[](const int i)
	{
#ifdef DEBUG
		if (i < 0 || i >= 2)
		{
			std::cerr << "OUT OF ARRAY ACCESS!!! creating null exception..." << std::endl;
			*((int*)(0)) = 0;
		}
#endif
		return data[i];
	}

	/**
	 * \brief	compare set for sort operation
	 */
	struct compareSet
	{
		/**
		 * compare set operator
		 */
		inline bool operator()(CVertex2d<T> *v1, CVertex2d<T> *v2)
		{
			if ((*v1)[0] != (*v2)[0])
				return (*v1)[0] < (*v2)[0];
			if ((*v1)[1] != (*v2)[1])
				return (*v1)[1] < (*v2)[1];
			return false;
		}

		/**
		 * compare set operator
		 */
		inline bool operator()(const CVertex2d<T> &v1, const CVertex2d<T> &v2)
		{
			if (v1.data[0] != v2.data[0])
				return v1.data[0] < v2.data[0];
			if (v1.data[1] != v2.data[1])
				return v1.data[1] < v2.data[1];
			return false;
		}
	};
};


typedef CVertex2d<double>	vertex2d;
typedef CVertex2d<float>	vertex2f;
typedef CVertex2d<int>		vertex2i;


template <class T>
inline
::std::ostream&
operator<<(::std::ostream &co, const CVertex2d<T> &v)
{
	return co << "[" << v.data[0] << ", " << v.data[1] << "]";
}


#endif
