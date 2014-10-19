/*
 ****************************************************************************
 *  Copyright (c) 2014 Uriah Liggett <hairu526@gmail.com>                   *
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


namespace scanner
{

class Settings
{
public:
	~Settings();
	static Settings * get();
	static void initialize(const char * filename);
	static void release();

	/** Read an integer from the settings database */
	int readInt(const char * tableName, const char * columnName);

	/** Read a floating point value from the settings database */
	real readReal(const char * tableName, const char * columnName);

	/** Updates the given real value in the settings database */
	void writeReal(const char * tableName, const char * columnName, real value);

	/** Updates the given integer value in the settings database */
	void writeInt(const char * tableName, const char * columnName, int value);

	/** The directory where scans are written to */
	static const char * SCAN_OUTPUT_DIR;

	/** The directory where debug images are written to */
	static const char * DEBUG_OUTPUT_DIR;

	// General Settings Table
	static const char * GENERAL_SETTINGS;

	// General Settings Columns
	static const char * CAMERA_X;
	static const char * CAMERA_Y;
	static const char * CAMERA_Z;
	static const char * RIGHT_LASER_X;
	static const char * RIGHT_LASER_Y;
	static const char * RIGHT_LASER_Z;
	static const char * RIGHT_LASER_PIN;
	static const char * LEFT_LASER_X;
	static const char * LEFT_LASER_Y;
	static const char * LEFT_LASER_Z;
	static const char * LEFT_LASER_PIN;
	static const char * LASER_MAGNITUDE_THRESHOLD;
	static const char * LASER_DELAY;
	static const char * LASER_ON_VALUE;
	static const char * LASER_SELECTION;
	static const char * STABILITY_DELAY;
	static const char * MAX_LASER_WIDTH;
	static const char * MIN_LASER_WIDTH;
	static const char * STEPS_PER_REVOLUTION;

	// A4988 Table
	static const char * A4988_SETTINGS;

	// A4988 Columns
	static const char * ENABLE_PIN;
	static const char * STEP_PIN;
	static const char * DIRECTION_PIN;
	static const char * RESPONSE_DELAY;
	static const char * STEP_DELAY;

private:
	Settings(const char * filename);
	void createDatabase(const char * filename);

	/** The singleton instance */
	static Settings * m_instance;

	/** The SQL for creating the database */
	static const char * CREATE_DATABASE_SQL;

	/** The Sqlite database */
	sqlite3 * m_db;
};


}
