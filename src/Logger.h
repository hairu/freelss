/*
 ****************************************************************************
 *  Copyright (c) 2015 Uriah Liggett <freelaserscanner@gmail.com>           *
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


namespace freelss
{

class Logger
{

public:
	Logger(FILE * fp);
	Logger& operator << (const std::string& str);
	Logger& operator << (const char * str);
	Logger& operator << (unsigned long number);
	Logger& operator << (long number);
	Logger& operator << (unsigned int number);
	Logger& operator << (int number);
	Logger& operator << (unsigned short number);
	Logger& operator << (short number);
	Logger& operator << (unsigned char number);
	Logger& operator << (char number);
	Logger& operator << (float number);
	Logger& operator << (double number);
	static const char * ENDL;
private:
	FILE * m_fp;
};

extern Logger InfoLog;
extern Logger ErrorLog;
}
