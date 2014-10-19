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

/**
The 3D coordinate system is defined as positive X going to the right, 
positive Y going up into the air, and negative Z going into the scene (camera look direction).
This is also the standard OpenGL coordinate system.
The origin is the center of the turn table.
*/

// LIBPNG
#define PNG_DEBUG 3
#include <png.h>

// MMAL/BCM
// This mmal is from the RaspiCam package and was copied to /usr/local/include
#include "bcm_host.h"
#include "interface/vcos/vcos.h"
#include <mmal/mmal.h>
#include <mmal/mmal_buffer.h>
#include <mmal_util.h>
#include <mmal_util_params.h>
#include <mmal_default_components.h>
#include <mmal_connection.h>

// RASPICAM
#include <raspicam/raspicam.h>

// MICROHTTPD
#include <microhttpd.h>

// LIBJPEG
#include <jpeglib.h>

// SQLITE
#include <sqlite3.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <exception>
#include <math.h>
#include <pthread.h>
#include <list>
#include <map>
#include <sstream>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <sysexits.h>
#include <setjmp.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>

// Non-configurable settings
#define PI 3.14159265359

#define RADIANS_TO_DEGREES(r) ((r / (2 * PI)) * 360)
#define DEGREES_TO_RADIANS(d) ((d / 360.0) * (2 * PI))
#define ROUND(d)((int)(d + 0.5))
/*
#define CAMERA_IMAGE_HEIGHT 1944
#define CAMERA_IMAGE_WIDTH 2592
#define CAMERA_IMAGE_COMPONENTS 3         // RGB
#define CAMERA_SENSOR_WIDTH 3.629
#define CAMERA_SENSOR_HEIGHT 2.722        // http://elinux.org/Rpi_Camera_Module reports it as 3.67 x 2.74 mm
#define CAMERA_FOCAL_LENGTH 3.6
*/

#define ABS(a)	   (((a) < 0) ? -(a) : (a))

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MIN3
#define MIN3(a, b, c) (MIN(MIN(a, b), c))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MAX3
#define MAX3(a, b, c) (MAX(MIN(a, b), c))
#endif

namespace scanner
{

typedef std::string Exception;
typedef unsigned char byte;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef float real32;
typedef double real64;
typedef float real;

/** 
 * 2D Pixel Location.  
 * Image coordinates are from 0 to (MAX - 1). 
 * Pixel layout is top to bottom and left to right.
 */
struct PixelLocation
{	
	real x;
	real y;
};

/** 3D Real Location */
struct Vector3
{
	void normalize()
	{
		real len = sqrt(x * x + y * y + z * z);
		x /= len;
		y /= len;
		z /= len;
	}
	
	real dot(const Vector3& a) const
	{
		return x * a.x + y * a.y + z * a.z;
	}

	real x;
	real y;
	real z;
};

struct ColoredPoint
{
	real x;
	real y;
	real z;

	Vector3 normal;

	unsigned char r;
	unsigned char g;
	unsigned char b;
};

/** Color in the Hue, Saturation, Value format */
struct Hsv
{
	float h;
	float s;
	float v;
};

struct Plane
{
	/** The plane normal */
	Vector3 normal;
	
	/** A point in the plane */
	Vector3 point;
};

struct Ray
{
	Vector3 origin;
	Vector3 direction;
};

struct NeutralFileRecord
{
	PixelLocation pixel;
	ColoredPoint point;
	real rotation;
	int step;
	int laserSide;
	int pseudoStep;
};

struct ScanResultFile
{
	std::string extension;
	time_t creationTime;
	__off_t fileSize;
};

struct ScanResult
{
	time_t getScanDate() const;
	std::vector<ScanResultFile> files;
};

/** Returns the current point in time in ms */
double GetTimeInSeconds();
}

// Forward declaration
struct sqlite3;
struct sqlite3_stmt;


// Include wiringPi
#include <wiringPi.h>


