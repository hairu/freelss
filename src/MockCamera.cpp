/*
 ****************************************************************************
 *  Copyright (c) 2015 Uriah Liggett <freelaserscanner@gmail.com>           *
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
#include "MockCamera.h"
#include "Logger.h"

namespace freelss
{

MockCamera::MockCamera() :
	m_imageWidth(-1),
	m_imageHeight(-1)
{
	m_name = "MockCamera";
	setSensorProperties(3.629, 2.722, 3.6);
}

void MockCamera::initialize(CameraMode cameraMode)
{
	m_resolution = CreateResolution(640, 480, 15, CT_UNKNOWN, cameraMode, "Mock Camera VGA");
	m_supportedResolutions.push_back(m_resolution);
	m_imageWidth = m_resolution.width;
	m_imageHeight = m_resolution.height;
}

Image * MockCamera::acquireImage()
{
	std::auto_ptr<Image> image(new Image(m_imageWidth, m_imageHeight, getImageComponents()));

	// Make the image black
	unsigned rowSpacing = image->getWidth() * image->getNumComponents();
	unsigned char * pixels = image->getPixels();
	memset(pixels, 0, image->getPixelBufferSize());

	int xWidth = ROUND(image->getWidth() / 100.0f);
	float aspect = (float)image->getWidth() / (float) image->getHeight();
	for (unsigned y = 0; y < image->getHeight(); y++)
	{
		int x = (int)(y * aspect) - xWidth / 2;
		int endX = x + xWidth;
		while (x < endX)
		{
			if (x >= 0 && x < (int)image->getWidth())
			{
				// Diagonal #1
				pixels[x * image->getNumComponents()] = 255;

				// Diagonal #2
				pixels[rowSpacing - x * image->getNumComponents()] = 255;
			}

			x++;
		}

		pixels += rowSpacing;
	}

	return image.release();
}

void MockCamera::releaseImage(Image * image)
{
	delete image;
}

void MockCamera::setShutterSpeed(unsigned shutterSpeedUs)
{
	// Do nothing
}

void MockCamera::setFlipRedBlue(bool flip)
{
	// Do nothing
}

int MockCamera::getImageHeight() const
{
	return m_imageHeight;
}

int MockCamera::getImageWidth() const
{
	return m_imageWidth;
}

int MockCamera::getImageComponents() const
{
	return 3;
}

void MockCamera::setBurstModeEnabled(bool /*enable*/)
{
	// Do nothing
}

} // end ns scanner
