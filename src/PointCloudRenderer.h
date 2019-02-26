/*
 ****************************************************************************
 *  Copyright (c) 2016 Uriah Liggett <freelaserscanner@gmail.com>           *
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
 ****************************************************************************/

#include "Image.h"

namespace freelss
{

/**
 * Renders 3D colored points onto on image.
 */
class PointCloudRenderer
{
public:
	/** Default Constructor */
	PointCloudRenderer(int imageWidth = 640, int imageHeight = 480, int pixelRadius = 1, real rotation = 0);
	~PointCloudRenderer();

	/** Adds the given data points to the image */
	void addPoints(const std::vector<DataPoint>& dataPoints);

	/** Adds the given data points to the image */
	void addPoints(const std::vector<ColoredPoint>& points);

	/** Returns the rendered image. */
	Image * getImage();

private:

	template<class T>
	void addPointsImpl(const std::vector<T>& dataPoints);
	const ColoredPoint * getPoint(const ColoredPoint& pt);
	const ColoredPoint * getPoint(const DataPoint& pt);

	/** Sets the image pixels to a particular color */
	void setPixel(unsigned char * pixels, int sx, int sy, int pixelRad, int width, int height, int rowSize, int nc, real depth, unsigned char r, unsigned char g, unsigned char b);

private:
	/** The image that is being populated */
	Image * m_image;

	/** The transformation matrix for the pixels */
	Eigen::Matrix4f m_transform;

	/** The radius of the pixels */
	int m_pixelRadius;

	/** The depth buffer */
	std::vector<real> m_depthBuffer;
};

} // ns sdl
