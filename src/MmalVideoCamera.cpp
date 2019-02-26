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
#include "MmalVideoCamera.h"
#include "Thread.h"
#include "MmalImageStore.h"
#include "Logger.h"
#include "MmalUtil.h"

#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT   1
#define MMAL_CAMERA_CAPTURE_PORT 2
#define MMAL_VIDEO_SENSOR_MODE   2    // no-binning mode: https://www.raspberrypi.org/documentation/raspbian/applications/camera.md
#define VIDEO_OUTPUT_BUFFERS_NUM 3

#define FULL_RES_VIDEO_FRAME_RATE_DEN 1

#define NUM_IMAGE_BUFFERS 3 // The number of images available for acquisition before calling release

#define MMAL_CHECK(cmd) {\
	int status = cmd;\
	if (status != MMAL_SUCCESS)\
	{\
		std::stringstream sstr;\
		sstr << "Error calling " << #cmd << ", error=" << status;\
		throw Exception(sstr.str());\
	}\
}

namespace freelss
{

struct MmalVideoCallbackData
{
	MmalVideoCallbackData(unsigned width, unsigned height, unsigned numComponents) :
		acquire(false),
		pool(NULL),
		camera(NULL),
		outputQueue(NULL),
		mmalBuffer(NULL),
		image(NULL),
		imageStore(NUM_IMAGE_BUFFERS, width, height, numComponents)
	{
		vcos_assert(vcos_semaphore_create(&complete_semaphore, "Scanner-Video-sem", 0) == VCOS_SUCCESS);
	}

	~MmalVideoCallbackData()
	{
		vcos_semaphore_delete(&complete_semaphore);
	}

	VCOS_SEMAPHORE_T complete_semaphore;
	bool acquire;
	CriticalSection cs;
	MMAL_POOL_T * pool;
	MMAL_COMPONENT_T * camera;
	MMAL_QUEUE_T * outputQueue;
	MMAL_BUFFER_HEADER_T * mmalBuffer;
	Image * image;
	MmalImageStore imageStore;
};

static void EncoderBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   //InfoLog << "EncoderBufferCallback: " << pthread_self() << Logger::ENDL;

   MmalVideoCallbackData *pData = (MmalVideoCallbackData *)port->userdata;

   bool mappedBuffer = false;

   if (pData)
   {
	   if (buffer->length)
	   {
         // Copy the image data
         pData->cs.enter("EncoderBufferCallback");
         try
         {
			 if (pData->acquire)
			 {
				 MmalImageStoreItem * item = pData->imageStore.reserve(buffer);
				 if (item == NULL)
				 {
					 throw Exception("No images available in the MmalImageStore");
				 }

				 pData->image = &item->image;

				 pData->acquire = false;
				 mappedBuffer = true;
			 }
         }
         catch (Exception& e)
         {
        	 ErrorLog << "!! Exception thrown in EncoderBufferCallback: " << e << Logger::ENDL;
         }
         catch (...)
         {
        	 ErrorLog << "!! Exception thrown in EncoderBufferCallback" << Logger::ENDL;
         }
         pData->cs.leave();
	   }
   }
   else
   {
	   ErrorLog << "!! Received a encoder buffer callback with no state" << Logger::ENDL;
   }

   // release buffer back to the pool
   if (mappedBuffer)
   {
	   vcos_semaphore_post(&(pData->complete_semaphore));
   }
   else
   {
	   mmal_buffer_header_release(buffer);
   }

   // and send one back to the port (if still open)
   if (port->is_enabled)
   {
      MMAL_BUFFER_HEADER_T *new_buffer = mmal_queue_get(pData->pool->queue);

      if (new_buffer)
      {
          mmal_port_send_buffer(port, new_buffer);
      }
   }
}

MmalVideoCamera::MmalVideoCamera() :
	m_imageWidth(-1),
	m_imageHeight(-1),
	m_frameRate(-1),
	m_imagePreviewWidth(-1),
	m_imagePreviewHeight(-1),
	m_callbackData(NULL),
	m_cs(),
	m_camera(NULL),
	m_pool(NULL),
	m_videoPort(NULL),
	m_stillPort(NULL),
	m_previewPort(NULL),
	m_videoPortEnabled(false),
	m_flipRedBlue(false)
{
	m_name = MmalUtil::get()->getCameraName();
	m_supportedResolutions = MmalUtil::get()->getSupportedResolutions(m_name);
}

MmalVideoCamera::~MmalVideoCamera()
{
	// Disable the video port
	if (m_videoPort != NULL && m_videoPort->is_enabled)
	{
		if (mmal_port_disable(m_videoPort) != MMAL_SUCCESS)
		{
			ErrorLog << "Error disabling the video port" << Logger::ENDL;
		}
		else
		{
			InfoLog << "Disabled the video port" << Logger::ENDL;
		}
	}

	if (m_camera != NULL)
	{
		mmal_component_disable(m_camera);
	}

	if (m_pool != NULL)
	{
		mmal_port_pool_destroy(m_videoPort, m_pool);
	}

	if (m_camera != NULL)
	{
		mmal_component_destroy(m_camera);
	}

	delete m_callbackData;
	m_callbackData = NULL;
}

void MmalVideoCamera::initialize(CameraMode cameraMode)
{
	m_resolution = MmalUtil::get()->getClosestResolution(m_name, cameraMode);

	if (m_imageWidth != -1)
	{
		throw Exception("Camera is already initialized");
	}

	m_imageWidth = m_resolution.width;
	m_imageHeight = m_resolution.height;
	m_frameRate = m_resolution.frameRate;
	m_imagePreviewWidth = VCOS_ALIGN_UP(m_resolution.width, 32);
	m_imagePreviewHeight = VCOS_ALIGN_UP(m_resolution.height, 16);

	// Handle the sensor properties
	real sensorWidth, sensorHeight, focalLength;
	MmalUtil::get()->getSensorProperties(m_name, sensorWidth, sensorHeight, focalLength);
	setSensorProperties(sensorWidth, sensorHeight, focalLength);

	InfoLog << "Creating callback data..." << Logger::ENDL;

	m_callbackData = new MmalVideoCallbackData(m_imageWidth, m_imageHeight, 3);

	InfoLog << "Creating camera..." << Logger::ENDL;

	createCameraComponent();

	InfoLog << "Created camera" << Logger::ENDL;

	InfoLog << "Target Image Width: " << m_imageWidth << Logger::ENDL;
	InfoLog << "Target Image Height: " << m_imageHeight << Logger::ENDL;

	createBufferPool();

	InfoLog << "Initialized camera" << Logger::ENDL;
}

void MmalVideoCamera::setShutterSpeed(unsigned shutterSpeedUs)
{
	if (m_camera != NULL)
	{
		mmal_port_parameter_set_uint32(m_camera->control, MMAL_PARAMETER_SHUTTER_SPEED, shutterSpeedUs);
	}
}

void MmalVideoCamera::setFlipRedBlue(bool flip)
{
	m_flipRedBlue = flip;
}

void MmalVideoCamera::createBufferPool()
{
	if (m_videoPort == NULL)
	{
		throw Exception("Target port is not set.");
	}

	// Create pool of buffer headers for the output port to consume
	m_videoPort->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;
	m_videoPort->buffer_size = m_videoPort->buffer_size_recommended;
	m_pool = mmal_port_pool_create(m_videoPort, MAX(m_videoPort->buffer_num, NUM_IMAGE_BUFFERS), m_videoPort->buffer_size);

	if (m_pool == NULL)
	{
		throw Exception("Failed to create buffer header pool for encoder output port");
	}

	// Set the userdata
	m_callbackData->pool = m_pool;
	m_callbackData->acquire = false;
	m_videoPort->userdata = (struct MMAL_PORT_USERDATA_T *) m_callbackData;

	// Enable the encoder output port and tell it its callback function
	if (mmal_port_enable(m_videoPort, EncoderBufferCallback) != MMAL_SUCCESS)
	{
		throw Exception("Error enabling the encoder buffer callback");
	}
	else
	{
		m_videoPortEnabled = true;
	}

	// Send all the buffers to the encoder output port
	sendBuffers();

	// Start the capture
	if (mmal_port_parameter_set_boolean(m_videoPort, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
	{
		throw Exception("Error starting the camera capture");
	}
}

void MmalVideoCamera::createCameraComponent()
{
	MMAL_COMPONENT_T *camera = 0;
	MMAL_ES_FORMAT_T *format;
	MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;
	MMAL_STATUS_T status;

	try
	{
		status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Failed to create camera component");
		}

		if (camera->output_num == 0)
		{
			status = MMAL_ENOSYS;
			throw Exception("Camera doesn't have output ports");
		}

		MMAL_CHECK(mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG, MMAL_VIDEO_SENSOR_MODE));

		preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
		video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
		still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

		//  Populate the camera configuration struct
		MMAL_PARAMETER_CAMERA_CONFIG_T cameraConfig;
		cameraConfig.hdr.id = MMAL_PARAMETER_CAMERA_CONFIG;
		cameraConfig.hdr.size = sizeof(cameraConfig);
		cameraConfig.max_stills_w = m_imageWidth;
		cameraConfig.max_stills_h = m_imageHeight;
		cameraConfig.stills_yuv422 = 0;
		cameraConfig.one_shot_stills = 0;
		cameraConfig.max_preview_video_w = m_imagePreviewWidth;
		cameraConfig.max_preview_video_h = m_imagePreviewHeight;
		cameraConfig.num_preview_video_frames = 3;
		cameraConfig.stills_capture_circular_buffer_height = 0;
		cameraConfig.fast_preview_resume = 0;
		cameraConfig.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC;

		if (mmal_port_parameter_set(camera->control, &cameraConfig.hdr) != MMAL_SUCCESS)
		{
			throw Exception("Error calling mmal_port_parameter_set()");
		}

		//
		// Setup the video port
		//
		format = video_port->format;
		format->encoding = m_flipRedBlue ? MMAL_ENCODING_BGR24 : MMAL_ENCODING_RGB24;
		format->encoding_variant = m_flipRedBlue ? MMAL_ENCODING_BGR24 : MMAL_ENCODING_RGB24;

		format->es->video.width = VCOS_ALIGN_UP(m_imageWidth, 32);
		format->es->video.height = VCOS_ALIGN_UP(m_imageHeight, 16);
		format->es->video.crop.x = 0;
		format->es->video.crop.y = 0;
		format->es->video.crop.width = m_imageWidth;
		format->es->video.crop.height = m_imageHeight;
		format->es->video.frame_rate.num = m_frameRate;
		format->es->video.frame_rate.den = FULL_RES_VIDEO_FRAME_RATE_DEN;

		status = mmal_port_format_commit(video_port);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Failure to setup camera video port");
		}

		// Make sure we have enough video buffers
		if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
		{
			video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;
		}

		// Flip the image
		MMAL_PARAMETER_MIRROR_T mirror = {{MMAL_PARAMETER_MIRROR, sizeof(MMAL_PARAMETER_MIRROR_T)}, MMAL_PARAM_MIRROR_NONE};
		mirror.value = MMAL_PARAM_MIRROR_BOTH;
		mmal_port_parameter_set(preview_port, &mirror.hdr);
		mmal_port_parameter_set(video_port, &mirror.hdr);
		mmal_port_parameter_set(still_port, &mirror.hdr);

		// Enable component
		status = mmal_component_enable(camera);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("camera component couldn't be enabled");
		}

		//int iso = 100;
		//MMAL_CHECK(mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_ISO, iso));

		// MMAL_PARAM_EXPOSUREMODE_OFF
		//MMAL_PARAMETER_EXPOSUREMODE_T exp_mode = {{MMAL_PARAMETER_EXPOSURE_MODE, sizeof(exp_mode)}, MMAL_PARAM_EXPOSUREMODE_AUTO};
		//MMAL_CHECK(mmal_port_parameter_set(camera->control, &exp_mode.hdr));

		/*
		// AWB MODE
		{
			InfoLog << "Disabling auto white balancing" << Logger::ENDL;
			MMAL_PARAMETER_AWBMODE_T param = {{MMAL_PARAMETER_AWB_MODE,sizeof(param)}, MMAL_PARAM_AWBMODE_OFF};
			mmal_port_parameter_set(camera->control, &param.hdr);
		}

		// AWB GAIN
		{
			real redGain = 0.2;
			real blueGain = 1.0;

			InfoLog << "Setting color gain, r=" << redGain << ", b=" << blueGain << Logger::ENDL;
			MMAL_PARAMETER_AWB_GAINS_T param = {{MMAL_PARAMETER_CUSTOM_AWB_GAINS,sizeof(param)}, {0,0}, {0,0}};
			param.r_gain.num = (unsigned int)(redGain * 65536);
			param.b_gain.num = (unsigned int)(blueGain * 65536);
			param.r_gain.den = param.b_gain.den = 65536;
			mmal_port_parameter_set(camera->control, &param.hdr);
		}
		*/

		InfoLog << "Camera Enabled..." << Logger::ENDL;
	}
	catch (...)
	{
		if (camera != NULL)
		{
			mmal_component_destroy(camera);
		}

		throw;
	}

	m_camera = camera;
	m_videoPort = video_port;
	m_stillPort = still_port;
	m_previewPort = preview_port;
}

void MmalVideoCamera::sendBuffers()
{
	// Send all the buffers to the encoder output port
	int num = mmal_queue_length(m_pool->queue);

	for (int q = 0; q < num; q++)
	{
		MMAL_BUFFER_HEADER_T *new_buffer = mmal_queue_get(m_pool->queue);

		if (new_buffer == NULL)
		{
			ErrorLog << "Unable to get buffer " << q << " from the pool" << Logger::ENDL;
		}

		if (mmal_port_send_buffer(m_videoPort, new_buffer)!= MMAL_SUCCESS)
		{
			ErrorLog << "Unable to send buffer " << (q + 1) << " to encoder output port " << Logger::ENDL;
		}
	}
}

Image * MmalVideoCamera::acquireImage()
{
	// Wait for the camera to be ready
	handleAcquisitionDelay();

	if (m_callbackData == NULL)
	{
		throw Exception("NULL callback data");
	}

	// Enable the encoder output port
	m_videoPort->userdata = (struct MMAL_PORT_USERDATA_T *) m_callbackData;

	m_callbackData->cs.enter();
	m_callbackData->pool = m_pool;
	m_callbackData->acquire = true;
	m_callbackData->camera = m_camera;
	m_callbackData->image = NULL;
	m_callbackData->cs.leave();

	// Wait for the capture to end
	vcos_semaphore_wait(&m_callbackData->complete_semaphore);

	// Grab the image
	Image * image = NULL;
	m_callbackData->cs.enter();
	image = m_callbackData->image;
	m_callbackData->cs.leave();

	return image;
}

void MmalVideoCamera::releaseImage(Image * image)
{
	if (image != NULL)
	{
		m_callbackData->imageStore.release(image);
	}
}

int MmalVideoCamera::getImageHeight() const
{
	return m_imageHeight;
}

int MmalVideoCamera::getImageWidth() const
{
	return m_imageWidth;
}

int MmalVideoCamera::getImageComponents() const
{
	return 3;
}

} // end ns scanner
