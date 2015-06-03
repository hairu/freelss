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


namespace freelss
{

// Forward declaration
struct MmalStillCallbackData;

/** Reads an image use the raspberry pi's MMAL interfaces */
class MmalStillCamera : public Camera
{
public:
	MmalStillCamera(int imageWidth, int imageHeight, bool enableBurstMode);
	~MmalStillCamera();

	Image * acquireImage();

	void releaseImage(Image * image);

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
	void createPreview();
	void createEncoder();
	void createCameraComponent();

private:

	int m_imageWidth;
	int m_imageHeight;
	MmalStillCallbackData * m_callbackData;

	CriticalSection m_cs;
	MMAL_COMPONENT_T * m_camera;
	MMAL_COMPONENT_T * m_preview;
	MMAL_COMPONENT_T * m_encoder;
	MMAL_POOL_T * m_pool;
	MMAL_PORT_T * m_previewPort;
	MMAL_PORT_T * m_videoPort;
	MMAL_PORT_T * m_stillPort;
	MMAL_PORT_T * m_targetPort;
};



}
