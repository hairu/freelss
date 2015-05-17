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

#ifndef BUILD_TESTS

#include "Main.h"
#include "MmalVideoCamera.h"
#include "Thread.h"

#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT   1
#define MMAL_CAMERA_CAPTURE_PORT 2
#define VIDEO_OUTPUT_BUFFERS_NUM 3

#define FULL_RES_VIDEO_FRAME_RATE_DEN 1

namespace freelss
{

struct Mmal_CallbackData
{
	Mmal_CallbackData() :
		acquire(false),
		buffer(NULL),
		bufferSize(0),
		bytesWritten(0),
		pool(NULL),
		outputQueue(NULL)
	{
		vcos_assert(vcos_semaphore_create(&complete_semaphore, "Scanner-Video-sem", 0) == VCOS_SUCCESS);
	}

	~Mmal_CallbackData()
	{
		vcos_semaphore_delete(&complete_semaphore);
	}

	bool acquire;
	byte * buffer;
	size_t bufferSize;
	size_t bytesWritten;
	CriticalSection cs;
	VCOS_SEMAPHORE_T complete_semaphore;
	MMAL_POOL_T * pool;
	MMAL_QUEUE_T * outputQueue;
};


static void EncoderBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   // We pass our file handle and other stuff in via the userdata field.

   Mmal_CallbackData *pData = (Mmal_CallbackData *)port->userdata;

   bool imageTaken = false;

   //std::cout << "EncoderBufferCallback" << std::endl;

   if (pData)
   {
      if (buffer->length)
      {
         // Copy the image data
         pData->cs.enter();

         if (pData->acquire)
         {
			 //std::cout << "EncoderBufferCallback - Locking buffer" << std::endl;
			 mmal_buffer_header_mem_lock(buffer);

			 if (pData->buffer != NULL  && pData->bytesWritten == 0)
			 {
				 int writeSize = buffer->length;

				 // Copy to our image buffer and move the pointer forward
				 if (writeSize > 0)
				 {
					 memcpy(pData->buffer, buffer->data, MIN(writeSize, pData->bufferSize));

					 pData->bytesWritten += writeSize;
					 imageTaken = true;

					 //std::cerr << "SRC Buffer Length: " << buffer->length << std::endl;
					 //std::cerr << "SRC Buffer Offset: " << buffer->offset << std::endl;
					 //std::cerr << "DST Buffer Size: " << pData->bufferSize << std::endl;
					 //std::cerr << "TOT Bytes Written: " << pData->bytesWritten << std::endl;
					 //std::cerr << "Write Size: " << writeSize << std::endl;
				 }
			 }

			 //std::cout << "EncoderBufferCallback - Unlocking buffer" << std::endl;
			 mmal_buffer_header_mem_unlock(buffer);

			 pData->acquire = false;
         }
         pData->cs.leave();
      }
   }
   else
   {
      std::cerr << "Received a encoder buffer callback with no state" << std::endl;
   }

   // Notify the user that the image is ready
   if (imageTaken)
   {
	   //std::cout << "EncoderBufferCallback - Posting semaphore" << std::endl << std::endl;
	   vcos_semaphore_post(&(pData->complete_semaphore));
   }

   // release buffer back to the pool
   mmal_buffer_header_release(buffer);

   // and send one back to the port (if still open)
   if (port->is_enabled && pData->pool && pData->pool->queue)
   {
	  //std::cout << "EncoderBufferCallback - Requeing buffer" << std::endl;
      MMAL_BUFFER_HEADER_T *new_buffer = NULL;

      if (pData->pool && pData->pool->queue)
      {
    	  new_buffer = mmal_queue_get(pData->pool->queue);
      }

      if (new_buffer)
      {
          mmal_port_send_buffer(port, new_buffer);
      }

      //std::cout << "EncoderBufferCallback - Buffer Returned" << std::endl;
   }
}

MmalVideoCamera::MmalVideoCamera(int imageWidth, int imageHeight, int frameRate) :
	m_imageWidth(imageWidth),
	m_imageHeight(imageHeight),
	m_frameRate(frameRate),
	m_imagePreviewWidth(VCOS_ALIGN_UP(imageWidth, 32)),
	m_imagePreviewHeight(VCOS_ALIGN_UP(imageHeight, 16)),
	m_callbackData(NULL),
	m_camera(NULL),
	m_pool(NULL),
	m_videoPort(NULL),
	m_stillPort(NULL),
	m_targetPort(NULL),
	m_targetPortEnabled(false)
{
	m_callbackData = new Mmal_CallbackData();

	createCameraComponent();
	std::cout << "Created camera" << std::endl;

	//std::cout << "Target Image Width: " << VCOS_ALIGN_UP(m_imageWidth, 32) << std::endl;
	//std::cout << "Target Image Height: " << VCOS_ALIGN_UP(m_imageHeight, 16) << std::endl;

	std::cout << "Target Image Width: " << m_imageWidth << std::endl;
	std::cout << "Target Image Height: " << m_imageHeight << std::endl;

	m_targetPort = m_videoPort;

	createBufferPool();

	double startTime = GetTimeInSeconds();
	do
	{
		sendBuffers();
		Thread::usleep(10000);
	} while ((GetTimeInSeconds() - startTime) < 1);

	std::cout << "Initialized camera" << std::endl;
}

MmalVideoCamera::~MmalVideoCamera()
{
	// Disable the process
	if (m_targetPort != NULL && m_targetPortEnabled)
	{
		if (mmal_port_disable(m_targetPort) != MMAL_SUCCESS)
		{
			std::cerr << "Error disabling the encoder port" << std::endl;
		}
	}

	if (m_camera != NULL)
	{
		mmal_component_disable(m_camera);
	}

	if (m_pool != NULL)
	{
		mmal_port_pool_destroy(m_targetPort, m_pool);
	}

	if (m_camera != NULL)
	{
		mmal_component_destroy(m_camera);
	}

	delete m_callbackData;
}


void MmalVideoCamera::createBufferPool()
{
	if (m_targetPort == NULL)
	{
		throw Exception("Target port is not set.");
	}

	// Create pool of buffer headers for the output port to consume
	m_targetPort->buffer_num = 3;
	m_targetPort->buffer_size = m_targetPort->buffer_size_recommended;
	m_pool = mmal_port_pool_create(m_targetPort, m_targetPort->buffer_num, m_targetPort->buffer_size);
	if (m_pool == NULL)
	{
		throw Exception("Failed to create buffer header pool for encoder output port");
	}

	// Set the userdata
	m_targetPort->userdata = (struct MMAL_PORT_USERDATA_T *) m_callbackData;

	// Enable the encoder output port and tell it its callback function
	if (mmal_port_enable(m_targetPort, EncoderBufferCallback) != MMAL_SUCCESS)
	{
		throw Exception("Error enabling the encoder buffer callback");
	}
	else
	{
		m_targetPortEnabled = true;
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
		//cameraConfig.max_preview_video_w = VCOS_ALIGN_UP(m_imagePreviewWidth, 32);
		//cameraConfig.max_preview_video_h = VCOS_ALIGN_UP(m_imagePreviewHeight, 16);
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
		format->encoding = MMAL_ENCODING_BGR24;
		format->encoding_variant = MMAL_ENCODING_BGR24;

		format->es->video.width = VCOS_ALIGN_UP(m_imageWidth, 32);
		format->es->video.height = VCOS_ALIGN_UP(m_imageHeight, 16);
		//format->es->video.width = m_imageWidth;
		//format->es->video.height = m_imageHeight;
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

		std::cout << "Camera Enabled..." << std::endl;
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
	//m_previewPort = preview_port;
	m_videoPort = video_port;
	m_stillPort = still_port;
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
			std::cerr << "Unable to get buffer " << q << " from the pool" << std::endl;
		}

		if (mmal_port_send_buffer(m_videoPort, new_buffer)!= MMAL_SUCCESS)
		{
			std::cerr << "Unable to send a buffer " << q << " to encoder output port " << std::endl;
		}
	}
}

void MmalVideoCamera::acquireImage(Image * image)
{
	// Enable the encoder output port
	m_targetPort->userdata = (struct MMAL_PORT_USERDATA_T *) m_callbackData;

	m_callbackData->cs.enter();
	m_callbackData->buffer = image->getPixels();
	m_callbackData->pool = m_pool;
	m_callbackData->bufferSize = image->getPixelBufferSize();
	m_callbackData->bytesWritten = 0;
	m_callbackData->acquire = true;
	m_callbackData->cs.leave();

	sendBuffers();

	bool waiting = true;
	while (waiting)
	{
		m_callbackData->cs.enter();
		if (!m_callbackData->acquire)
		{
			waiting = false;
		}
		else
		{
			//Thread::usleep(10000);
			Thread::usleep(10000);
		}
		m_callbackData->cs.leave();
	}
}

bool MmalVideoCamera::acquireJpeg(byte* buffer, unsigned * size)
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

real MmalVideoCamera::getSensorWidth() const
{
	return 3.629;
}

real MmalVideoCamera::getSensorHeight() const
{
	return 2.722;
}

real MmalVideoCamera::getFocalLength() const
{
	return 3.6;
}
} // end ns scanner

#endif
