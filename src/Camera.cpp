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
#include "Camera.h"
#include "MmalStillCamera.h"
#include "MmalVideoCamera.h"
#include "PresetManager.h"
#include "Thread.h"

namespace freelss
{

CriticalSection Camera::m_cs;
Camera * Camera::m_instance = NULL;
Camera::CameraType Camera::m_cameraType = Camera::CT_MMALVIDEO;
int Camera::m_reqImageWidth = 0;
int Camera::m_reqImageHeight = 0;
int Camera::m_reqFrameRate = 0;

// 5MP Video - 360000 laser delay
// 5MP Still -  90000 laser delay
// 1.9MP     -  90000 laser delay
// 1.2MP     -  90000 laser delay
// 0.3MP     -  90000 laser delay

Camera::Camera() :
	m_nextAcquisitionTimeSec(0)
{
	// Do nothing
}

Camera::~Camera()
{
	// Do nothing
}

Camera * Camera::getInstance()
{
	m_cs.enter();
	try
	{
		if (m_instance == NULL)
		{
			if (m_cameraType == Camera::CT_MMALSTILL)
			{
				Preset& preset = PresetManager::get()->getActivePreset();
				std::cout << "Creating still mode camera resolution=" << m_reqImageWidth << "x" << m_reqImageHeight << std::endl;
				m_instance = new MmalStillCamera(m_reqImageWidth, m_reqImageHeight, preset.enableBurstModeForStillImages);
			}
			else if (m_cameraType == Camera::CT_MMALVIDEO)
			{
				std::cout << "Creating MMAL video mode camera resolution=" << m_reqImageWidth << "x" << m_reqImageHeight << std::endl;
				m_instance = new MmalVideoCamera(m_reqImageWidth, m_reqImageHeight, m_reqFrameRate);
			}
			else
			{
				throw Exception("Unsupported camera type");
			}
		}
	}
	catch (...)
	{
		m_cs.leave();
		throw;
	}
	m_cs.leave();

	return m_instance;
}

void Camera::release()
{

	m_cs.enter();
	try
	{
		delete m_instance;
		m_instance = NULL;
	}
	catch (...)
	{
		m_cs.leave();
		throw;
	}
	m_cs.leave();
}

void Camera::reinitialize()
{
	// Read the camera type from settings
	Camera::CameraMode cameraMode = PresetManager::get()->getActivePreset().cameraMode;
	Camera::CameraType type;
	int reqImageWidth;
	int reqImageHeight;
	int reqFrameRate;

	switch (cameraMode)
	{
	case CM_STILL_5MP:
		type = CT_MMALSTILL;
		reqImageWidth = 2592;
		reqImageHeight = 1944;
		reqFrameRate = 15;
		break;

	case CM_VIDEO_5MP:
		type = CT_MMALVIDEO;
		reqImageWidth = 2592;
		reqImageHeight = 1944;
		reqFrameRate = 15;
		break;

	case CM_VIDEO_HD:
		type = CT_MMALVIDEO;
		reqImageWidth = 1600;
		reqImageHeight = 1200;
		reqFrameRate = 15;
		break;

	case CM_VIDEO_1P2MP:
		type = CT_MMALVIDEO;
		reqImageWidth = 1280;
		reqImageHeight = 960;
		reqFrameRate = 15;
		break;

	case CM_VIDEO_VGA:
		type = CT_MMALVIDEO;
		reqImageWidth = 640;
		reqImageHeight = 480;
		reqFrameRate = 15;
		break;

	default:
		throw Exception("Unsupported camera mode");
	}


	m_cs.enter();
	try
	{
		// Set the new type
		delete m_instance;
		m_instance = NULL;
		m_cameraType = type;
		m_reqImageWidth = reqImageWidth;
		m_reqImageHeight = reqImageHeight;
		m_reqFrameRate = reqFrameRate;
	}
	catch (...)
	{
		m_cs.leave();
		throw;
	}
	m_cs.leave();

	// Create an instance of the new type
	getInstance();
}

bool Camera::acquireJpeg(byte* buffer, unsigned * size)
{
	// Acquire an image
	Image * image = NULL;
	bool retVal = false;

	try
	{
		image = acquireImage();

		if (image == NULL)
		{
			throw Exception("NULL image in Camera::acquireJpeg");
		}

		// Make sure the buffer is big enough
		if (* size >= image->getPixelBufferSize())
		{
			// Convert the image to a JPEG
			Image::convertToJpeg(* image, buffer, size);
			retVal = true;
		}
		else
		{
			* size = image->getPixelBufferSize();
			retVal = false;
		}

		releaseImage(image);
	}
	catch (...)
	{
		releaseImage(image);
		throw;
	}

	return retVal;
}

void Camera::setAcquisitionDelay(double acquisitionDelaySec)
{
	double seconds = GetTimeInSeconds() + acquisitionDelaySec;
	m_nextAcquisitionTimeSec = MAX(m_nextAcquisitionTimeSec, seconds);
}

void Camera::handleAcquisitionDelay()
{
	double sleepTime = m_nextAcquisitionTimeSec - GetTimeInSeconds();
	if (sleepTime > 0)
	{
		//std::cout << "Camera::handleAcquisitionDelay() - Sleeping for " << sleepTime << " seconds" << std::endl;
		Thread::usleep((unsigned long) (sleepTime * 1000000.0));
		m_nextAcquisitionTimeSec = 0;
	}
}

}
