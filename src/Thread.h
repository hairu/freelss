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

/** Thread class implementation */
class Thread
{
public:
	Thread();
	virtual ~Thread();
	
	/** The thread function */
	virtual void run() = 0;
	
	/** Begins execution of the thread */
	void execute();
	
	/** Blocks the active thread until this thread ends */
	void join();
	
	/** Indicates to the thread that we want it to stop but does not forcefully kill it */
	void stop();
	
	/** Sleeps the active thread the specified number of MICROSECONDS */
	static void usleep(unsigned long microseconds);
	
protected:
	/** True if stop() was called on this thread */
	bool m_stopRequested;
	
private:
	pthread_t m_handle;
};

}
