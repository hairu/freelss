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

#include "Main.h"
#include "CriticalSection.h"

namespace freelss
{

CriticalSection::CriticalSection()
{
	if (pthread_mutex_init(&m_handle, NULL) != 0)
    {
        throw Exception("Error initializing mutex");
    }
}


CriticalSection::~CriticalSection()
{
	if (pthread_mutex_destroy(&m_handle) != 0)
	{
		throw Exception("Error destroying mutex");
	}
}


void CriticalSection::enter()
{
	if (pthread_mutex_lock(&m_handle) != 0)
	{
		throw Exception("Error locking mutex");
	}
}

void CriticalSection::leave()
{
	if (pthread_mutex_unlock(&m_handle) != 0)
	{
		throw Exception("Error unlocking mutex");
	}
}

}
