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

#include "CWiiBalanceBoard.hpp"
#include <bluetooth/bluetooth.h>
#include <cwiid.h>
#include "lib/CError.hpp"


class CPrivateWiiBalanceBoard
{
public:
	cwiid_wiimote_t *wiimote;

	int callback_mesg_count;
	cwiid_wiimote_t *callback_wiimote;
	union cwiid_mesg *callback_mesg;
	struct timespec *callback_timestamp;
};


class CWiiBalanceBoard *wii_balance_board_callback_class;


static void wiiremote_callback_static(
				cwiid_wiimote_t *p_wiimote,
				int p_mesg_count,
				union cwiid_mesg p_mesg[],
				struct timespec *p_timestamp
)
{
	wii_balance_board_callback_class->privateWiiBalanceBoard->callback_wiimote = p_wiimote;
	wii_balance_board_callback_class->privateWiiBalanceBoard->callback_mesg_count = p_mesg_count;
	wii_balance_board_callback_class->privateWiiBalanceBoard->callback_mesg = p_mesg;
	wii_balance_board_callback_class->privateWiiBalanceBoard->callback_timestamp = p_timestamp;

	wii_balance_board_callback_class->wiiremote_callback();
}


void CWiiBalanceBoard::wiiremote_callback(
)
{
	for (int i=0; i < privateWiiBalanceBoard->callback_mesg_count; i++)
	{
		switch (privateWiiBalanceBoard->callback_mesg[i].type)
		{
			case CWIID_MESG_BALANCE:
				right_top = privateWiiBalanceBoard->callback_mesg[i].balance_mesg.right_top;
				right_bottom = privateWiiBalanceBoard->callback_mesg[i].balance_mesg.right_bottom;
				left_top = privateWiiBalanceBoard->callback_mesg[i].balance_mesg.left_top;
				left_bottom = privateWiiBalanceBoard->callback_mesg[i].balance_mesg.left_bottom;

				if (!init_values_valid)
				{
					init_right_top = privateWiiBalanceBoard->callback_mesg[i].balance_mesg.right_top;
					init_right_bottom = privateWiiBalanceBoard->callback_mesg[i].balance_mesg.right_bottom;
					init_left_top = privateWiiBalanceBoard->callback_mesg[i].balance_mesg.left_top;
					init_left_bottom = privateWiiBalanceBoard->callback_mesg[i].balance_mesg.left_bottom;
					init_values_valid = true;
				}

				delta_right_top = (int)privateWiiBalanceBoard->callback_mesg[i].balance_mesg.right_top - init_right_top;
				delta_right_bottom = (int)privateWiiBalanceBoard->callback_mesg[i].balance_mesg.right_bottom - init_right_bottom;
				delta_left_top = (int)privateWiiBalanceBoard->callback_mesg[i].balance_mesg.left_top - init_left_top;
				delta_left_bottom = (int)privateWiiBalanceBoard->callback_mesg[i].balance_mesg.left_bottom - init_left_bottom;

				update_counter++;
				break;

			default:
				break;
		}
	}
}

/**
 * update
 */
bool CWiiBalanceBoard::updateVectorAndWeight()
{
	if (last_update_vector_update_counter == update_counter)
		return false;

	vector_x = 	((float)delta_right_top - (float)delta_left_top) +
				((float)delta_right_bottom - (float)delta_left_bottom);

	vector_y = 	((float)delta_right_top - (float)delta_right_bottom) +
				((float)delta_left_top - (float)delta_left_bottom);

	vector_x *= 1.0f/1700.0f;
	vector_y *= 1.0f/1700.0f;

	weight = (float)delta_right_top + (float)delta_right_bottom + (float)delta_left_top + (float)delta_left_bottom;
	weight *=  (4.0*4.0/1700.0);


	last_update_vector_update_counter = update_counter;
	return true;
}

CWiiBalanceBoard::CWiiBalanceBoard(bool p_verbose)	:
	verbose(p_verbose)
{
	privateWiiBalanceBoard = new CPrivateWiiBalanceBoard;
	privateWiiBalanceBoard->wiimote = NULL;
}

void CWiiBalanceBoard::clear()
{
	if (privateWiiBalanceBoard->wiimote != NULL)
	{
		if (cwiid_close(privateWiiBalanceBoard->wiimote))
		{
			error << "ERROR: cwiid_close()" << CError::endl;
			return;
		}

		privateWiiBalanceBoard->wiimote = NULL;
	}

	init_values_valid = false;
	update_counter = 0;
	last_update_vector_update_counter = 0;
}

void CWiiBalanceBoard::connect(char *btaddr)
{
	clear();

	bdaddr_t bdaddr;

	if (btaddr != NULL)
	{
		if (str2ba(btaddr, &bdaddr))
		{
			error << "Bluetooth address has wrong MAC format: " << btaddr << CError::endl;
			return;
		}
	}
	else
	{
//			bdaddr = *(bdaddr_t*)BDADDR_ANY;
		bdaddr.b[0] = 0;
		bdaddr.b[1] = 0;
		bdaddr.b[2] = 0;
		bdaddr.b[3] = 0;
		bdaddr.b[4] = 0;
		bdaddr.b[5] = 0;
	}

//		std::cout << "BT: " << btaddr << std::endl;

	if (verbose)	std::cout << "Searching and connecting to  wii remote..." << std::endl;

	// connect to wiimote using default timeout
	privateWiiBalanceBoard->wiimote = cwiid_open(&bdaddr, 0);
	if (!privateWiiBalanceBoard->wiimote)
	{
		error << "ERROR: cannot connect to wii remote" << CError::endl;
		return;
	}

	wii_balance_board_callback_class = this;
	if (cwiid_set_mesg_callback(privateWiiBalanceBoard->wiimote, wiiremote_callback_static))
	{
		error << "ERROR: unable to set message callback for wii remote" << CError::endl;
		return;
	}

	if (cwiid_set_rpt_mode(privateWiiBalanceBoard->wiimote, CWIID_RPT_BALANCE | CWIID_RPT_BTN))
	{
		error << "ERROR: unable to set report mode for balance board" << CError::endl;
		return;
	}

	if (cwiid_enable(privateWiiBalanceBoard->wiimote, CWIID_FLAG_MESG_IFC))
	{
		error << "ERROR: error enabling messaging" << CError::endl;
		return;
	}

	if (verbose)
	{
		char straddr[19];
		ba2str(&bdaddr, straddr);
		std::cout << "Connected to wiiremote " << straddr << std::endl;
	}
}


CWiiBalanceBoard::~CWiiBalanceBoard()
{
	clear();
	delete privateWiiBalanceBoard;
}
