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

#pragma once


namespace freelss
{

/**
 * Singleton class for controlling the lighting system
 */
class Lighting
{
public:
	/** Returns the singleton instance */
	static Lighting * get();

	/** Releases the singleton instance */
	static void release();

	Lighting();

	/** 0 (off) to 100 (full intensity) */
	void setIntensity(int intensity);

	int getIntensity() const;
private:
	static Lighting * m_instance;
	int m_pin;
	int m_intensity;
};

}
