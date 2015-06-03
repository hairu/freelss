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
#include "Thread.h"

namespace freelss
{

// The threaded function
static void * G_Thread_ThreadFunc ( void *ptr )
{
	Thread * thread = reinterpret_cast<Thread *>(ptr);

	try
	{
		thread->run();
	}
	catch (freelss::Exception& ex)
	{
		std::cerr << "Exception thrown: " << ex << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unknown Exception Thrown" << std::endl;
	}
	
	return NULL;
}

Thread::Thread() : 	
	m_stopRequested(false),
	m_handle(0)
{
	// Do nothing
}

Thread::~Thread()
{
	// Do nothing
}

void Thread::execute()
{
	m_stopRequested = false;

	if (pthread_create(&m_handle, NULL, G_Thread_ThreadFunc, (void*) this) != 0)
	{
		throw Exception("Error creating thread");
	}
}

void Thread::join()
{
	if (pthread_join(m_handle, NULL) != 0)
	{
		throw Exception("Error joining thread");
	}
}

void Thread::stop()
{
	m_stopRequested = true;
}

void Thread::usleep(unsigned long microseconds)
{
	if (::usleep(microseconds) != 0)
	{
		std::stringstream sstr;
		sstr << "Error sleeping thread, errno=" << errno;
		std::cerr << sstr.str() << std::endl;
	}
}

}
