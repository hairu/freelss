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
#include "RaspicamCamera.h"
#include "Thread.h"


namespace freelss
{

RaspicamCamera::RaspicamCamera(int imageWidth, int imageHeight) :
	m_imageWidth(imageWidth), // was 1280
	m_imageHeight(imageHeight), // was 960
	m_cs(),
	m_raspicam(NULL),
	m_numWritten(0)
{
	m_raspicam = new raspicam::RaspiCam();
	m_raspicam->setWidth(m_imageWidth);
	m_raspicam->setHeight(m_imageHeight);
	m_raspicam->setCaptureSize(m_imageWidth, m_imageHeight);
	m_raspicam->setVerticalFlip(true);
	m_raspicam->setHorizontalFlip(true);
	//m_raspicam->setSaturation(-100);
	//m_raspicam->setSharpness(-100);
	
	if (!m_raspicam->open())
	{
		throw Exception("Error opening camera");
	}

	std::cout << "Camera intialized" << std::endl;
}

RaspicamCamera::~RaspicamCamera()
{
	if (m_raspicam != NULL)
	{
		std::cout << "Destroying RaspicamCamera" << std::endl;

		// Release the camera
		m_raspicam->release();

		Thread::usleep(1000000);

		delete m_raspicam;
	}
}

void RaspicamCamera::acquireImage(Image * image)
{
	m_cs.enter();
	try
	{
		m_raspicam->grab();

		unsigned pixelBufferSize = image->getPixelBufferSize();
		if (pixelBufferSize < m_raspicam->getImageTypeSize (raspicam::RASPICAM_FORMAT_RGB))
		{
			throw Exception("Pixel buffer is not big enough to hold the camera image");
		}

		m_raspicam->retrieve (image->getPixels(), raspicam::RASPICAM_FORMAT_IGNORE);

#if 0
		// Write it to disk as a jpeg to DEBUG
		{
			byte * buffer = new byte[pixelBufferSize];
			Image::convertToJpeg(* image, buffer, &pixelBufferSize);
			writeJpegToDisk(buffer, pixelBufferSize);
			delete [] buffer;
		}
#endif
		// Increment the number of images we have written
		m_numWritten++;
	}
	catch(...)
	{
		m_cs.leave();
		throw;
	}

	m_cs.leave();
}

void RaspicamCamera::writeJpegToDisk(byte * buffer, unsigned size)
{
	std::stringstream sstr;
	sstr << "/home/pi/raspistill_" << m_numWritten << ".jpg";

	std::string filename = sstr.str();
	std::ofstream fout (filename.c_str());
	if (!fout.is_open())
	{
		throw Exception("Error opening file: " + filename);
	}

	fout.write((const char *)buffer, size);
	fout.close();
}

bool RaspicamCamera::acquireJpeg(byte* buffer, unsigned * size)
{
	// Make sure the buffer is big enough
	Image image;
	if (* size < image.getPixelBufferSize())
	{
		* size = image.getPixelBufferSize();
		return false;
	}

	// Acquire an image
	acquireImage(&image);

	// Convert the image to a JPEG
	Image::convertToJpeg(image, buffer, size);

	return true;
}


int RaspicamCamera::getImageHeight() const
{
	return m_imageHeight;
}

int RaspicamCamera::getImageWidth() const
{
	return m_imageWidth;
}

int RaspicamCamera::getImageComponents() const
{
	return 3;
}

real RaspicamCamera::getSensorWidth() const
{
	// Only 49.382716% of the full sensor width is used
	// return 1.792098765;

	return 3.629;
}

real RaspicamCamera::getSensorHeight() const
{
	// Only 49.382716% of the full sensor height is used
	//return 1.344197531;

	return 2.722;
}

real RaspicamCamera::getFocalLength() const
{
	return 3.6;
}

}
