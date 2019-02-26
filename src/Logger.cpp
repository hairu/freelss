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
#include "Main.h"
#include "Logger.h"

namespace freelss
{
Logger InfoLog(stdout);
Logger ErrorLog(stderr);
const char * Logger::ENDL = "\n";

Logger::Logger(FILE * fp) :
	m_fp(fp)
{
	// Do nothing
}

Logger& Logger::operator << (const std::string& str)
{
	fprintf(m_fp, "%s", str.c_str());
	return *this;
}

Logger& Logger::operator << (const char * str)
{
	fprintf(m_fp, "%s", str);
	return *this;
}

Logger& Logger::operator << (unsigned long number)
{
	fprintf(m_fp, "%lu", number);
	return *this;
}

Logger& Logger::operator << (long number)
{
	fprintf(m_fp, "%ld", number);
	return *this;
}

Logger& Logger::operator << (unsigned int number)
{
	fprintf(m_fp, "%u", number);
	return *this;
}

Logger& Logger::operator << (int number)
{
	fprintf(m_fp, "%d", number);
	return *this;
}

Logger& Logger::operator << (unsigned short number)
{
	fprintf(m_fp, "%hu", number);
	return *this;
}

Logger& Logger::operator << (short number)
{
	fprintf(m_fp, "%hd", number);
	return *this;
}

Logger& Logger::operator << (unsigned char number)
{
	fprintf(m_fp, "%hhu", number);
	return *this;
}

Logger& Logger::operator << (char number)
{
	fprintf(m_fp, "%hhd", number);
	return *this;
}

Logger& Logger::operator << (float number)
{
	fprintf(m_fp, "%f", number);
	return *this;
}

Logger& Logger::operator << (double number)
{
	fprintf(m_fp, "%f", number);
	return *this;
}

}
