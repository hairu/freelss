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

/** Interface for laser control */
class Laser
{
public:

	/** Represents one of the available lasers */
	enum LaserSide { LEFT_LASER, RIGHT_LASER, ALL_LASERS };

	/** Returns the singleton instance */
	static Laser * getInstance();

	/** Releases the singleton instance */
	static void release();

	/** Returns the string representation of the laser side */
	static std::string toString(Laser::LaserSide side);

	virtual ~Laser();

	/** Turns the laser on */
	virtual void turnOn(Laser::LaserSide laser) = 0;

	/** Turns the laser off */
	virtual void turnOff(Laser::LaserSide laser) = 0;

	/** Returns true if the given laser is on */
	virtual bool isOn(Laser::LaserSide laser) = 0;

protected:

	Laser();

private:
	/** The singleton instance */
	static Laser * m_instance;
};

}
