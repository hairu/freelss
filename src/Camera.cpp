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
#include "Camera.h"
#include "RaspistillCamera.h"
#include "RaspicamCamera.h"
#include "MmalStillCamera.h"
#include "MmalVideoCamera.h"

namespace scanner
{

CriticalSection Camera::m_cs = CriticalSection();
Camera * Camera::m_instance = NULL;
//Camera::CameraType Camera::m_cameraType = Camera::CT_RASPICAM;
//Camera::CameraType Camera::m_cameraType = Camera::CT_RASPISTILL;
Camera::CameraType Camera::m_cameraType = Camera::CT_MMALSTILL;

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
				m_instance = new RaspicamCamera();
			}
			else if (m_cameraType == Camera::CT_MMALSTILL)
			{
				m_instance = new MmalStillCamera();
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

void Camera::reinitialize(CameraType type)
{
	m_cs.enter();
	try
	{
		// Set the new type
		if (m_cameraType != type)
		{
			delete m_instance;
			m_instance = NULL;
			m_cameraType = type;
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
