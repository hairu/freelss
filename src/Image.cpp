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
#include "Image.h"
#include "Camera.h"

static void InitBuffer(jpeg_compress_struct* cinfo) { }
static boolean EmptyBuffer(jpeg_compress_struct* cinfo) { return TRUE; }
static void TermBuffer(jpeg_compress_struct* cinfo) { }


namespace freelss
{

Image::Image() :
	m_pixels(NULL),
	m_numComponents(0),
	m_width(0),
	m_height(0)
{
	Camera * camera = Camera::getInstance();
	m_pixels = new unsigned char [camera->getImageComponents() * camera->getImageHeight() * camera->getImageWidth()];
	m_height = camera->getImageHeight();
	m_width = camera->getImageWidth();
	m_numComponents = camera->getImageComponents();
}

Image::~Image()
{
	delete [] m_pixels;
}

unsigned Image::getHeight() const
{
	return m_height;
}

unsigned Image::getWidth() const
{
	return m_width;
}

unsigned char * Image::getPixels() const
{
	return m_pixels;
}

unsigned Image::getNumComponents() const
{
	return m_numComponents;
}

unsigned Image::getPixelBufferSize() const
{
	return m_width * m_height * m_numComponents;
}

void Image::convertToJpeg(const Image& image, byte* buffer, unsigned * size)
{
	size_t bufferSize = * size;

	struct jpeg_destination_mgr dmgr;
	dmgr.init_destination    = InitBuffer;
	dmgr.empty_output_buffer = EmptyBuffer;
	dmgr.term_destination    = TermBuffer;
	dmgr.next_output_byte    = buffer;
	dmgr.free_in_buffer      = bufferSize;

	struct jpeg_compress_struct cinfo;

	struct jpeg_error_mgr       jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	cinfo.dest = &dmgr;
	cinfo.image_width      = image.getWidth();
	cinfo.image_height     = image.getHeight();
	cinfo.input_components = image.getNumComponents();
	cinfo.in_color_space   = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality (&cinfo, 90, true);
	jpeg_start_compress(&cinfo, true);

	unsigned rowSize = cinfo.image_width * cinfo.input_components;

	JSAMPROW rowPointer;
	unsigned char * pixels = image.getPixels();

	// Write the JPEG data
	while (cinfo.next_scanline < cinfo.image_height)
	{
		rowPointer = (JSAMPROW) &pixels[cinfo.next_scanline * rowSize];
		jpeg_write_scanlines(&cinfo, &rowPointer, 1);
	}

	jpeg_finish_compress(&cinfo);

	* size = bufferSize - dmgr.free_in_buffer;
}


void Image::writeJpeg(const Image& image, const std::string& filename)
{
	std::cout << "Writing " << filename << "..." << std::endl;

	// Get the image from the camera
	unsigned imageSize = image.getPixelBufferSize();
	byte * imageData = (byte *) malloc(imageSize);
	if (imageData == NULL)
	{
		throw Exception("Out of memory");
	}

	std::ofstream fout (filename.c_str());
	if (!fout.is_open())
	{
		free(imageData);
		throw Exception("Error opening file for writing: " + filename);
	}

	try
	{
		// Convert to JPEG
		Image::convertToJpeg(image, imageData, &imageSize);

		// Write the JPEG to the disk
		fout.write((const char *)imageData, imageSize);
	}
	catch (...)
	{
		fout.close();
		free(imageData);
		throw;
	}

	fout.close();
	free(imageData);
}

void Image::overlayPixels(Image& image, PixelLocation * locations, int numLocations)
{
	int width = image.getWidth();
	int height = image.getHeight();
	int numComp = image.getNumComponents();
	unsigned char * pixels = image.getPixels();

	int rowStep = (width * numComp);
	for (int iLoc = 0; iLoc < numLocations; iLoc++)
	{
		const PixelLocation & location = locations[iLoc];
		int x = ROUND(location.x);
		if (x < 0) x = 0;
		if (x >= width) x = width - 1;

		int y = ROUND(location.y);
		if (y < 0) y = 0;
		if (y >= height) y = height - 1;

		// Find the pixels
		unsigned char * px = pixels + (rowStep * y + x * numComp);

		// Set the pixel to red
		px[0] = 255;
		px[1] = 0;
		px[2] = 0;
	}
}

}
