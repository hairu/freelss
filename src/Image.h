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

class Camera;

/** RGB Image with the same dimensions that the camera takes */
class Image
{
public:
	Image();
	Image(unsigned width, unsigned height, unsigned numComponents);
	~Image();
	
	unsigned getHeight() const;
	unsigned getWidth() const;
	unsigned getNumComponents() const;
	unsigned char * getPixels();
	
	/** Sets the image to use pixels from a different source.  The image does not own the data in this case */
	void assignPixels(unsigned char * newPixels);

	/** Returns true if this object owns the underlying buffer */
	bool isOwner() const;

	/** The size of the allocated pixel buffer */
	unsigned getPixelBufferSize() const;

	/** Converts the image to a JPEG */
	static void convertToJpeg(Image& image, byte* buffer, unsigned * size);

	/** Writes the image as a JPEG */
	static void writeJpeg(Image& image, const std::string& filename);

	/** Overlay the given pixels as full red on top of the given image */
	static void overlayPixels(Image& image, PixelLocation * locations, int numLocations, unsigned char r = 255, unsigned char g = 0, unsigned b = 0);
private:
	unsigned char * m_pixels;
	unsigned m_numComponents;
	unsigned m_width;
	unsigned m_height;
	bool m_owner;
};

}
