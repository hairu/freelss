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

#include "Main.h"
#include "Settings.h"


namespace scanner
{

Settings * Settings::m_instance = NULL;

static int int_callback(void *data, int argc, char **argv, char **azColName)
{
   if(argc == 0)
   {
	   return -1;
   }

   int * value = (int *) data;
   * value = atoi(argv[0]);

   return 0;
}

static int real_callback(void *data, int argc, char **argv, char **azColName)
{
   if(argc == 0)
   {
	   return -1;
   }

   real * value = (real *) data;
   * value = atof(argv[0]);

   return 0;
}

static int print_callback(void *data, int argc, char **argv, char **azColName)
{
   if(argc == 0)
   {
	   return -1;
   }

   for (int i = 0; i < argc; i++)
   {
	   std::cout << azColName[i] << ": " << argv[i] << std::endl;
   }

   return 0;
}

const char * Settings::SCAN_OUTPUT_DIR = "/scans";
const char * Settings::DEBUG_OUTPUT_DIR = "/debug";
const char * Settings::GENERAL_SETTINGS = "GENERAL_SETTINGS";
const char * Settings::CAMERA_X = "CAMERA_X";
const char * Settings::CAMERA_Y = "CAMERA_Y";
const char * Settings::CAMERA_Z = "CAMERA_Z";
const char * Settings::CAMERA_MODE = "CAMERA_MODE";
const char * Settings::RIGHT_LASER_X = "RIGHT_LASER_X";
const char * Settings::RIGHT_LASER_Y = "RIGHT_LASER_Y";
const char * Settings::RIGHT_LASER_Z = "RIGHT_LASER_Z";
const char * Settings::RIGHT_LASER_PIN = "RIGHT_LASER_PIN";
const char * Settings::LEFT_LASER_X = "LEFT_LASER_X";
const char * Settings::LEFT_LASER_Y = "LEFT_LASER_Y";
const char * Settings::LEFT_LASER_Z = "LEFT_LASER_Z";
const char * Settings::LEFT_LASER_PIN = "LEFT_LASER_PIN";
const char * Settings::LASER_MAGNITUDE_THRESHOLD = "LASER_MAGNITUDE_THRESHOLD";
const char * Settings::LASER_DELAY = "LASER_DELAY";
const char * Settings::LASER_ON_VALUE = "LASER_ON_VALUE";
const char * Settings::LASER_SELECTION = "LASER_SELECTION";
const char * Settings::STABILITY_DELAY = "STABILITY_DELAY";
const char * Settings::MAX_LASER_WIDTH = "MAX_LASER_WIDTH";
const char * Settings::MIN_LASER_WIDTH = "MIN_LASER_WIDTH";
const char * Settings::STEPS_PER_REVOLUTION = "STEPS_PER_REVOLUTION";
const char * Settings::A4988_SETTINGS = "A4988_SETTINGS";
const char * Settings::ENABLE_PIN = "ENABLE_PIN";
const char * Settings::STEP_PIN = "STEP_PIN";
const char * Settings::DIRECTION_PIN = "DIRECTION_PIN";
const char * Settings::RESPONSE_DELAY = "RESPONSE_DELAY";
const char * Settings::STEP_DELAY = "STEP_DELAY";

Settings::Settings(const char * filename) :
	m_db(NULL)
{
	// If the file doesn't exists, write it
	if (access(filename, F_OK) == -1)
	{
		std::cout << "Creating initial settings file: " << filename << std::endl;
		createDatabase(filename);
	}
	else if (sqlite3_open(filename, &m_db) != 0)
	{
		throw Exception(std::string("Could not open sqlite database: ") + filename);
	}
}

Settings::~Settings()
{
	if (m_db != NULL)
	{
		sqlite3_close(m_db);
	}
}

int Settings::readInt(const char * tableName, const char * columnName)
{
	int value = 0;
	char * errMsg = 0;
	std::string sql = std::string("SELECT ") + columnName + std::string(" FROM ") + tableName;
	int rc = sqlite3_exec(m_db, sql.c_str(), int_callback, (void*)&value, &errMsg);
	if( rc != SQLITE_OK ){
		std::string error = errMsg;
		sqlite3_free(errMsg);
	    throw Exception("SQL Error: " + error);
	}

	return value;
}

real Settings::readReal(const char * tableName, const char * columnName)
{
	real value = 0;
	char * errMsg = 0;

	std::string sql = std::string("SELECT ") + columnName + std::string(" FROM ") + tableName;
	int rc = sqlite3_exec(m_db, sql.c_str(), real_callback, (void*)&value, &errMsg);
	if( rc != SQLITE_OK ){
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception("SQL Error: " + error);
	}

	return value;
}

void Settings::writeReal(const char * tableName, const char * columnName, real value)
{
	std::stringstream sstr;
	sstr << value;

	std::string valueStr = sstr.str();
	char * errMsg = 0;

	std::string sql = std::string("UPDATE ") + tableName + std::string(" SET ") + columnName + std::string(" = ") + valueStr;
	int rc = sqlite3_exec(m_db, sql.c_str(), print_callback, (void*)&value, &errMsg);
	if( rc != SQLITE_OK ){
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception("SQL Error: " + error);
	}
}

void Settings::writeInt(const char * tableName, const char * columnName, int value)
{
	std::stringstream sstr;
	sstr << value;

	std::string valueStr = sstr.str();
	char * errMsg = 0;

	std::string sql = std::string("UPDATE ") + tableName + std::string(" SET ") + columnName + std::string(" = ") + valueStr;
	int rc = sqlite3_exec(m_db, sql.c_str(), print_callback, (void*)&value, &errMsg);
	if( rc != SQLITE_OK ){
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception("SQL Error: " + error);
	}
}

Settings * Settings::get()
{
	if (Settings::m_instance == NULL)
	{
		throw Exception("Singleton instance is NULL");
	}

	return Settings::m_instance;
}

void Settings::initialize(const char * filename)
{
	std::cout << "Reading settings from: " << filename << std::endl;
	if (Settings::m_instance == NULL)
	{
		Settings::m_instance = new Settings(filename);
	}
}

void Settings::release()
{
	delete Settings::m_instance;
	Settings::m_instance = NULL;
}

void Settings::createDatabase(const char * filename)
{
	if (sqlite3_open(filename, &m_db) != 0)
	{
		throw Exception(std::string("Could not open sqlite database: ") + filename);
	}

	char * errMsg = 0;
	int rc = sqlite3_exec(m_db, CREATE_DATABASE_SQL, print_callback, NULL, &errMsg);
	if( rc != SQLITE_OK )
	{
		std::string error = errMsg;
		sqlite3_free(errMsg);
		throw Exception("SQL Error: " + error);
	}
}

const char * Settings::CREATE_DATABASE_SQL = "\n\
CREATE TABLE GENERAL_SETTINGS (\n\
	CAMERA_X REAL,                        -- X-compoment of camera location in mm. ie: The camera is always at X = 0.\n\
	CAMERA_Y REAL,                        -- Y-component of camera location in mm. ie: The distance from camera center to the XZ plane\n\
	CAMERA_Z REAL,                        -- Z-component of camera location in mm. ie: The distance from camera center to origin\n\
	CAMERA_MODE INTEGER,                  -- The camera implementation to scan with.  Set to 0 for video mode, 1 for still mode\n\
    RIGHT_LASER_X REAL,                         -- X coordinate of the laser in mm.  This is how far the laser is from the camera. \n\
	RIGHT_LASER_Y REAL,                         -- Y coordinate of the laser\n\
	RIGHT_LASER_Z REAL,                         -- Z coordinate of the laser.  The laser is parallel with the camera focal point\n\
	RIGHT_LASER_PIN INTEGER,                    -- The wiringPi pin number for controlling the laser\n\
	LEFT_LASER_X REAL,                         -- X coordinate of the laser in mm.  This is how far the laser is from the camera. \n\
	LEFT_LASER_Y REAL,                         -- Y coordinate of the laser\n\
	LEFT_LASER_Z REAL,                         -- Z coordinate of the laser.  The laser is parallel with the camera focal point\n\
	LEFT_LASER_PIN INTEGER,                    -- The wiringPi pin number for controlling the laser\n\
	LASER_MAGNITUDE_THRESHOLD REAL,       -- How bright the difference between a laser on and laser off pixel must be in order to be detected as part of the laser\n\
	LASER_DELAY INTEGER,                  -- The time to delay after turning the laser on or off\n\
	LASER_ON_VALUE INTEGER,               -- Set to 1 if setting the laser pin HIGH turns if on and 0 if LOW turns it on\n\
	LASER_SELECTION INTEGER,              -- 0 for left laser, 1 for right laser, 2 for both lasers.\n\
	STABILITY_DELAY INTEGER,              -- The time in microseconds to delay after moving the turntable and before taking a picture\n\
	MAX_LASER_WIDTH INTEGER,              -- Maximum laser width in pixels\n\
	MIN_LASER_WIDTH INTEGER,              -- Minimum laser width in pixels\n\
	STEPS_PER_REVOLUTION INTEGER          -- The number of motor steps before the turntable (not the motor) spins 360 degrees\n\
);\n\
\n\
CREATE TABLE A4988_SETTINGS (\n\
	ENABLE_PIN INTEGER,                   -- The wiringPi pin number for enabling the stepper motor\n\
	STEP_PIN INTEGER,                     -- The wiringPi pin number for stepping the stepper motor\n\
	DIRECTION_PIN INTEGER,                -- The wiringPi pin number for the stepper motor direction or rotation\n\
	RESPONSE_DELAY INTEGER,               -- The time it takes for the stepper controller to recognize a pin value change in microseconds\n\
	STEP_DELAY INTEGER                    -- The amount of time between steps in microseconds\n\
);\n\
\n\
INSERT INTO GENERAL_SETTINGS (\n\
	CAMERA_X, CAMERA_Y, CAMERA_Z, CAMERA_MODE,\n\
	RIGHT_LASER_X, RIGHT_LASER_Y, RIGHT_LASER_Z,\n\
	LEFT_LASER_X, LEFT_LASER_Y, LEFT_LASER_Z,\n\
	LASER_MAGNITUDE_THRESHOLD, LASER_DELAY, RIGHT_LASER_PIN, LEFT_LASER_PIN, LASER_ON_VALUE,\n\
	LASER_SELECTION, STABILITY_DELAY, MAX_LASER_WIDTH, MIN_LASER_WIDTH, STEPS_PER_REVOLUTION\n\
	) VALUES (\n\
	0, 44.45, 304.8, 3,\n\
	127, 44.45, 304.8,\n\
	-127, 44.45, 304.8,\n\
	3, 90000, 0, 4, 1,\n\
	2, 0, 60, 5, 6400\n\
);\n\
INSERT INTO A4988_SETTINGS (ENABLE_PIN, STEP_PIN, DIRECTION_PIN, RESPONSE_DELAY, STEP_DELAY) VALUES\n\
                           (1, 2, 3, 2, 5000);";
}
