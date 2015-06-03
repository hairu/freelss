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
#include "RelayLaser.h"
#include "Setup.h"

namespace freelss
{

RelayLaser::RelayLaser() :
	m_rightLaserPin(-1),
	m_leftLaserPin(-1),
	m_laserOnValue(-1),
	m_laserOffValue(-1),
	m_rightLaserOn(false),
	m_leftLaserOn(false)
{
	Setup * setup = Setup::get();

	m_rightLaserPin = setup->rightLaserPin;
	m_leftLaserPin = setup->leftLaserPin;
	m_laserOnValue = setup->laserOnValue;

	m_laserOffValue = m_laserOnValue ? 0 : 1;
}

RelayLaser::~RelayLaser()
{
	turnOff(Laser::ALL_LASERS);
}

void RelayLaser::initialize()
{
	Setup * setup = Setup::get();
	int rightLaserPin = setup->rightLaserPin;
	int leftLaserPin = setup->leftLaserPin;
	int laserOnValue = setup->laserOnValue;
	int laserOffValue = laserOnValue ? 0 : 1;

	// Setup the pin but disable the laser
	pinMode(rightLaserPin, OUTPUT);
	pinMode(leftLaserPin, OUTPUT);
	digitalWrite (rightLaserPin, laserOffValue);
	digitalWrite (leftLaserPin, laserOffValue);
}

void RelayLaser::turnOn(Laser::LaserSide laser)
{
	if (laser == Laser::RIGHT_LASER || laser == Laser::ALL_LASERS)
	{
		digitalWrite (m_rightLaserPin, m_laserOnValue);
		m_rightLaserOn = true;
	}

	if (laser == Laser::LEFT_LASER || laser == Laser::ALL_LASERS)
	{
		digitalWrite (m_leftLaserPin, m_laserOnValue);
		m_leftLaserOn = true;
	}
}

void RelayLaser::turnOff(Laser::LaserSide laser)
{
	if (laser == Laser::RIGHT_LASER || laser == Laser::ALL_LASERS)
	{
		digitalWrite (m_rightLaserPin, m_laserOffValue);
		m_rightLaserOn = false;
	}

	if (laser == Laser::LEFT_LASER || laser == Laser::ALL_LASERS)
	{
		digitalWrite (m_leftLaserPin, m_laserOffValue);
		m_leftLaserOn = false;
	}
}

bool RelayLaser::isOn(Laser::LaserSide laser)
{
	bool on = false;

	if (laser == Laser::ALL_LASERS)
	{
		on = m_rightLaserOn && m_leftLaserOn;
	}
	else if (laser == Laser::RIGHT_LASER)
	{
		on = m_rightLaserOn;
	}
	else if (laser == Laser::LEFT_LASER)
	{
		on = m_leftLaserOn;
	}

	return on;
}

}
