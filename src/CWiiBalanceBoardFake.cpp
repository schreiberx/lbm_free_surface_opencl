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
#include "lib/CError.hpp"


class CPrivateWiiBalanceBoard
{
public:
};



void CWiiBalanceBoard::wiiremote_callback()
{
}

bool CWiiBalanceBoard::updateVectorAndWeight()
{
	return true;
}

CWiiBalanceBoard::CWiiBalanceBoard(bool p_verbose)	:
	verbose(p_verbose)
{
}

void CWiiBalanceBoard::clear()
{
}

void CWiiBalanceBoard::connect(char *btaddr)
{
}


CWiiBalanceBoard::~CWiiBalanceBoard()
{
}
