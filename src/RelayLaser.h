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

#include "Laser.h"

namespace freelss
{
class RelayLaser : public Laser
{
public:
	RelayLaser();
	~RelayLaser();

	/** Initialize the laser */
	static void initialize();

	void turnOn(Laser::LaserSide laser);
	void turnOff(Laser::LaserSide laser);
	bool isOn(Laser::LaserSide laser);
private:
	int m_rightLaserPin;
	int m_leftLaserPin;
	int m_laserOnValue;
	int m_laserOffValue;
	bool m_rightLaserOn;
	bool m_leftLaserOn;
};

}
