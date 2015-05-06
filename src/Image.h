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

#pragma once

namespace freelss
{

/** RGB Image with the same dimensions that the camera takes */
class Image
{
public:
	Image();
	~Image();
	
	unsigned getHeight() const;
	unsigned getWidth() const;
	unsigned getNumComponents() const;
	unsigned char * getPixels() const;
	
	/** The size of the allocated pixel buffer */
	unsigned getPixelBufferSize() const;

	/** Converts the image to a JPEG */
	static void convertToJpeg(const Image& image, byte* buffer, unsigned * size);

	/** Writes the image as a JPEG */
	static void writeJpeg(const Image& image, const std::string& filename);

	/** Overlay the given pixels as full red on top of the given image */
	static void overlayPixels(Image& image, PixelLocation * locations, int numLocations);
private:
	unsigned char * m_pixels;
	unsigned m_numComponents;
	unsigned m_width;
	unsigned m_height;
};

}
