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
#include "PropertyReaderWriter.h"


namespace freelss
{

std::vector<Property> PropertyReaderWriter::readProperties(const std::string& filename)
{
	std::cout << "Reading properties file: " << filename << std::endl;

	std::vector<Property> properties;

	std::ifstream fin (filename.c_str());
	if (fin.is_open() == false)
	{
		throw Exception("Error opening properties file for reading: " + filename);
	}


	std::string line;
	while (std::getline(fin, line))
	{
		size_t delimPos = line.find("=");
		if (delimPos != std::string::npos && delimPos < line.size() - 1 && delimPos > 0)
		{
			Property property;
			property.name = line.substr(0, delimPos);
			property.value = line.substr(delimPos + 1);
			properties.push_back(property);
		}
	}

	return properties;
}


void PropertyReaderWriter::writeProperties(const std::vector<Property>& properties, const std::string& filename)
{
	std::ofstream fout (filename.c_str());
	if (fout.is_open() == false)
	{
		throw Exception("Error opening properties file for writing: " + filename);
	}

	writeProperties(fout, properties);
}

void PropertyReaderWriter::writeProperties(std::ostream& out, const std::vector<Property>& properties)
{
	for (size_t iProp = 0; iProp < properties.size(); iProp++)
	{
		const Property& prop = properties[iProp];
		out << prop.name << "=" << prop.value << std::endl;
	}
}

} // end ns
