/*
 ****************************************************************************
 *  Copyright (c) 2016 Uriah Liggett <freelaserscanner@gmail.com>           *
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
 ****************************************************************************/
#pragma once

#include "Camera.h"

namespace freelss
{

/**
 * Utility class for MMAL.
 */
class MmalUtil
{
public:
	MmalUtil();

	/** Returns the name of the attached camera */
	std::string getCameraName();

	void getSensorProperties(const std::string& cameraName, real& sensorWidth, real& sensorHeight, real& focalLength);

	/** Returns the list of supported resolutions for the MMAL camera of the given name. */
	std::vector<CameraResolution> getSupportedResolutions(const std::string& cameraName);

	/** Returns the closest resolution to the given one that is supported by the camera of the given name */
	CameraResolution getClosestResolution(const std::string& cameraName, CameraMode cameraMode);

	/** Return the singleton instance */
	static MmalUtil * get();

	/** Release the singleton instance */
	static void release();

	/** Name of camera v2 */
	static const std::string IMX219;

	/** Name of camera v1 */
	static const std::string OV5647;
private:

	std::vector<CameraResolution> getAllResolutions();

	static MmalUtil * m_instance;
};

} // ns freelss
