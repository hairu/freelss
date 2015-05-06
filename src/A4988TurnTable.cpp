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
#include "A4988TurnTable.h"
#include "Thread.h"
#include "Setup.h"
#include "PresetManager.h"

namespace freelss
{

A4988TurnTable::A4988TurnTable()
{
	Setup * setup = Setup::get();
	m_responseDelay = setup->motorResponseDelay;
	m_stepDelay = setup->motorStepDelay;
	m_enablePin = setup->motorEnablePin;
	m_stepPin = setup->motorStepPin;
	m_directionPin = setup->motorDirPin;
	m_stepsPerRevolution = setup->stepsPerRevolution;
	m_stabilityDelay = PresetManager::get()->getActivePreset().stabilityDelay;
}

A4988TurnTable::~A4988TurnTable()
{
	// Disable the stepper
	digitalWrite(m_enablePin, HIGH);
	Thread::usleep(m_responseDelay);
}

void A4988TurnTable::initialize()
{
	Setup * setup = Setup::get();
	int responseDelay = setup->motorResponseDelay;
	int enablePin = setup->motorEnablePin;
	int stepPin = setup->motorStepPin;
	int directionPin = setup->motorDirPin;

	// Disable the stepper
	pinMode(enablePin, OUTPUT);
	digitalWrite (enablePin, HIGH);

	// Put us in a known state
	pinMode(stepPin, OUTPUT);
	digitalWrite (stepPin, LOW);

	pinMode(directionPin, OUTPUT);
	digitalWrite (directionPin, LOW);

	Thread::usleep(responseDelay);
}

void A4988TurnTable::step()
{
	digitalWrite(m_stepPin, LOW);
	Thread::usleep(m_responseDelay);

	digitalWrite(m_stepPin, HIGH);
	Thread::usleep(m_responseDelay);

	// Wait the step delay (this is how speed is controlled)
	Thread::usleep(m_stepDelay);
}

int A4988TurnTable::rotate(real theta)
{
	// Get the percent of a full revolution that theta is and convert that to number of steps
	int numSteps = (theta / (2 * PI)) * m_stepsPerRevolution;

	// Step the required number of steps
	for (int i = 0; i < numSteps; i++)
	{
		step();
	}

	// Sleep the stability delay amount
	Thread::usleep(m_stabilityDelay);

	return numSteps;
}

void A4988TurnTable::setMotorEnabled(bool enabled)
{
	int value = enabled ? LOW : HIGH;

	digitalWrite (m_enablePin, value);
	Thread::usleep(m_responseDelay);
}

}
