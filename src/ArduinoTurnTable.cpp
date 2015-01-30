/*
 ****************************************************************************
 *  Copyright (c) 2015 Vik Olliver vik@diamondage.co.nz
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
#include "ArduinoTurnTable.h"
#include "ArduinoSerial.h"
#include "Thread.h"
#include "Settings.h"
#include "fsdefines.h"

namespace scanner
{

ArduinoTurnTable::ArduinoTurnTable()
{
	Settings * settings = Settings::get();
	m_stepsPerRevolution = settings->readInt(Settings::GENERAL_SETTINGS, Settings::STEPS_PER_REVOLUTION);
	m_stabilityDelay = settings->readInt(Settings::GENERAL_SETTINGS, Settings::STABILITY_DELAY);
	m_stepDelay = settings->readInt(Settings::A4988_SETTINGS, Settings::STEP_DELAY);
}

ArduinoTurnTable::~ArduinoTurnTable()
{
	// Disable the stepper
    ArduinoSerial::sendchar(MC_SELECT_STEPPER);     // Select the turntable
    ArduinoSerial::sendchar(MC_TURNTABLE_STEPPER);
    ArduinoSerial::sendchar(MC_TURN_STEPPER_OFF);   // Shut down turntable stepper
    ArduinoSerial::sendchar(MC_TURN_LIGHT_OFF);
}

void ArduinoTurnTable::initialize()
{
	// Enable the stepper
    ArduinoSerial::sendchar(MC_SELECT_STEPPER);     // Select the turntable
    ArduinoSerial::sendchar(MC_TURNTABLE_STEPPER);
    ArduinoSerial::sendchar(MC_TURN_STEPPER_ON);
    ArduinoSerial::sendchar(MC_SET_DIRECTION_CCW);

    // Turn the scanner light on full
    ArduinoSerial::sendchar(MC_TURN_LIGHT_ON);      // Light on full.
    ArduinoSerial::sendchar(255);
}


int ArduinoTurnTable::rotate(real theta)
{
	// Get the percent of a full revolution that theta is and convert that to number of steps
	unsigned int returnSteps,numSteps;
    unsigned int segSteps;   // Number of steps in this segment - we do 255 at a time

    returnSteps=numSteps = (theta / (2 * PI)) * m_stepsPerRevolution;

	// Step the required number of steps
    do {
        ArduinoSerial::sendchar(MC_PERFORM_STEP);
        if(numSteps<256){
            segSteps=numSteps;
        }else{
            segSteps=255;
        }
        ArduinoSerial::sendchar(segSteps);
	    Thread::usleep(m_stepDelay*segSteps);
        numSteps-=segSteps;
    } while (numSteps > 0);

	// Sleep the stability delay amount
	Thread::usleep(m_stabilityDelay);

	return returnSteps;
}

void ArduinoTurnTable::setMotorEnabled(bool enabled)
{
	//  Enable stepper
    std::cout << (enabled ? "Enabling":"Disabling") << " Arduino-based stepper motor\n";
    ArduinoSerial::sendchar(MC_SELECT_STEPPER);     // Select the turntable
    ArduinoSerial::sendchar(MC_TURNTABLE_STEPPER);
	Thread::usleep(m_stabilityDelay);
    if (enabled == true)
        {
        ArduinoSerial::sendchar(MC_TURN_STEPPER_ON);
        // Turn the scanner light on full
        ArduinoSerial::sendchar(MC_TURN_LIGHT_ON);      // Light on full.
        ArduinoSerial::sendchar(255);
        }
    else 
        {   
        ArduinoSerial::sendchar(MC_TURN_STEPPER_OFF);
        // Turn the scanner light off
        ArduinoSerial::sendchar(MC_TURN_LIGHT_OFF);      // Light on full.
        }
	Thread::usleep(m_stabilityDelay);

    ArduinoSerial::sendchar(MC_SET_DIRECTION_CCW);
	Thread::usleep(m_stabilityDelay);
}

}
