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

// Forward declaration
struct MHD_Daemon;

namespace freelss
{

class Scanner;


class HttpServer
{
public:
	~HttpServer();

	/** Start the HTTP server */
	void start(int port);

	void stop();
	void reinitialize();
	static HttpServer * get();
	static void release();

	Scanner * getScanner();
private:
	HttpServer();
	MHD_Daemon *m_daemon;
	Scanner * m_scanner;
	static HttpServer * m_instance;
};
}
