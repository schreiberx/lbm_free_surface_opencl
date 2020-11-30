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
 * CWiiBalanceBoard.hpp
 *
 *  Created on: May 11, 2010
 *      Author: martin
 */

#ifndef CWIIBALANCEBOARD_HPP_
#define CWIIBALANCEBOARD_HPP_

#include "lib/CError.hpp"


/**
 * abstraction class to wii balance board
 */
class CWiiBalanceBoard
{
public:
	class CPrivateWiiBalanceBoard *privateWiiBalanceBoard;

	void wiiremote_callback();

public:
	bool init_values_valid;

	int update_counter;

	// balance board initialization values
	int init_right_top;
    int init_right_bottom;
    int init_left_top;
    int init_left_bottom;

	// balance board values
	int right_top;
    int right_bottom;
    int left_top;
    int left_bottom;

	// balance board difference values between recent and initialization values
	int delta_right_top;
    int delta_right_bottom;
    int delta_left_top;
    int delta_left_bottom;

    // vector representing the direction of the heaviest weight
    float vector_x;
    float vector_y;
    int last_update_vector_update_counter;

    float weight;

	CError error;
	bool verbose;

	/**
	 * update
	 */
	bool updateVectorAndWeight();

	CWiiBalanceBoard(bool p_verbose);

	void clear();

	void connect(char *btaddr);

	~CWiiBalanceBoard();
};

#endif /* CWIIBALANCEBOARD_HPP_ */
