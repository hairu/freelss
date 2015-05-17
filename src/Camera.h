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

namespace freelss
{

/** Camera interface */
class Camera
{
public:

	/** The type of camera instance to create */
	enum CameraType { CT_RASPICAM, CT_MMALSTILL, CT_MMALVIDEO, CT_RASPISTILL};

	/** The camera mode */
	enum CameraMode { CM_STILL_5MP, CM_VIDEO_5MP, CM_VIDEO_HD, CM_VIDEO_1P2MP, CM_VIDEO_VGA};

	/** Returns the singleton instance */
	static Camera * getInstance();

	/** Releases the singleton instance */
	static void release();

	/** Reinitialize the singleton with a different camera implementation */
	static void reinitialize();

	/** Destructor */
	virtual ~Camera();

	/** Reads an image from the camera */
	virtual void acquireImage(Image * image) = 0;

	/**
	 *  Acquires an image as a JPEG file.
	 *  @return Returns true if buffer is populated with the JPEG data. Returns false
	 *      if buffer is not big enough to store the JPEG.
	 *  @param buffer - The buffer to write the JPEG contents to.
	 *  @param size [IN/OUT] - The size of the passed in buffer.  If false is returned,
	 *      this parameter is set to the size that the buffer needs to be.
	 */
	virtual bool acquireJpeg(byte* buffer, unsigned * size) = 0;

	/** Returns the height of the image that this camera takes. */
	virtual int getImageHeight() const = 0;

	/** Returns the width of the image that this camera takes */
	virtual int getImageWidth() const = 0;

	/** Returns the number of image components */
	virtual int getImageComponents() const = 0;

	/** Returns the width of the sensor in mm */
	virtual real getSensorWidth() const = 0;

	/** Returns the height of the sensor in mm */
	virtual real getSensorHeight() const = 0;

	/** Returns the focal length of the camera in mm */
	virtual real getFocalLength() const = 0;

protected:
	Camera();

private:
	/** The singleton instance */
	static Camera * m_instance;

	/** The type of camera that m_instance is */
	static CameraType m_cameraType;

	static CriticalSection m_cs;

	/** The requested image width */
	static int m_reqImageWidth;

	/** The requested image height */
	static int m_reqImageHeight;

	/** The requested frame rate */
	static int m_reqFrameRate;
};

}
