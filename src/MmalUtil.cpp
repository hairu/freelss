/*
 ****************************************************************************
 *  Copyright (c) 2016 Uriah Liggett <freelaserscanner@gmail.com>           *
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
 ****************************************************************************/
#include "Main.h"
#include "MmalUtil.h"
#include "Logger.h"

namespace freelss
{
MmalUtil * MmalUtil::m_instance = NULL;

// Add backwards compatibility for Raspbian Wheezy
#ifndef MMAL_COMPONENT_DEFAULT_CAMERA_INFO
#define MMAL_COMPONENT_DEFAULT_CAMERA_INFO "vc.camera_info"
#define MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN 16

typedef struct FREELSS_MMAL_PARAMETER_CAMERA_INFO_CAMERA_T
{
   uint32_t    port_id;
   uint32_t    max_width;
   uint32_t    max_height;
   MMAL_BOOL_T lens_present;
   char        camera_name[MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN];
} FREELSS_MMAL_PARAMETER_CAMERA_INFO_CAMERA_T;

typedef struct FREELSS_MMAL_PARAMETER_CAMERA_INFO_T
{
   MMAL_PARAMETER_HEADER_T             hdr;
   uint32_t                            num_cameras;
   uint32_t                            num_flashes;
   FREELSS_MMAL_PARAMETER_CAMERA_INFO_CAMERA_T cameras[MMAL_PARAMETER_CAMERA_INFO_MAX_CAMERAS];
   MMAL_PARAMETER_CAMERA_INFO_FLASH_T  flashes[MMAL_PARAMETER_CAMERA_INFO_MAX_FLASHES];
} FREELSS_MMAL_PARAMETER_CAMERA_INFO_T;

#else
#define FREELSS_MMAL_PARAMETER_CAMERA_INFO_CAMERA_T MMAL_PARAMETER_CAMERA_INFO_CAMERA_T
#define FREELSS_MMAL_PARAMETER_CAMERA_INFO_T MMAL_PARAMETER_CAMERA_INFO_T
#endif


const std::string MmalUtil::IMX219 = "IMX219";
const std::string MmalUtil::OV5647 = "OV5647";

static bool CompareWidth(const CameraResolution& a, const CameraResolution& b)
{
	return a.width < b.width;
}

MmalUtil * MmalUtil::get()
{
	if (MmalUtil::m_instance == NULL)
	{
		MmalUtil::m_instance = new MmalUtil();
	}

	return MmalUtil::m_instance;
}

void MmalUtil::release()
{
	delete MmalUtil::m_instance;
	MmalUtil::m_instance = NULL;
}

MmalUtil::MmalUtil()
{
	// Do nothing
}

std::string MmalUtil::getCameraName()
{
	MMAL_COMPONENT_T *camera_info;

	std::string name = "OV5647";

	// Try to get the camera name and maximum supported resolution
	if (MMAL_SUCCESS == mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA_INFO, &camera_info))
	{
		FREELSS_MMAL_PARAMETER_CAMERA_INFO_T param;
		memset(&param, 0, sizeof(param));

		param.hdr.id = MMAL_PARAMETER_CAMERA_INFO;
		param.hdr.size = sizeof(param) - 4;  // Deliberately undersize to check firmware veresion

		if (MMAL_SUCCESS != mmal_port_parameter_get(camera_info->control, &param.hdr))
		{
			 // Running on newer firmware
			 param.hdr.size = sizeof(param);
			 MMAL_STATUS_T status = mmal_port_parameter_get(camera_info->control, &param.hdr);
			 if (status == MMAL_SUCCESS && param.num_cameras > 0)
			 {
				 char camera_name[MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN] = {0};

				 // Copy to the FREELSS struct for backwards compability
				 FREELSS_MMAL_PARAMETER_CAMERA_INFO_CAMERA_T info;
				 memcpy(&info, &param.cameras[0], sizeof(info));
				 strncpy(camera_name,  info.camera_name, MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN);

				 camera_name[MMAL_PARAMETER_CAMERA_INFO_MAX_STR_LEN - 1] = 0;

				 if (TrimString(camera_name).size() > 0)
				 {
					 name = camera_name;
					 std::transform(name.begin(), name.end(), name.begin(), toupper);

					 ErrorLog << "Found Camera: " << name << Logger::ENDL;
				 }
			 }
			 else
			 {
				 ErrorLog << "Cannot read camera info, keeping the defaults for OV5647" << Logger::ENDL;
			 }
		}
		else
		{
		 // Older firmware
		 // Nothing to do here, keep the defaults for OV5647
		}

		mmal_component_destroy(camera_info);
	}

	return name;
}

std::vector<CameraResolution> MmalUtil::getSupportedResolutions(const std::string& cameraName)
{
	std::vector<CameraResolution> res;
	if (cameraName == OV5647 || cameraName.empty())
	{
		res.push_back(CreateResolution(2592, 1944, 15, CT_MMALSTILL, CM_STILL_5MP, "5 Megapixel"));
		res.push_back(CreateResolution(2592, 1944, 15, CT_MMALVIDEO, CM_VIDEO_5MP, "5 Megapixel"));
		res.push_back(CreateResolution(1600, 1200, 15, CT_MMALVIDEO, CM_VIDEO_HD, "1.9 Megapixel"));
		res.push_back(CreateResolution(1280, 960, 15, CT_MMALVIDEO, CM_VIDEO_1P2MP, "1.2 Megapixel"));
		res.push_back(CreateResolution(640, 480, 15, CT_MMALVIDEO, CM_VIDEO_VGA, "0.3 Megapixel"));
	}

	if (cameraName == IMX219 || cameraName.empty())
	{
		res.push_back(CreateResolution(3296, 2512, 15, CT_MMALSTILL, CM_STILL_8MP, "8 Megapixel"));
		res.push_back(CreateResolution(1280, 960, 15, CT_MMALSTILL, CM_STILL_HD, "1.2 Megapixel"));
		res.push_back(CreateResolution(640, 480, 15, CT_MMALSTILL, CM_STILL_VGA, "0.3 Megapixel"));
	}


	return res;
}

std::vector<CameraResolution> MmalUtil::getAllResolutions()
{
	return getSupportedResolutions("");
}

CameraResolution MmalUtil::getClosestResolution(const std::string& cameraName, CameraMode cameraMode)
{
	// Find the resolution for the camera mode
	std::vector<CameraResolution> all = getAllResolutions();
	CameraResolution resolution = all.front();

	for (size_t iAll = 0; iAll < all.size(); iAll++)
	{
		if (all[iAll].cameraMode == cameraMode)
		{
			resolution = all[iAll];
			break;
		}
	}

	std::vector<CameraResolution> resolutions = getSupportedResolutions(cameraName);

	// Sort by width
	std::sort(resolutions.begin(), resolutions.end(), CompareWidth);

	if (resolutions.empty())
	{
		throw Exception("No valid camera resolutions found");
	}

	CameraResolution out = resolutions.front();

	// First try to do an exact match
	bool found = false;
	for (size_t iRes = 0; iRes < resolutions.size(); iRes++)
	{
		if (resolutions[iRes].cameraMode == resolution.cameraMode)
		{
			out = resolution;
			found = true;
			break;
		}
	}

	// Find the resolution closest to the requested one
	if (!found)
	{
		for (size_t iRes = 0; iRes < resolutions.size(); iRes++)
		{
			CameraResolution res = resolutions[iRes];
			if (res.width > resolution.width)
			{
				break;
			}

			out = res;
		}
	}

	return out;
}

void MmalUtil::getSensorProperties(const std::string& cameraName, real& sensorWidth, real& sensorHeight, real& focalLength)
{
	if (cameraName == MmalUtil::IMX219)
	{
		sensorWidth = 3.674;
		sensorHeight = 2.760;
		focalLength = 3.04;
	}
	else if (cameraName == MmalUtil::OV5647)
	{
		sensorWidth = 3.629;
		sensorHeight = 2.722;
		focalLength = 3.6;
	}
	else
	{
		throw Exception("Unsupported camera: " + cameraName);
	}
}


} // ns freelss
