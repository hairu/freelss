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
#include "Preset.h"
#include <set>

namespace freelss
{

Preset::Preset() :
	name("Preset"),
	laserSide(Laser::RIGHT_LASER),
	cameraMode(Camera::CM_VIDEO_1P2MP),
	laserThreshold(10),
	minLaserWidth(3),
	maxLaserWidth(40),
	maxObjectSize(215.9),  // 8.5"
	maxTriangleEdgeLength(12), // 12mm
	numLaserRowBins(4),
	laserDelay(180000),
	stabilityDelay(0),
	id(-1),
	framesPerRevolution(800),
	generateXyz (false),
	generateStl(true),
	generatePly(true),
	groundPlaneHeight(0),
	laserMergeAction (LMA_PREFER_RIGHT_LASER),
	plyDataFormat(PLY_BINARY)
{
	// Do nothing
}


void Preset::encodeProperties(std::vector<Property>& properties, bool isActivePreset)
{
	properties.push_back(Property("presets." + name + ".laserSide", ToString((int)laserSide)));
	properties.push_back(Property("presets." + name + ".cameraMode", ToString((int)cameraMode)));
	properties.push_back(Property("presets." + name + ".laserThreshold", ToString(laserThreshold)));
	properties.push_back(Property("presets." + name + ".minLaserWidth", ToString(minLaserWidth)));
	properties.push_back(Property("presets." + name + ".maxLaserWidth", ToString(maxLaserWidth)));
	properties.push_back(Property("presets." + name + ".maxObjectSize", ToString(maxObjectSize)));
	properties.push_back(Property("presets." + name + ".maxTriangleEdgeLength", ToString(maxTriangleEdgeLength)));
	properties.push_back(Property("presets." + name + ".numLaserRowBins", ToString(numLaserRowBins)));
	properties.push_back(Property("presets." + name + ".laserDelay", ToString(laserDelay)));
	properties.push_back(Property("presets." + name + ".stabilityDelay", ToString(stabilityDelay)));
	properties.push_back(Property("presets." + name + ".framesPerRevolution", ToString(framesPerRevolution)));
	properties.push_back(Property("presets." + name + ".generateXyz", ToString(generateXyz)));
	properties.push_back(Property("presets." + name + ".generateStl", ToString(generateStl)));
	properties.push_back(Property("presets." + name + ".generatePly", ToString(generatePly)));
	properties.push_back(Property("presets." + name + ".groundPlaneHeight", ToString(groundPlaneHeight)));
	properties.push_back(Property("presets." + name + ".laserMergeAction", ToString((int)laserMergeAction)));
	properties.push_back(Property("presets." + name + ".plyDataFormat", ToString((int)plyDataFormat)));

	if (isActivePreset)
	{
		properties.push_back(Property("presets." + name + ".active", "1"));
	}
}

void Preset::decodeProperties(const std::vector<Property>& properties, const std::string& name, bool& isActivePreset)
{
	isActivePreset = false;

	this->name = name;
	if (name.empty())
	{
		return;
	}

	const std::string prefix = "presets.";

	for (size_t iProp = 0; iProp < properties.size(); iProp++)
	{
		const Property& prop = properties[iProp];

		if (prop.name == prefix + name + ".laserSide")
		{
			laserSide = (Laser::LaserSide) ToInt(prop.value);
		}
		else if (prop.name == prefix + name + ".cameraMode")
		{
			cameraMode = (Camera::CameraMode) ToInt(prop.value);
		}
		else if (prop.name == prefix + name +  ".laserThreshold")
		{
			laserThreshold = ToReal(prop.value);
		}
		else if (prop.name == prefix + name +  ".minLaserWidth")
		{
			minLaserWidth = ToInt(prop.value);
		}
		else if (prop.name == prefix + name +  ".maxLaserWidth")
		{
			maxLaserWidth = ToInt(prop.value);
		}
		else if (prop.name == prefix + name + ".maxObjectSize")
		{
			maxObjectSize = ToReal(prop.value);
		}
		else if (prop.name == prefix + name + ".maxTriangleEdgeLength")
		{
			maxTriangleEdgeLength = ToReal(prop.value);
		}
		else if (prop.name == prefix + name + ".numLaserRowBins")
		{
			numLaserRowBins = ToInt(prop.value);
		}
		else if (prop.name == prefix + name + ".laserDelay")
		{
			laserDelay = ToInt(prop.value);
		}
		else if (prop.name == prefix + name + ".stabilityDelay")
		{
			stabilityDelay = ToInt(prop.value);
		}
		else if (prop.name == prefix + name + ".active")
		{
			isActivePreset = ToInt(prop.value) == 1;
		}
		else if (prop.name == prefix + name + ".framesPerRevolution")
		{
			framesPerRevolution = ToInt(prop.value);
		}
		else if (prop.name == prefix + name + ".generateXyz")
		{
			generateXyz = ToBool(prop.value);
		}
		else if (prop.name == prefix + name + ".generateStl")
		{
			generateStl = ToBool(prop.value);
		}
		else if (prop.name == prefix + name + ".generatePly")
		{
			generatePly = ToBool(prop.value);
		}
		else if (prop.name == prefix + name + ".groundPlaneHeight")
		{
			groundPlaneHeight = ToReal(prop.value);
		}
		else if (prop.name == prefix + name + ".laserMergeAction")
		{
			laserMergeAction = (LaserMergeAction) ToInt(prop.value);
		}
		else if (prop.name == prefix + name + ".plyDataFormat")
		{
			plyDataFormat = (PlyDataFormat) ToInt(prop.value);
		}
	}
}

std::vector<std::string> Preset::detectPresetNames(const std::vector<Property>& properties)
{
	std::set<std::string> presetNames;

	for (size_t iProp = 0; iProp < properties.size(); iProp++)
	{
		const Property& prop = properties[iProp];

		if (prop.name.find("presets.") == 0)
		{
			std::stringstream sstr;
			for (size_t iPos = std::string("presets.").length(); iPos < prop.name.length(); iPos++)
			{
				char c = prop.name[iPos];
				if (c == '.')
				{
					break;
				}
				else
				{
					sstr << c;
				}
			}

			// Store the name
			std::string name = sstr.str();
			if (!name.empty())
			{
				presetNames.insert(name);
			}
		}

	}

	std::vector<std::string> out;
	std::set<std::string>::iterator it = presetNames.begin();
	while (it != presetNames.end())
	{
		out.push_back(*it);
		it++;
	}

	return out;
}

}
