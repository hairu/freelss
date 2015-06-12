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

#pragma once

namespace freelss
{

/**
 * Holds setup information about the hardware.
 */
class Setup
{
public:

	/** Returns the singleton instance */
	static Setup * get();
	static void release();

	/** Encodes property information to the properties vector */
	void encodeProperties(std::vector<Property>& properties);

	/** Decodes property information from the given vector  */
	void decodeProperties(const std::vector<Property>& properties);

	Vector3 cameraLocation;
	Vector3 leftLaserLocation;
	Vector3 rightLaserLocation;
	int rightLaserPin;
	int leftLaserPin;
	int motorEnablePin;
	int motorStepPin;
	int motorDirPin;
	int motorDirPinValue;
	int laserOnValue;
	int stepsPerRevolution;
	int motorResponseDelay;
	int motorStepDelay;
	int httpPort;
	std::string serialNumber;
	UnitOfLength unitOfLength;
	bool haveLaserPlaneNormals;
	Vector3 leftLaserPlaneNormal;
	Vector3 rightLaserPlaneNormal;
	PixelLocation leftLaserCalibrationTop;
	PixelLocation leftLaserCalibrationBottom;
	PixelLocation rightLaserCalibrationTop;
	PixelLocation rightLaserCalibrationBottom;
private:

	/** Default Constructor */
	Setup();

	/** Singleton instance */
	static Setup * m_instance;
};

}
