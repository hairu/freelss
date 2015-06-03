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
	int val = pthread_mutex_init(&m_handle, NULL);
	if (val != 0)
	{
		throw Exception("Error initializing mutex: " + errorToString(val));
	}
}


CriticalSection::~CriticalSection()
{
	int val = pthread_mutex_destroy(&m_handle);
	if (val != 0)
	{
		throw Exception("Error destroying mutex: " + errorToString(val));
	}
}

void CriticalSection::enter(const std::string& name)
{
	int val = pthread_mutex_lock(&m_handle);
	if (val != 0)
	{
		throw Exception("Error locking mutex: " + errorToString(val));
	}
}

void CriticalSection::enter()
{
	int val = pthread_mutex_lock(&m_handle);
	if (val != 0)
	{
		throw Exception("Error locking mutex: " + errorToString(val));
	}
}

void CriticalSection::leave()
{
	int val = pthread_mutex_unlock(&m_handle);
	if (val != 0)
	{
		throw Exception("Error unlocking mutex: " + errorToString(val));
	}
}

std::string CriticalSection::errorToString(int error)
{
	std::string message;
	switch (error)
	{
	case EINVAL:
		message = "EINVAL";
		break;

	case EBUSY:
		message = "EBUSY";
		break;

	case EAGAIN:
		message = "EAGAIN";
		break;

	case EDEADLK:
		message = "EDEADLK";
		break;

	case EPERM:
		message = "EPERM";
		break;

	case EINTR:
		message = "EINTR";
		break;

	default:
		message = "UNKNOWN";
		break;
	}

	return message;
}

}
