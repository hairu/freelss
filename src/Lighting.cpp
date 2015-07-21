/*
 ****************************************************************************
 *  Copyright (c) 2014 Uriah Liggett <freelaserscanner@gmail.com>           *
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
#include "Lighting.h"
#include "Setup.h"
#include <softPwm.h>

namespace freelss
{
Lighting * Lighting::m_instance = NULL;

Lighting * Lighting::get()
{
	if (Lighting::m_instance == NULL)
	{
		Lighting::m_instance = new Lighting();
	}

	return Lighting::m_instance;
}

void Lighting::release()
{
	delete Lighting::m_instance;
	Lighting::m_instance = NULL;
}

Lighting::Lighting() :
	m_pin(-1),
	m_intensity(0)
{
	Setup * setup = Setup::get();
	m_pin = setup->lightingPin;

	if (setup->enableLighting)
	{
		softPwmCreate(m_pin, m_intensity, 100);
	}
}


void Lighting::setIntensity(int intensity)
{
	if (intensity >= 0 && intensity <= 100)
	{
		softPwmWrite (m_pin, intensity);
		m_intensity = intensity;
	}
}

int Lighting::getIntensity() const
{
	return m_intensity;
}

}
