/*
 ****************************************************************************
 *  Copyright (c) 2015 Vik Olliver vik@diamondage.co.nz                     *
 *	This file is part of FreeLSS.                                           *
 *                                                                          *
 *  FreeLSS is free software: you can redistribute it and/or modify         *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  FreeLSS is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with FreeLSS.  If not, see <http://www.gnu.org/licenses/>.       *
 ****************************************************************************
*/

#pragma once

// A simple, send-only serial driver to control an Arduino turntable

namespace scanner
{
class ArduinoSerial
{
public:
	ArduinoSerial();
	~ArduinoSerial();

	/* Initialize the serial port */
	static int initialize(const char* port);
	static int sendchar(char c);
private:
	static int m_fd;
};

}
