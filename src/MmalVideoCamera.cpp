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

#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT   1
#define MMAL_CAMERA_CAPTURE_PORT 2
#define VIDEO_OUTPUT_BUFFERS_NUM 3

#define FULL_RES_VIDEO_FRAME_RATE_NUM 30
#define FULL_RES_VIDEO_FRAME_RATE_DEN 1

#define FULL_FOV_PREVIEW_4x3_X 1296
#define FULL_FOV_PREVIEW_4x3_Y 972

#define MMAL_SPLITTER_VIDEO_PORT 0
#define MMAL_SPLITTER_CAPTURE_PORT 1

//#define CAMERA_IMAGE_WIDTH 2592
//#define CAMERA_IMAGE_HEIGHT 1944

#define CAMERA_IMAGE_WIDTH 1280
#define CAMERA_IMAGE_HEIGHT 960

namespace freelss
{

struct Mmal_CallbackData
{
	Mmal_CallbackData()
	{
		vcos_assert(vcos_semaphore_create(&complete_semaphore, "Scanner-Video-sem", 0) == VCOS_SUCCESS);
	}

	~Mmal_CallbackData()
	{
		vcos_semaphore_delete(&complete_semaphore);
	}

	byte * buffer;
	size_t bufferSize;
	size_t bytesWritten;
	CriticalSection cs;
	VCOS_SEMAPHORE_T complete_semaphore;
	MMAL_POOL_T * pool;
	MMAL_QUEUE_T * outputQueue;
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

#if 1
static void EncoderBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	std::cout << "In EncoderBufferCallback" << std::endl;

	Mmal_CallbackData *pData = (Mmal_CallbackData *)port->userdata;

	//to handle the user not reading frames, remove and return any pre-existing ones
	if(mmal_queue_length(pData->outputQueue) >= 2)
	{
		if(MMAL_BUFFER_HEADER_T* existing_buffer = mmal_queue_get(pData->outputQueue))
		{
			mmal_buffer_header_release(existing_buffer);
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
					std::cerr << "Unable to return a buffer to the video port" << std::endl;
				}
			}
		}
	}

		//add the buffer to the output queue
		mmal_queue_put(pData->outputQueue, buffer);
}
#else
static void EncoderBufferCallback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
   int complete = 0;

   // We pass our file handle and other stuff in via the userdata field.

   Mmal_CallbackData *pData = (Mmal_CallbackData *)port->userdata;

   bool imageTaken = false;

   std::cout << "EncoderBufferCallback" << std::endl;

   if (pData)
   {
      int bytes_written = buffer->length;

      if (buffer->length)
      {
         // Copy the image data
         pData->cs.enter();
         std::cout << "EncoderBufferCallback - Locking buffer" << std::endl;
         mmal_buffer_header_mem_lock(buffer);

         if (pData->buffer != NULL)
         {
			 int writeSize = buffer->length;
			 if (pData->bytesWritten + writeSize > pData->bufferSize)
			 {
				 writeSize = pData->bufferSize - pData->bytesWritten;
			 }

			 // Copy to our image buffer and move the pointer forward
			 if (writeSize > 0)
			 {
				 // memcpy(pData->buffer, buffer->data, writeSize);

				 // The data is RGBA so we must copy just the RGB bytes
				 int step = 1;
				 byte * dst = pData->buffer;
				 for (int i = 0; i < buffer->length; i++)
				 {
					 * dst = buffer->data[i];
					 dst++;

					 if (step == 3)
					 {
						 i++;
						 step = 1;
					 }
				 }
			 }

			 // Unset our buffer now that we have copied to it
			 pData->buffer = NULL;
			 imageTaken = true;
         }

         std::cout << "EncoderBufferCallback - Unlocking buffer" << std::endl;
         mmal_buffer_header_mem_unlock(buffer);
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
	   std::cout << "EncoderBufferCallback - Posting semaphore" << std::endl;
	   vcos_semaphore_post(&(pData->complete_semaphore));
   }

   // release buffer back to the pool
   mmal_buffer_header_release(buffer);

   // and send one back to the port (if still open)
   if (port->is_enabled)
   {
	  std::cout << "EncoderBufferCallback - Requeing buffer" << std::endl;
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
}
#endif

MmalVideoCamera::MmalVideoCamera() :
	m_callbackData(NULL),
	m_camera(NULL),
	m_preview(NULL),
	m_resizer(NULL),
	m_splitter(NULL),
	m_pool(NULL),
	m_previewPort(NULL),
	m_videoPort(NULL),
	m_stillPort(NULL),
	m_resizerPort(NULL),
	m_targetPort(NULL),
	m_splitterPort(NULL),
	m_outputQueue(NULL),
	m_targetPortEnabled(false)
{
	m_callbackData = new Mmal_CallbackData();

	createCameraComponent();
	std::cout << "Created camera" << std::endl;

	createPreview();
	std::cout << "Created preview" << std::endl;

	createSplitter();
	std::cout << "Created splitter" << std::endl;

	createResizer();
	std::cout << "Created resizer" << std::endl;

	std::cout << "Target Image Width: " << VCOS_ALIGN_UP(CAMERA_IMAGE_WIDTH, 32) << std::endl;
	std::cout << "Target Image Height: " << VCOS_ALIGN_UP(CAMERA_IMAGE_HEIGHT, 16) << std::endl;


	MMAL_STATUS_T status;

	// We will read from the resizer
	m_targetPort = m_resizerPort;

	createBufferPool();
	std::cout << "Created buffer pool" << std::endl;

	// Hook-up the preview port because the camera requires it
	MMAL_CONNECTION_T *preview_connection = NULL;
	status = connect_ports(m_previewPort, m_preview->input[0], &preview_connection);
	if (status != MMAL_SUCCESS)
	{
		throw Exception("Error connecting preview port");
	}

	// Connect the video to the splitter
	MMAL_CONNECTION_T *splitter_connection = NULL;
	status = connect_ports(m_videoPort, m_splitter->input[0], &splitter_connection);
	if (status != MMAL_SUCCESS)
	{
		throw Exception("Error connecting preview port");
	}

	// Connect the splitter to our resizer
	MMAL_CONNECTION_T *resizer_connection = NULL;
	status = connect_ports(m_splitterPort, m_resizer->input[0], &resizer_connection);
	if (status != MMAL_SUCCESS)
	{
		throw Exception("Failed to connect camera video out port to resizer input port");
	}

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

	if (m_outputQueue != NULL)
	{
		mmal_queue_destroy(m_outputQueue);
	}

	if (m_camera != NULL)
	{
		mmal_component_destroy(m_camera);
	}

	if (m_preview != NULL)
	{
		mmal_component_destroy(m_preview);
	}

	if (m_pool != NULL)
	{
		mmal_port_pool_destroy(m_targetPort, m_pool);
	}

	if (m_splitter != NULL)
	{
		mmal_component_destroy(m_splitter);
	}

	if (m_resizer != NULL)
	{
		mmal_component_destroy(m_resizer);
	}

	delete m_callbackData;
}

void MmalVideoCamera::createSplitter()
{
	if (m_videoPort == NULL)
	{
		throw Exception("Video port is not configured");
	}

	if (m_splitter != NULL)
	{
		throw Exception("The splitter is already configured");
	}

	MMAL_PORT_T *input_port = NULL, *output_port = NULL;
	MMAL_STATUS_T status;

	//create the camera component
	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_SPLITTER, &m_splitter);
	if (status != MMAL_SUCCESS)
	{
		throw Exception("Failed to create splitter component");
	}

	//check we have output ports
	if (m_splitter->output_num != 4 || m_splitter->input_num != 1)
	{
		throw Exception("Splitter doesn't have correct ports");
	}

	//get the ports
	input_port = m_splitter->input[0];
	mmal_format_copy(input_port->format, m_videoPort->format);

	input_port->buffer_num = 3;

	status = mmal_port_format_commit(input_port);
	if (status != MMAL_SUCCESS)
	{
		throw Exception("Couldn't set resizer input port format");
	}

	for(unsigned i = 0; i < m_splitter->output_num; i++)
	{
		output_port = m_splitter->output[i];
		output_port->buffer_num = 3;

		mmal_format_copy(output_port->format, input_port->format);
		status = mmal_port_format_commit(output_port);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Couldn't set resizer output port format");
		}
	}

	m_splitterPort = m_splitter->output[MMAL_SPLITTER_CAPTURE_PORT];
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
	if (mmal_port_parameter_set_boolean(m_videoPort, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
	{
		throw Exception("Error starting the camera capture");
	}
}

void MmalVideoCamera::createResizer()
{
	MMAL_COMPONENT_T *resizer = NULL;
	MMAL_PORT_T *resizer_input = NULL, *resizer_output = NULL;
	MMAL_STATUS_T status;
	MMAL_QUEUE_T* output_queue = NULL;

	if (m_videoPort == NULL)
	{
		throw Exception("Video port is NULL");
	}

	try
	{
		status = mmal_component_create("vc.ril.resize", &resizer);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Unable to create image resizer component");
		}

		if (resizer->input_num != 1 || resizer->output_num != 1)
		{
			throw Exception("Resizer doesn't have input/output ports");
		}

		resizer_output = resizer->output[0];
		resizer_input = resizer->input[0];

		// Specify the input format
		mmal_format_copy(resizer_input->format, m_videoPort->format);
		resizer_input->buffer_num = 3;
		status = mmal_port_format_commit(resizer_input);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Unable to set format on image resizer input port");
		}

		// Specify out output format
		mmal_format_copy(resizer_output->format, resizer_input->format);
		resizer_output->format->encoding = MMAL_ENCODING_RGBA;          //MMAL_ENCODING_BGR24; // MMAL_ENCODING_RGBA
		resizer_output->format->encoding_variant = MMAL_ENCODING_RGBA;  //MMAL_ENCODING_BGR24; // MMAL_ENCODING_RGBA
		resizer_output->format->es->video.width = VCOS_ALIGN_UP(CAMERA_IMAGE_WIDTH, 32);
		resizer_output->format->es->video.height = VCOS_ALIGN_UP(CAMERA_IMAGE_HEIGHT, 16);
		resizer_output->format->es->video.crop.x = 0;
		resizer_output->format->es->video.crop.y = 0;
		resizer_output->format->es->video.crop.width = VCOS_ALIGN_UP(CAMERA_IMAGE_WIDTH, 32);
		resizer_output->format->es->video.crop.height = VCOS_ALIGN_UP(CAMERA_IMAGE_HEIGHT, 16);
		resizer_output->format->es->video.frame_rate.num = FULL_RES_VIDEO_FRAME_RATE_NUM;
		resizer_output->format->es->video.frame_rate.den = FULL_RES_VIDEO_FRAME_RATE_DEN;

		// Commit the port changes to the output port
		status = mmal_port_format_commit(resizer_output);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Unable to set format on image resizer output port");
		}

		// Enable the resizer component
		status = mmal_component_enable(resizer);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Error enabling resizer");
		}

		/* UML
		//create the output queue
		output_queue = mmal_queue_create();
		if(!output_queue)
		{
			throw Exception("Error creating output queue");
		}
		*/
	}
	catch (Exception& ex)
	{
		std::cerr << ex << std::endl;
		if (resizer != NULL)
		{
			mmal_component_destroy(resizer);
			resizer = NULL;
		}

		throw;
	}
	catch (...)
	{
		std::cerr << "Unknown error occurred" << std::endl;
		if (resizer != NULL)
		{
			mmal_component_destroy(resizer);
			resizer = NULL;
		}

		throw;
	}

	m_resizer = resizer;
	m_resizerPort = resizer_output;
	m_outputQueue = output_queue;
	m_callbackData->outputQueue = m_outputQueue;
}

void MmalVideoCamera::createPreview()
{
	MMAL_COMPONENT_T * preview = NULL;
	MMAL_STATUS_T status;

	try
	{
		status = mmal_component_create("vc.null_sink", &preview);
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
		cameraConfig.max_stills_w = CAMERA_IMAGE_WIDTH;
		cameraConfig.max_stills_h = CAMERA_IMAGE_HEIGHT;
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

		//
		// Setup the preview port
		//
		format = preview_port->format;
		format->encoding = MMAL_ENCODING_OPAQUE;
		format->encoding_variant = MMAL_ENCODING_I420;

		format->es->video.width = VCOS_ALIGN_UP(FULL_FOV_PREVIEW_4x3_X, 32);
		format->es->video.height = VCOS_ALIGN_UP(FULL_FOV_PREVIEW_4x3_Y, 16);
		format->es->video.crop.x = 0;
		format->es->video.crop.y = 0;
		format->es->video.crop.width = FULL_FOV_PREVIEW_4x3_X;
		format->es->video.crop.height = FULL_FOV_PREVIEW_4x3_Y;
		format->es->video.frame_rate.num = FULL_RES_VIDEO_FRAME_RATE_NUM;
		format->es->video.frame_rate.den = FULL_RES_VIDEO_FRAME_RATE_DEN;

		status = mmal_port_format_commit(preview_port);
		if (status != MMAL_SUCCESS)
		{
			throw Exception("Failure to setup camera preview port");
		}

		//
		// Setup the video port
		//
		format = video_port->format;
		format->encoding = MMAL_ENCODING_I420;
		format->encoding_variant = MMAL_ENCODING_I420;

		format->es->video.width = VCOS_ALIGN_UP(CAMERA_IMAGE_WIDTH, 32);
		format->es->video.height = VCOS_ALIGN_UP(CAMERA_IMAGE_HEIGHT, 16);
		format->es->video.crop.x = 0;
		format->es->video.crop.y = 0;
		format->es->video.crop.width = CAMERA_IMAGE_WIDTH;
		format->es->video.crop.height = CAMERA_IMAGE_HEIGHT;
		format->es->video.frame_rate.num = FULL_RES_VIDEO_FRAME_RATE_NUM;
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

		// Use the video format for images as well
		format = still_port->format;

		// Set our stills format on the stills (for encoder) port


		format->encoding = MMAL_ENCODING_I420;
		format->encoding_variant = MMAL_ENCODING_I420;

		format->es->video.width = VCOS_ALIGN_UP(CAMERA_IMAGE_WIDTH, 32);
		format->es->video.height = VCOS_ALIGN_UP(CAMERA_IMAGE_HEIGHT, 16);
		format->es->video.crop.x = 0;
		format->es->video.crop.y = 0;
		format->es->video.crop.width = CAMERA_IMAGE_WIDTH;
		format->es->video.crop.height = CAMERA_IMAGE_HEIGHT;
		format->es->video.frame_rate.num = FULL_RES_VIDEO_FRAME_RATE_NUM;
		format->es->video.frame_rate.den = FULL_RES_VIDEO_FRAME_RATE_DEN;

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

void MmalVideoCamera::acquireImage(Image * image)
{
	// Enable the encoder output port
	std::cout << "Acquiring image..." << std::endl;

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
		if (mmal_port_parameter_set_boolean(m_videoPort, MMAL_PARAMETER_CAPTURE, 1) == MMAL_SUCCESS)
		{

		}

	/*
	m_targetPort->userdata = (struct MMAL_PORT_USERDATA_T *) m_callbackData;

	m_callbackData->cs.enter();
	m_callbackData->buffer = image->getPixels();
	m_callbackData->pool = m_pool;
	m_callbackData->bufferSize = image->getPixelBufferSize();
	m_callbackData->bytesWritten = 0;
	m_callbackData->cs.leave();
	*/

	/* UML
	unsigned char * dst = image->getPixels();

	if(MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(m_outputQueue))
	{
		//lock it
		mmal_buffer_header_mem_lock(buffer);

		// memcpy(pData->buffer, buffer->data, writeSize);

		// The data is RGBA so we must copy just the RGB bytes
		int step = 1;
		for (unsigned i = 0; i < buffer->length; i++)
		{
			* dst = buffer->data[i];
			dst++;

			if (step == 3)
			{
				i++;
				step = 1;
			}
		}

		mmal_buffer_header_mem_unlock(buffer);
		mmal_buffer_header_release(buffer);

		// and send it back to the port (if still open)
		if (m_targetPort->is_enabled)
		{
			MMAL_STATUS_T status = MMAL_SUCCESS;
			MMAL_BUFFER_HEADER_T *new_buffer;
			new_buffer = mmal_queue_get(m_pool->queue);
			if (new_buffer)
			{
				status = mmal_port_send_buffer(m_targetPort, new_buffer);
			}

			if (!new_buffer || status != MMAL_SUCCESS)
			{
				std::cerr << "Unable to return a buffer to the video port\n";
			}
		}
	}
	else
	{
		std::cerr << "!! No buffer available" << std::endl;
	}
	*/

	std::cout << "Acquisition done..." << std::endl;
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

	//memcpy(buffer, image.getPixels(), image.getPixelBufferSize());

	// Convert the image to a JPEG
	Image::convertToJpeg(image, buffer, size);

	return true;
}

int MmalVideoCamera::getImageHeight() const
{
	return CAMERA_IMAGE_HEIGHT;
}

int MmalVideoCamera::getImageWidth() const
{
	return CAMERA_IMAGE_WIDTH;
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
