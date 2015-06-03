/*
 ****************************************************************************
 *  Copyright (c) 2014 Uriah Liggett <freelaserscanner@gmail.com>           *
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

/** CriticalSection implementation */
class CriticalSection
{
public:
	CriticalSection();
	~CriticalSection();

	void enter();
	void enter(const std::string& name);
	void leave();
	
private:
	std::string errorToString(int error);
	CriticalSection(const CriticalSection& ) { /* NO COPYING */ }
	CriticalSection& operator = (const CriticalSection& ) { return * this; /* NO ASSIGNMENT */ }
	pthread_mutex_t m_handle;
};

}

