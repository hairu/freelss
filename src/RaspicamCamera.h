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
#include "Image.h"
#include "CriticalSection.h"
#include "Camera.h"

// Forward declaration
namespace raspicam
{
	class RaspiCam;
}

namespace freelss
{

class RaspicamCamera : public Camera
{
public:
	RaspicamCamera(int imageWidth = 1280, int imageHeight = 960);

	~RaspicamCamera();

	void acquireImage(Image * image);

	bool acquireJpeg(byte* buffer, unsigned * size);

	/** Returns the height of the image that this camera takes. */
	int getImageHeight() const;

	/** Returns the width of the image that this camera takes */
	int getImageWidth() const;

	/** Returns the number of image components */
	int getImageComponents() const;

	/** Returns the width of the sensor in mm */
	real getSensorWidth() const;

	/** Returns the height of the sensor in mm */
	real getSensorHeight() const;

	/** Returns the focal length of the camera in mm */
	real getFocalLength() const;

private:

	/** Writes the JPEG to disk for debugging purposes */
	void writeJpegToDisk(byte * buffer, unsigned size);

private:
	const int m_imageWidth;
	const int m_imageHeight;
	CriticalSection m_cs;
	raspicam::RaspiCam * m_raspicam;
	int m_numWritten;
};

}
