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

#include "Preset.h"

namespace freelss
{

/**
 * Manages access to the Preset instances.
 */
class PresetManager
{
public:

	static PresetManager * get();
	static void release();

	/** Returns the preset that is currently active */
	Preset& getActivePreset();

	/** Sets the given preset as the active preset */
	void setActivePreset(int presetId);

	/** Removes the active preset */
	void removeActivePreset();

	/** Returns the presets */
	const std::vector<Preset>& getPresets();

	/** Adds the preset */
	int addPreset(const Preset& preset);

	/** Encodes property information to the properties vector */
	void encodeProperties(std::vector<Property>& properties);

	/** Decodes property information from the given vector  */
	void decodeProperties(const std::vector<Property>& properties);

private:

	PresetManager();

	static PresetManager * m_instance;

	/** The Presets */
	std::vector<Preset> m_presets;

	/** The ID of the next preset */
	int m_nextId;

	/** The index of the preset that is currently active */
	int m_activePresetIndex;
};

}
