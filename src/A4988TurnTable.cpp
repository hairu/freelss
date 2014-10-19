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

#include "Main.h"
#include "A4988TurnTable.h"
#include "Thread.h"
#include "Settings.h"

namespace scanner
{

A4988TurnTable::A4988TurnTable()
{
	Settings * settings = Settings::get();
	m_responseDelay = settings->readInt(Settings::A4988_SETTINGS, Settings::RESPONSE_DELAY);
	m_stepDelay = settings->readInt(Settings::A4988_SETTINGS, Settings::STEP_DELAY);
	m_enablePin = settings->readInt(Settings::A4988_SETTINGS, Settings::ENABLE_PIN);
	m_stepPin = settings->readInt(Settings::A4988_SETTINGS, Settings::STEP_PIN);
	m_directionPin = settings->readInt(Settings::A4988_SETTINGS, Settings::DIRECTION_PIN);
	m_stepsPerRevolution = settings->readInt(Settings::GENERAL_SETTINGS, Settings::STEPS_PER_REVOLUTION);
	m_stabilityDelay = settings->readInt(Settings::GENERAL_SETTINGS, Settings::STABILITY_DELAY);
}

A4988TurnTable::~A4988TurnTable()
{
	// Disable the stepper
	digitalWrite(m_enablePin, HIGH);
	Thread::usleep(m_responseDelay);
}

void A4988TurnTable::initialize()
{
	Settings * settings = Settings::get();
	int responseDelay = settings->readInt(Settings::A4988_SETTINGS, Settings::RESPONSE_DELAY);
	int enablePin = settings->readInt(Settings::A4988_SETTINGS, Settings::ENABLE_PIN);
	int stepPin = settings->readInt(Settings::A4988_SETTINGS, Settings::STEP_PIN);
	int directionPin = settings->readInt(Settings::A4988_SETTINGS, Settings::DIRECTION_PIN);

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
