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
#include "Progress.h"

namespace freelss
{


Progress::Progress() :
	m_percent(0),
	m_label(""),
	m_cs()
{
	// Do nothing
}


real Progress::getPercent()
{
	real out;

	m_cs.enter();
	out = m_percent;
	m_cs.leave();

	return out;
}

std::string Progress::getLabel()
{
	std::string out;

	m_cs.enter();
	out = m_label;
	m_cs.leave();

	return out;
}

void Progress::setPercent(real percent)
{
	m_cs.enter();
	m_percent = percent;
	m_cs.leave();
}

void Progress::setLabel(const std::string& label)
{
	m_cs.enter();
	std::cout << label << "..." << std::endl;
	m_label = label;
	m_cs.leave();
}

}
