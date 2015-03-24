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

/**
 * This class controls the motor that rotates the turn table.
 */
class TurnTable
{
public:

	/** Returns the singleton instance */
	static TurnTable * getInstance();

	/** Releases the singleton instance */
	static void release();

	/** Destructor */
	virtual ~TurnTable();

	/** Rotates this amount in radians.  Returns the number of steps taken. */
	virtual int rotate(real theta) = 0;

	/** Enable/Disable the stepper motor */
	virtual void setMotorEnabled(bool enabled) = 0;

protected:

	/** Default Constructor */
	TurnTable();

private:
	/** Singleton instance */
	static TurnTable * m_instance;
};

}
