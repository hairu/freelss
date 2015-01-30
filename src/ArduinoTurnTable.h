/*
 ****************************************************************************
 *  Copyright (c) 2014 Uriah Liggett <hairu526@gmail.com>                   *
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

#include "TurnTable.h"

namespace scanner
{

/**
 * An implementation of the TurnTable class that works
 * with the A4988 motor driver.
 */
class ArduinoTurnTable : public TurnTable
{
public:
	ArduinoTurnTable();
	~ArduinoTurnTable();

	/** Rotates this amount in radians */
	int rotate(real theta);

	/** Enable/Disable the stepper motor */
	void setMotorEnabled(bool enabled);

	/** Initialize the turn table */
	static void initialize();

private:

	/** The time to sleep between steps in microseconds */
	int m_stepDelay;

	/** The number of steps per revolution */
	int m_stepsPerRevolution;

	/** The time to sleep between steps in microseconds */
	int m_stabilityDelay;
};

}
