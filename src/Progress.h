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

#include "CriticalSection.h"

namespace freelss
{

/** Tracks the progress of a task in a thread-safe manner. */
class Progress
{
public:
	Progress();

	real getPercent();
	std::string getLabel();

	void setPercent(real percent);
	void setLabel(const std::string& label);

private:
	// DISABLE COPY SEMANTICS
	Progress(const Progress&) { }
	Progress& operator = (const Progress&) { return *this; }

	real m_percent;
	std::string m_label;
	CriticalSection m_cs;
};

}
