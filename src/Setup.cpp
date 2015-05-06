/*
 ****************************************************************************
 *  Copyright (c) 2015 Uriah Liggett <freelaserscanner@gmail.com>           *
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
#include "Setup.h"

namespace freelss
{

Setup * Setup::m_instance = NULL;

Setup * Setup::get()
{
	if (Setup::m_instance == NULL)
	{
		Setup::m_instance = new Setup();
	}

	return Setup::m_instance;
}

void Setup::release()
{
	delete Setup::m_instance;
	Setup::m_instance = NULL;
}

Setup::Setup() :
	cameraLocation(),
	leftLaserLocation(),
	rightLaserLocation(),
	rightLaserPin(5),
	leftLaserPin(4),
	motorEnablePin(0),
	motorStepPin(7),
	motorDirPin(1),
	motorDirPinValue(0),
	laserOnValue(1),
	stepsPerRevolution(3200),
	motorResponseDelay(2),
	motorStepDelay(5000),
	httpPort(80),
	serialNumber(""),
	unitOfLength(UL_INCHES)
{
	cameraLocation.x = 0;
	cameraLocation.y = 82.55;
	cameraLocation.z = 260.35;

	leftLaserLocation.x = -125.73;
	leftLaserLocation.y = 82.55;
	leftLaserLocation.z = 260.35;

	rightLaserLocation.x = 125.73;
	rightLaserLocation.y = 82.55;
	rightLaserLocation.z = 260.35;
}


void Setup::encodeProperties(std::vector<Property>& properties)
{
	properties.push_back(Property("setup.cameraLocation.x", ToString(cameraLocation.x)));
	properties.push_back(Property("setup.cameraLocation.y", ToString(cameraLocation.y)));
	properties.push_back(Property("setup.cameraLocation.z", ToString(cameraLocation.z)));

	properties.push_back(Property("setup.leftLaserLocation.x", ToString(leftLaserLocation.x)));
	properties.push_back(Property("setup.leftLaserLocation.y", ToString(leftLaserLocation.y)));
	properties.push_back(Property("setup.leftLaserLocation.z", ToString(leftLaserLocation.z)));

	properties.push_back(Property("setup.rightLaserLocation.x", ToString(rightLaserLocation.x)));
	properties.push_back(Property("setup.rightLaserLocation.y", ToString(rightLaserLocation.y)));
	properties.push_back(Property("setup.rightLaserLocation.z", ToString(rightLaserLocation.z)));

	properties.push_back(Property("setup.rightLaserPin", ToString(rightLaserPin)));
	properties.push_back(Property("setup.leftLaserPin", ToString(leftLaserPin)));
	properties.push_back(Property("setup.motorEnablePin", ToString(motorEnablePin)));
	properties.push_back(Property("setup.motorStepPin", ToString(motorStepPin)));
	properties.push_back(Property("setup.motorDirPin", ToString(motorDirPin)));
	properties.push_back(Property("setup.motorDirPinValue", ToString(motorDirPinValue)));
	properties.push_back(Property("setup.laserOnValue", ToString(laserOnValue)));
	properties.push_back(Property("setup.stepsPerRevolution", ToString(stepsPerRevolution)));
	properties.push_back(Property("setup.motorResponseDelay", ToString(motorResponseDelay)));
	properties.push_back(Property("setup.motorStepDelay", ToString(motorStepDelay)));
	properties.push_back(Property("setup.httpPort", ToString(httpPort)));
	properties.push_back(Property("setup.serialNumber", serialNumber));

	properties.push_back(Property("setup.unitOfLength", ToString((int)unitOfLength)));
}

void Setup::decodeProperties(const std::vector<Property>& properties)
{
	for (size_t iProp = 0; iProp < properties.size(); iProp++)
	{
		const Property& prop = properties[iProp];

		if (prop.name == "setup.cameraLocation.x")
		{
			cameraLocation.x = ToReal(prop.value);
		}
		else if (prop.name == "setup.cameraLocation.y")
		{
			cameraLocation.y = ToReal(prop.value);
		}
		else if (prop.name == "setup.cameraLocation.z")
		{
			cameraLocation.z = ToReal(prop.value);
		}
		else if (prop.name == "setup.leftLaserLocation.x")
		{
			leftLaserLocation.x = ToReal(prop.value);
		}
		else if (prop.name == "setup.leftLaserLocation.y")
		{
			leftLaserLocation.y = ToReal(prop.value);
		}
		else if (prop.name == "setup.leftLaserLocation.z")
		{
			leftLaserLocation.z = ToReal(prop.value);
		}
		else if (prop.name == "setup.rightLaserLocation.x")
		{
			rightLaserLocation.x = ToReal(prop.value);
		}
		else if (prop.name == "setup.rightLaserLocation.y")
		{
			rightLaserLocation.y = ToReal(prop.value);
		}
		else if (prop.name == "setup.rightLaserLocation.z")
		{
			rightLaserLocation.z = ToReal(prop.value);
		}
		else if (prop.name == "setup.rightLaserPin")
		{
			rightLaserPin = ToInt(prop.value);
		}
		else if (prop.name == "setup.leftLaserPin")
		{
			leftLaserPin = ToInt(prop.value);
		}
		else if (prop.name == "setup.motorEnablePin")
		{
			motorEnablePin = ToInt(prop.value);
		}
		else if (prop.name == "setup.motorStepPin")
		{
			motorStepPin = ToInt(prop.value);
		}
		else if (prop.name == "setup.motorDirPin")
		{
			motorDirPin = ToInt(prop.value);
		}
		else if (prop.name == "setup.motorDirPinValue")
		{
			motorDirPinValue = ToInt(prop.value);
		}
		else if (prop.name == "setup.laserOnValue")
		{
			laserOnValue = ToInt(prop.value);
		}
		else if (prop.name == "setup.stepsPerRevolution")
		{
			stepsPerRevolution = ToInt(prop.value);
		}
		else if (prop.name == "setup.motorResponseDelay")
		{
			motorResponseDelay = ToInt(prop.value);
		}
		else if (prop.name == "setup.motorStepDelay")
		{
			motorStepDelay = ToInt(prop.value);
		}
		else if (prop.name == "setup.httpPort")
		{
			httpPort = ToInt(prop.value);
		}
		else if (prop.name == "setup.serialNumber")
		{
			serialNumber = prop.value;
		}
		else if (prop.name == "setup.unitOfLength")
		{
			unitOfLength = (UnitOfLength) ToInt(prop.value);
		}
	}
}

}
