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
 * Reads and writes properties to a file.
 */
class PropertyReaderWriter
{
public:

	/** Reads the properties from the given properties file */
	static std::vector<Property> readProperties(const std::string& filename);

	/** Writes the given properties to the given file */
	static void writeProperties(const std::vector<Property>& properties, const std::string& filename);

	/** Writes the given properties to the given stream */
	static void writeProperties(std::ostream& out, const std::vector<Property>& properties);
};

}
