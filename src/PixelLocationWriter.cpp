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
#include "PixelLocationWriter.h"
#include "Image.h"

namespace freelss
{

void PixelLocationWriter::writePixels(const PixelLocation * pixels,
		                              int numPixels,
		                              int imageWidth,
		                              int imageHeight,
		                              const std::string& pngFilename)
{
	std::cout << "Writing " << pngFilename << "..." << std::endl;

	FILE *fp = fopen(pngFilename.c_str(), "wb");
	if (!fp)
	{
		throw Exception(std::string("Error opening png file for writing: ") + pngFilename);
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	setjmp(png_jmpbuf(png_ptr));
	png_init_io(png_ptr, fp);
	setjmp(png_jmpbuf(png_ptr));

	png_set_IHDR(png_ptr, info_ptr, imageWidth, imageHeight,
	             8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	setjmp(png_jmpbuf(png_ptr));

	png_write_info(png_ptr, info_ptr);

	unsigned char * row = new unsigned char[imageWidth * 3];

	for (int y = 0; y < imageHeight; y++)
	{
		// Zero the row
		memset(row, 0, imageWidth * 3);

		for (int iPx = 0; iPx < numPixels; iPx++)
		{
			// If this pixels should be lit up
			if (ROUND(pixels[iPx].y) == y)
			{
				int x = ROUND(pixels[iPx].x);
				row[x * 3 + 0] = 255;
				row[x * 3 + 1] = 0;
				row[x * 3 + 2] = 0;
			}
		}

		// Write the row
		png_write_row(png_ptr, row);

	}

	delete [] row;
	png_write_end(png_ptr, NULL);
	fclose(fp);
}

void PixelLocationWriter::writeImage(Image& image, int dstWidth, int dstHeight, const std::string& pngFilename)
{
	std::cout << "Writing " << pngFilename << "..." << std::endl;
	FILE *fp = fopen(pngFilename.c_str(), "wb");
	if (!fp)
	{
		throw Exception(std::string("Error opening png file for writing: ") + pngFilename);
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	setjmp(png_jmpbuf(png_ptr));
	png_init_io(png_ptr, fp);
	setjmp(png_jmpbuf(png_ptr));

	png_set_IHDR(png_ptr, info_ptr, dstWidth, dstHeight,
				 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	setjmp(png_jmpbuf(png_ptr));

	png_write_info(png_ptr, info_ptr);

	// Sanity check
	if (image.getNumComponents() != 3)
	{
		throw Exception("Unsupported number of image components");
	}

	unsigned char * row = new unsigned char[dstWidth * 3];

	int imgHeight = (int)image.getHeight();
	int imgWidth = (int)image.getWidth();

	for (int y = 0; y < dstHeight; y++)
	{
		int srcY = y;
		if (dstHeight != imgHeight)
		{
			float yPct = y / (float)dstHeight;
			srcY = image.getHeight() * yPct;
			if (srcY >= imgHeight)
			{
				srcY = imgHeight - 1;
			}
		}

		const byte * srcRow = image.getPixels() + (imgWidth * srcY * 3);

		for (int x = 0; x < dstWidth; x++)
		{
			int srcX = x * 3;
			if (dstWidth != imgWidth)
			{
				float xPct = x / (float)dstWidth;
				srcX = (int)(image.getWidth() * xPct) * 3;
			}

			row[x * 3 + 0] = srcRow[srcX + 0];
			row[x * 3 + 1] = srcRow[srcX + 1];
			row[x * 3 + 2] = srcRow[srcX + 2];
		}

		// Write the row
		png_write_row(png_ptr, row);
	}

	delete [] row;
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
}

}
