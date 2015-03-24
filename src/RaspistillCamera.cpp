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
#include "RaspistillCamera.h"

namespace freelss
{


struct LibJpegErrorManager {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

METHODDEF(void) LibJpegErrorHandler (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  LibJpegErrorManager * myerr = (LibJpegErrorManager *) cinfo->err;

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

RaspistillCamera::RaspistillCamera() :
	m_numReadFromDisk(0),
	m_imageCount(0)
{
	// Do nothing
}

void RaspistillCamera::acquireImage(Image * image)
{
	m_cs.enter();
	try
	{
		readFromPi(image);
	}
	catch(...)
	{
		m_cs.leave();
		throw;
	}

	m_cs.leave();
}

void RaspistillCamera::readFromPi(Image * image)
{
	//
	// Write the JPEG to disk
	//
	std::stringstream sstr;
	sstr << "/akuma/raspistill_" << m_imageCount << ".jpg";
	std::string filename = sstr.str();

	writeToDisk(filename.c_str());


	//
	// Read the image from disk
	//
	readFromDisk(image, filename.c_str());

	m_imageCount++;
}

void RaspistillCamera::writeToDisk(const char * filename)
{
	std::stringstream sstr;
	sstr << "raspistill -t 100 -ex night -awb off -br 40 -n -o ";
	sstr << filename;

	std::string command = sstr.str();
	if (system(command.c_str()) == -1)
	{
		throw Exception(std::string("Error executing command: [") + command + "]");
	}
}

bool RaspistillCamera::acquireJpeg(byte* buffer, unsigned * size)
{
	bool fileRead = false;

	m_cs.enter();

	try
	{
		std::string filename = "/tmp/scanner_image.jpg";

		// If the file doesn't exists, write it
		if (access(filename.c_str(), F_OK) == -1)
		{
			writeToDisk(filename.c_str());
		}

		// Read the file
		struct stat s;
		if (stat(filename.c_str(), &s) == -1)
		{
			throw Exception("Error calling stat() on file: " + filename);
		}

		// Read the file if there is enough room
		if (* size >= (int)s.st_size)
		{
			FILE * fp = fopen(filename.c_str(), "r");
			if (fp == NULL)
			{
				throw Exception("Error opening file for reading: " + filename);
			}

			// Read the full file contents
			if (fread(buffer, s.st_size, 1, fp) != 1)
			{
				fclose(fp);
				throw Exception("Error reading from file: " + filename);
			}

			fclose(fp);

			// Remove the file we just read
			if (remove(filename.c_str()) == -1)
			{
				throw Exception("Error removing file: " + filename);
			}

			fileRead = true;
		}
		else
		{
			// Inform the caller of the necessary size
			* size = s.st_size;
		}
	}
	catch (...)
	{
		m_cs.leave();
		throw;
	}

	m_cs.leave();

	return fileRead;
}

void RaspistillCamera::readFromDisk(Image * image, const char * filename)
{
	struct jpeg_decompress_struct cinfo;
	struct LibJpegErrorManager jerr;
	bool decompressed = false;
	
	FILE * file = fopen(filename, "rb");
	if (file == NULL)
	{
		throw Exception(std::string("Error opening JPEG file: ") + filename);
	}

	std::cout.flush();

	try
	{
		cinfo.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = LibJpegErrorHandler;
		
		if (setjmp(jerr.setjmp_buffer))
		{
	    		jpeg_destroy_decompress(&cinfo);
			throw Exception("Error setting JPEG jump");
		}

  		jpeg_create_decompress(&cinfo);
  		jpeg_stdio_src(&cinfo, file);
  		jpeg_read_header(&cinfo, TRUE);
  		jpeg_start_decompress(&cinfo);
  		decompressed = true;
  		
  		if ((unsigned)cinfo.output_components != image->getNumComponents())
  		{
  			throw Exception("Invalid number of image pixel components");
  		}
  		
  		if (cinfo.output_width != image->getWidth())
  		{
  			throw Exception("Invalid image width");
  		}
  		
  		if (cinfo.output_height != image->getHeight())
  		{
  			throw Exception("Invalid image height");
  		}		

		unsigned char * imagePixels = image->getPixels();
		
		JSAMPROW buffer;
  		int rowStride = cinfo.output_width * cinfo.output_components;
  		unsigned iRow = 0;	
  		while (cinfo.output_scanline < cinfo.output_height) 
  		{
  			buffer = imagePixels + (iRow * rowStride);
	    		jpeg_read_scanlines(&cinfo, &buffer, 1);
	    	
	    		iRow++;
  		}
  
  		jpeg_finish_decompress(&cinfo);
	}
	catch (...)
	{
		if (decompressed)
		{
			jpeg_finish_decompress(&cinfo);
		}
		
		fclose(file);
		throw;
	}
	
	fclose(file);
	
	std::cout << " Done." << std::endl;
	m_numReadFromDisk++;
}

int RaspistillCamera::getImageHeight() const
{
	return 1944;
}

int RaspistillCamera::getImageWidth() const
{
	return 2592;
}

int RaspistillCamera::getImageComponents() const
{
	return 3;
}

real RaspistillCamera::getSensorWidth() const
{
	return 3.629;
}

real RaspistillCamera::getSensorHeight() const
{
	return 2.722;
}

real RaspistillCamera::getFocalLength() const
{
	return 3.6;
}

}
