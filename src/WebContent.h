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


namespace scanner
{

class WebContent
{
public:

	/** The temporary data that is built up as the calibration is performed */
	struct CalibrationData
	{
		real camZ;        // World Z camera location
		real rightLaserX; // In scaled image pixels
		real rightLaserY; // In scaled image pixelss
	};

	static std::string scan(const std::vector<ScanResult>& pastScans);
	static std::string scanRunning(real progress, real remainingTime);
	static std::string cal1();
	static std::string settings(const std::string& message, const std::string& rotationDegrees);
private:
	static std::string setting(const std::string& name, const std::string& label,
			const std::string& value, const std::string& description, const std::string& units = "", bool readOnly = false);

	static std::string setting(const std::string& name, const std::string& label,
			int value, const std::string& description, const std::string& units = "", bool readOnly = false);

	static std::string setting(const std::string& name, const std::string& label,
			real value, const std::string& description, const std::string& units = "", bool readOnly = false);

	static std::string scanResult(size_t index, const ScanResult& result);

	static const std::string CSS;
	static const std::string JAVASCRIPT;
	static const std::string CAMERA_X_DESCR;
	static const std::string CAMERA_Y_DESCR;
	static const std::string CAMERA_Z_DESCR;
	static const std::string RIGHT_LASER_X_DESCR;
	static const std::string RIGHT_LASER_Y_DESCR;
	static const std::string RIGHT_LASER_Z_DESCR;
	static const std::string LEFT_LASER_X_DESCR;
	static const std::string LEFT_LASER_Y_DESCR;
	static const std::string LEFT_LASER_Z_DESCR;
	static const std::string LASER_MAGNITUDE_THRESHOLD_DESCR;
	static const std::string LASER_DELAY_DESCR;
	static const std::string RIGHT_LASER_PIN_DESCR;
	static const std::string LEFT_LASER_PIN_DESCR;
	static const std::string LASER_ON_VALUE_DESCR;
	static const std::string STABILITY_DELAY_DESCR;
	static const std::string MAX_LASER_WIDTH_DESCR;
	static const std::string MIN_LASER_WIDTH_DESCR;
	static const std::string STEPS_PER_REVOLUTION_DESCR;
	static const std::string ENABLE_PIN_DESCR;
	static const std::string STEP_PIN_DESCR;
	static const std::string STEP_DELAY_DESCR;
	static const std::string DIRECTION_PIN_DESCR;
	static const std::string RESPONSE_DELAY_DESCR;
};

}
