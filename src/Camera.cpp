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
#include "RaspistillCamera.h"
#include "RaspicamCamera.h"
#include "MmalStillCamera.h"
#include "MmalVideoCamera.h"
#include "PresetManager.h"

namespace freelss
{

CriticalSection Camera::m_cs = CriticalSection();
Camera * Camera::m_instance = NULL;
Camera::CameraType Camera::m_cameraType = Camera::CT_RASPICAM;
int Camera::m_reqImageWidth = 0;
int Camera::m_reqImageHeight = 0;

Camera::Camera()
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
			if (m_cameraType == Camera::CT_RASPISTILL)
			{
				m_instance = new RaspistillCamera();
			}
			else if (m_cameraType == Camera::CT_RASPICAM)
			{
				std::cout << "Creating video mode camera resolution=" << m_reqImageWidth << "x" << m_reqImageHeight << std::endl;
				m_instance = new RaspicamCamera(m_reqImageWidth, m_reqImageHeight);
			}
			else if (m_cameraType == Camera::CT_MMALSTILL)
			{
				std::cout << "Creating still mode camera resolution=" << m_reqImageWidth << "x" << m_reqImageHeight << std::endl;
				m_instance = new MmalStillCamera(m_reqImageWidth, m_reqImageHeight);
			}
			else if (m_cameraType == Camera::CT_MMALVIDEO)
			{
				m_instance = new MmalVideoCamera();
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

	switch (cameraMode)
	{
	case CM_STILL_5MP:
		type = CT_MMALSTILL;
		reqImageWidth = 2592;
		reqImageHeight = 1944;
		break;

	case CM_VIDEO_5MP:
		type = CT_RASPICAM;
		reqImageWidth = 2560;
		reqImageHeight = 1920;
		break;

	case CM_VIDEO_HD:
		type = CT_RASPICAM;
		reqImageWidth = 1600;
		reqImageHeight = 1200;
		break;

	case CM_VIDEO_1P2MP:
		type = CT_RASPICAM;
		reqImageWidth = 1280;
		reqImageHeight = 960;
		break;

	case CM_VIDEO_VGA:
		reqImageWidth = 640;
		reqImageHeight = 480;
		type = CT_RASPICAM;
		break;

	default:
		throw Exception("Unsupported camera mode");
	}

	m_cs.enter();
	try
	{
		// Set the new type
		if (m_cameraType != type ||
			m_reqImageWidth != reqImageWidth ||
			m_reqImageHeight != reqImageHeight)
		{
			delete m_instance;
			m_instance = NULL;
			m_cameraType = type;
			m_reqImageWidth = reqImageWidth;
			m_reqImageHeight = reqImageHeight;
		}
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


}
