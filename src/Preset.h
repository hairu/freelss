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

#include "Laser.h"
#include "Camera.h"
#include "PlyWriter.h"

namespace freelss
{

/**
 * Holds camera settings, image processing settings, delay settings,
 * and other related settings for scanning.
 */
class Preset
{
public:

	/** The action that should be taken to merge laser results */
	enum LaserMergeAction {LMA_PREFER_RIGHT_LASER, LMA_SEPARATE_BY_COLOR };


	Preset();

	/** Encodes property information to the properties vector */
	void encodeProperties(std::vector<Property>& properties, bool isActivePreset);

	/**
	 * Decodes property information from the given vector.
	 */
	void decodeProperties(const std::vector<Property>& properties, const std::string& name, bool &isActivePreset);

	/** Detects the names of all the presets */
	static std::vector<std::string> detectPresetNames(const std::vector<Property>& properties);

	std::string name;
	Laser::LaserSide laserSide;
	Camera::CameraMode cameraMode;
	real laserThreshold;
	int minLaserWidth;
	int maxLaserWidth;
	real maxObjectSize;
	real maxTriangleEdgeLength;
	int numLaserRowBins;
	int laserDelay;
	int stabilityDelay;
	int id;
	int framesPerRevolution;
	bool generateXyz;
	bool generateStl;
	bool generatePly;
	real groundPlaneHeight;
	LaserMergeAction laserMergeAction;
	PlyDataFormat plyDataFormat;
};

}
