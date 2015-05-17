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
#include "MmalStillCamera.h"


#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1

#define MMAL_CAMERA_CAPTURE_PORT 2
#define VIDEO_OUTPUT_BUFFERS_NUM 3

#define STILLS_FRAME_RATE_NUM 0
#define STILLS_FRAME_RATE_DEN 1

#define FULL_RES_PREVIEW_FRAME_RATE_NUM 0
#define FULL_RES_PREVIEW_FRAME_RATE_DEN 1

#define FULL_FOV_PREVIEW_4x3_X 1296
#define FULL_FOV_PREVIEW_4x3_Y 972



namespace freelss
{

struct Mmal_CallbackData
{
	Mmal_CallbackData()
	{
		vcos_assert(vcos_semaphore_create(&complete_semaphore, "Scanner-Still-sem", 0) == VCOS_SUCCESS);
	}

	~Mmal_CallbackData()
	{
		vcos_semaphore_delete(&complete_semaphore);
	}

	byte * buffer;
	size_t bufferSize;
	size_t bytesWritten;
	VCOS_SEMAPHORE_T complete_semaphore;
	MMAL_POOL_T * pool;
};

static MMAL_STATUS_T connect_ports(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection)
{
   MMAL_STATUS_T status;

   status =  mmal_connection_create(connection, output_port, input_port, MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);

   if (status == MMAL_SUCCESS)
   {
      status =  mmal_connection_enable(*connection);
      if (status != MMAL_SUCCESS)
      {
    	  mmal_connection_destroy(*connection);
      }
   }

   return status;
}

static void CameraControlCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	if (buffer->cmd != MMAL_EVENT_PARAMETER_CHANGED)
	{
		std::cerr << "Received unexpected camera control callback event" << std::endl;
	}

	mmal_buffer_header_release(buffer);
}


static void EncoderBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   int complete = 0;

   // We pass our file handle and other stuff in via the userdata field.

   Mmal_CallbackData *pData = (Mmal_CallbackData *)port->userdata;

   if (pData)
   {

      if (buffer->length)
      {
         mmal_buffer_header_mem_lock(buffer);

         int writeSize = buffer->length;
         if (pData->bytesWritten + writeSize > pData->bufferSize)
         {
        	 writeSize = pData->bufferSize - pData->bytesWritten;
         }

         // Copy to our image buffer and move the pointer forward
         if (writeSize > 0)
         {
        	 memcpy(pData->buffer, buffer->data, writeSize);
        	 pData->buffer += writeSize;
         }

         mmal_buffer_header_mem_unlock(buffer);
      }

      // Now flag if we have completed
      if (buffer->flags & (MMAL_BUFFER_HEADER_FLAG_FRAME_END | MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED))
      {
         complete = 1;
      }
   }
   else
   {
      std::cerr << "Received a encoder buffer callback with no state" << std::endl;
   }

   // release buffer back to the pool
   mmal_buffer_header_release(buffer);

   // and send one back to the port (if still open)
   if (port->is_enabled)
   {
      MMAL_STATUS_T status = MMAL_SUCCESS;
      MMAL_BUFFER_HEADER_T *new_buffer;

      new_buffer = mmal_queue_get(pData->pool->queue);

      if (new_buffer)
      {
         status = mmal_port_send_buffer(port, new_buffer);
      }

      if (!new_buffer || status != MMAL_SUCCESS)
      {
         std::cerr << "Unable to return a buffer to the encoder port" << std::endl;
      }
   }

   if (complete)
   {
      vcos_semaphore_post(&(pData->complete_semaphore));
   }
}

MmalStillCamera::MmalStillCamera(int imageWidth, int imageHeight) :
	m_imageWidth(imageWidth),
	m_imageHeight(imageHeight),
	m_callbackData(NULL),
	m_cs(),
	m_camera(NULL),
	m_preview(NULL),
	m_encoder(NULL),
	m_pool(NULL),
	m_previewPort(NULL),
	m_videoPort(NULL),
	m_stillPort(NULL),
	m_targetPort(NULL)
{
	m_callbackData = new Mmal_CallbackData();

	createCameraComponent();
	std::cout << "Created camera" << std::endl;

	createPreview();
	std::cout << "Created preview" << std::endl;

	std::cout << "Target Image Width: " << VCOS_ALIGN_UP(m_imageWidth, 32) << std::endl;
	std::cout << "Target Image Height: " << VCOS_ALIGN_UP(m_imageHeight, 16) << std::endl;

	MMAL_STATUS_T status;

	m_targetPort = m_stillPort;

	// Create pool of buffer headers for the output port to consume
	m_pool = mmal_port_pool_create(m_targetPort, m_targetPort->buffer_num, m_targetPort->buffer_size);
	if (m_pool == NULL)
	{
		throw Exception("Failed to create buffer header pool for encoder output port");
	}

	MMAL_CONNECTION_T *preview_connection = NULL;
	status = connect_ports(m_previewPort, m_preview->input[0], &preview_connection);
	if (status != MMAL_SUCCESS)
	{
		throw Exception("Error connecting preview port");
	}

	//MMAL_CONNECTION_T *encoder_connection = NULL;
	//status = connect_ports(m_stillPort, m_encoder->input[0], &encoder_connection);
	//if (status != MMAL_SUCCESS)
	//{
	//	throw Exception("Failed to connect camera still port to encoder input");
	//}

	std::cout << "Initialized camera" << std::endl;
}

MmalStillCamera::~MmalStillCamera()
{
	if (m_camera != NULL)
	{
		mmal_component_destroy(m_camera);
	}

	if (m_preview != NULL)
	{
		mmal_component_destroy(m_preview);
	}

	if (m_encoder != NULL)
	{
		mmal_component_destroy(m_encoder);
	}

	if (m_pool != NULL)
	{
		mmal_port_pool_destroy(m_targetPort, m_pool);
	}

	delete m_callbackData;
}

void MmalStillCamera::createPreview()
{
	MMAL_COMPONENT_T * preview = NULL;
	MMAL_STATUS_T status;

	try
	{
		status = mmal_component_create("vc.null_sink", &preview);
		//status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_RENDERER, &preview);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Error creating preview sink");
		}

		status = mmal_component_enable(preview);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Error enabling preview");
		}
	}
	catch (...)
	{
		if (preview != NULL)
		{
			mmal_component_destroy(preview);
		}

		throw;
	}

	m_preview = preview;
}

void MmalStillCamera::createCameraComponent()
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

		// Enable the camera, and tell it its control callback function
		status = mmal_port_enable(camera->control, CameraControlCallback);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Unable to enable control port");
		}

		//  Populate the camera configuration struct
		MMAL_PARAMETER_CAMERA_CONFIG_T cameraConfig;
		cameraConfig.hdr.id = MMAL_PARAMETER_CAMERA_CONFIG;
		cameraConfig.hdr.size = sizeof(cameraConfig);
		cameraConfig.max_stills_w = m_imageWidth;
		cameraConfig.max_stills_h = m_imageHeight;
		cameraConfig.stills_yuv422 = 0;
		cameraConfig.one_shot_stills = 1;
		cameraConfig.max_preview_video_w = VCOS_ALIGN_UP(FULL_FOV_PREVIEW_4x3_X, 32);
		cameraConfig.max_preview_video_h = VCOS_ALIGN_UP(FULL_FOV_PREVIEW_4x3_Y, 16);
		cameraConfig.num_preview_video_frames = 3;
		cameraConfig.stills_capture_circular_buffer_height = 0;
		cameraConfig.fast_preview_resume = 0;
		cameraConfig.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC;


		if (mmal_port_parameter_set(camera->control, &cameraConfig.hdr) != MMAL_SUCCESS)
		{
			throw Exception("Error calling mmal_port_parameter_set()");
		}

		format = preview_port->format;
		format->encoding = MMAL_ENCODING_OPAQUE;
		format->encoding_variant = MMAL_ENCODING_I420;

		format->es->video.width = VCOS_ALIGN_UP(FULL_FOV_PREVIEW_4x3_X, 32);
		format->es->video.height = VCOS_ALIGN_UP(FULL_FOV_PREVIEW_4x3_Y, 16);
		format->es->video.crop.x = 0;
		format->es->video.crop.y = 0;
		format->es->video.crop.width = FULL_FOV_PREVIEW_4x3_X;
		format->es->video.crop.height = FULL_FOV_PREVIEW_4x3_Y;
		format->es->video.frame_rate.num = FULL_RES_PREVIEW_FRAME_RATE_NUM;
		format->es->video.frame_rate.den = FULL_RES_PREVIEW_FRAME_RATE_DEN;

		status = mmal_port_format_commit(preview_port);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Failure to setup camera preview port");
		}

		// We must also setup the video port even though this is an image acquisition
		mmal_format_full_copy(video_port->format, format);
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

		// Use the video format for images as well
		format = still_port->format;

		// Set our stills format on the stills (for encoder) port
		format->encoding = MMAL_ENCODING_BGR24;
		format->encoding_variant = MMAL_ENCODING_BGR24;

		format->es->video.width = VCOS_ALIGN_UP(m_imageWidth, 32);
		format->es->video.height = VCOS_ALIGN_UP(m_imageHeight, 16);
		format->es->video.crop.x = 0;
		format->es->video.crop.y = 0;
		format->es->video.crop.width = m_imageWidth;
		format->es->video.crop.height = m_imageHeight;
		format->es->video.frame_rate.num = STILLS_FRAME_RATE_NUM;
		format->es->video.frame_rate.den = STILLS_FRAME_RATE_DEN;

		status = mmal_port_format_commit(still_port);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Failure to setup camera image port");
		}

		// Verify number of buffers
		if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
		{
			still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;
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
	m_previewPort = preview_port;
	m_videoPort = video_port;
	m_stillPort = still_port;
}

void MmalStillCamera::acquireImage(Image * image)
{

	// Enable the encoder output port
	m_targetPort->userdata = (struct MMAL_PORT_USERDATA_T *) m_callbackData;
	m_callbackData->buffer = image->getPixels();
	m_callbackData->pool = m_pool;
	m_callbackData->bufferSize = image->getPixelBufferSize();
	m_callbackData->bytesWritten = 0;

	// Enable the encoder output port and tell it its callback function
	if (mmal_port_enable(m_targetPort, EncoderBufferCallback) != MMAL_SUCCESS)
	{
		std::cerr << "Error enabling the encoder buffer callback" << std::endl;
	}

	// Send all the buffers to the encoder output port
	int num = mmal_queue_length(m_pool->queue);

	for (int q = 0; q < num; q++)
	{
		MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(m_pool->queue);

		if (buffer == NULL)
		{
			std::cerr << "Unable to get buffer " << q << " from the pool" << std::endl;
		}

		if (mmal_port_send_buffer(m_targetPort, buffer)!= MMAL_SUCCESS)
		{
			std::cerr << "Unable to send a buffer " << q << " to encoder output port " << std::endl;
		}
	}

	// Start the capture
	if (mmal_port_parameter_set_boolean(m_stillPort, MMAL_PARAMETER_CAPTURE, 1) == MMAL_SUCCESS)
	{
		// Wait for the capture to end
		vcos_semaphore_wait(&m_callbackData->complete_semaphore);
	}
	else
	{
		std::cerr << "Error starting the camera capture" << std::endl;
	}

	// Disable the encoder
	if (mmal_port_disable(m_targetPort) != MMAL_SUCCESS)
	{
		std::cerr << "Error disabling the encoder port" << std::endl;
	}
}

bool MmalStillCamera::acquireJpeg(byte* buffer, unsigned * size)
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

int MmalStillCamera::getImageHeight() const
{
	return m_imageHeight;
}

int MmalStillCamera::getImageWidth() const
{
	return m_imageWidth;
}

int MmalStillCamera::getImageComponents() const
{
	return 3;
}

real MmalStillCamera::getSensorWidth() const
{
	return 3.629;
}

real MmalStillCamera::getSensorHeight() const
{
	return 2.722;
}

real MmalStillCamera::getFocalLength() const
{
	return 3.6;
}

} // end ns scanner
