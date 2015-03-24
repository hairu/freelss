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
#include "PresetManager.h"

namespace freelss
{
PresetManager * PresetManager::m_instance = NULL;

PresetManager::PresetManager() :
	m_presets(),
	m_nextId(0),
	m_activePresetIndex(-1)
{
	// Do nothing
}

PresetManager * PresetManager::get()
{
	if (PresetManager::m_instance == NULL)
	{
		PresetManager::m_instance = new PresetManager();
	}

	return PresetManager::m_instance;
}

void PresetManager::release()
{
	delete PresetManager::m_instance;
	PresetManager::m_instance = NULL;
}

Preset& PresetManager::getActivePreset()
{
	if (m_activePresetIndex < 0 || m_activePresetIndex >= (int)m_presets.size())
	{
		throw Exception("No active preset found: " + ToString(m_activePresetIndex));
	}

	return m_presets[m_activePresetIndex];
}

void PresetManager::setActivePreset(int presetId)
{
	for (size_t iPro = 0; iPro < m_presets.size(); iPro++)
	{
		if (m_presets[iPro].id == presetId)
		{
			m_activePresetIndex = (int) iPro;
		}
	}
}

const std::vector<Preset>& PresetManager::getPresets()
{
	return m_presets;
}

void PresetManager::removeActivePreset()
{
	// Don't allow the last preset to be removed
	if (m_presets.size() <= 1)
	{
		return;
	}

	// Ensure that we have an active preset
	if (m_activePresetIndex < 0 || m_activePresetIndex >= (int)m_presets.size())
	{
		return;
	}

	// Remove the preset
	m_presets.erase(m_presets.begin() + m_activePresetIndex);

	// Make the first preset the new active one
	m_activePresetIndex = 0;
}

int PresetManager::addPreset(const Preset& inPreset)
{
	Preset preset = inPreset;
	preset.id = m_nextId++;

	m_presets.push_back(preset);

	return preset.id;
}

void PresetManager::encodeProperties(std::vector<Property>& properties)
{
	for (size_t iPro = 0; iPro < m_presets.size(); iPro++)
	{
		m_presets[iPro].encodeProperties(properties, (int)iPro == m_activePresetIndex);
	}
}

void PresetManager::decodeProperties(const std::vector<Property>& properties)
{
	// Clear the existing presets
	m_presets.clear();
	m_activePresetIndex = -1;

	bool isActivePreset = false;

	std::vector<std::string> presetNames = Preset::detectPresetNames(properties);
	for (size_t iName = 0; iName < presetNames.size(); iName++)
	{
		Preset preset;
		preset.decodeProperties(properties, presetNames[iName], isActivePreset);
		preset.id = m_nextId++;

		m_presets.push_back(preset);

		if (isActivePreset || m_activePresetIndex == -1)
		{
			m_activePresetIndex = (int) m_presets.size() - 1;
		}
	}
}

}
