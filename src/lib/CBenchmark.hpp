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
 * CBenchmark.hpp
 *
 *  Created on: Feb 19, 2010
 *      Author: martin
 */

#ifndef CBENCHMARK_HPP_
#define CBENCHMARK_HPP_

#include "lib/CStopwatch.hpp"
#include <string>
#include <iostream>

/**
 * benchmark helper class
 */
class CBenchmark
{
public:
	static bool active;					///< benchmark is activated
	static float delta_output_time;		///< time between printing benchmark data

	/**
	 * activate benchmark
	 */
	static void activate()
	{
		active = true;
	}

	/**
	 * deactivate benchmark
	 */
	static void deactivate()
	{
		active = false;
	}

	/**
	 * set the seconds between 2 benchmark outputs
	 *
	 * \param	p_delta_output_time	time between outputs
	 */
	static void setDeltaOutputTime(float p_delta_output_time)
	{
		delta_output_time = p_delta_output_time;
	}

private:
	CStopwatch stopwatch;
	CStopwatch stopwatch_global;
	std::string prefix_string;
	int counter;
	double last_timestamp;
	double last_output_timestamp;

public:

	/**
	 * \param p_prefix_string	string to print before benchmark information
	 */
	CBenchmark(const char *p_prefix_string)
	{
		prefix_string = p_prefix_string;
		counter = 0;
		stopwatch_global.start();
	}

	/**
	 * start counting the time.
	 *
	 * call this function before the interesting program part
	 */
	void start()
	{
		stopwatch.start();
	}

	/**
	 * return true, if output happens probably next time
	 */
	bool outputReady()
	{
		return (last_output_timestamp + delta_output_time < stopwatch_global.getTime());
	}

	/**
	 * stop counting the time.
	 *
	 * call this function after the part for which the benchmark should be generated.
	 *
	 * for opengl: use glFinish() before this call to get accurate measurements!
	 */
	void stop()
	{
		stopwatch.stop();
		counter++;

		if (active)
		{
			if (outputReady())
			{
				std::cout << prefix_string << ": " << stopwatch.getTime()/(double)counter << std::endl;
				last_output_timestamp += delta_output_time;
			}
		}
	}
};

#endif /* CBENCHMARK_HPP_ */
