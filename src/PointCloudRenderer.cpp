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
#include "Main.h"
#include "PointCloudRenderer.h"
#include "Setup.h"
#include "Logger.h"

namespace freelss
{

PointCloudRenderer::PointCloudRenderer(int imageWidth, int imageHeight, int pixelRadius, real theta) :
	m_image(new Image(imageWidth, imageHeight, 3)),
	m_transform(),
	m_pixelRadius(pixelRadius),
	m_depthBuffer(imageWidth * imageHeight, FLT_MAX)
{
	float fovY = DEGREES_TO_RADIANS(45.0f);
	float aspectRatio = imageWidth / static_cast<float>(imageHeight);
	float yScale = static_cast<float>(1.0 / tan(fovY * 0.5));
	float xScale = yScale / aspectRatio;
	float zNear = 1;
	float zFar = 1000;

	// Set the background color
	memset(m_image->getPixels(), 0xaa, m_image->getPixelBufferSize());

	Eigen::Matrix4f projection;
	projection <<
			xScale, 0, 0, 0,
	        0, yScale, 0, 0,
	        0, 0, (zNear + zFar)/(zNear - zFar), -1,
	        0, 0, (-2 * zNear * zFar)/(zNear - zFar), 0;

	Setup * setup = Setup::get();
	Eigen::Affine3f translation(Eigen::Translation3f(setup->cameraLocation.x, -1.0 * setup->cameraLocation.y, -1.0 * setup->cameraLocation.z));
	Eigen::Affine3f rotation(Eigen::AngleAxisf(theta, Eigen::Vector3f::UnitY()));

	Eigen::Matrix4f modelView = translation.matrix() * rotation.matrix();

	m_transform = projection * modelView;
}

PointCloudRenderer::~PointCloudRenderer()
{
	delete m_image;
}

const ColoredPoint * PointCloudRenderer::getPoint(const ColoredPoint& pt)
{
	return &pt;
}

const ColoredPoint * PointCloudRenderer::getPoint(const DataPoint& pt)
{
	return &pt.point;
}

void PointCloudRenderer::addPoints(const std::vector<ColoredPoint>& dataPoints)
{
	addPointsImpl<ColoredPoint>(dataPoints);
}

void PointCloudRenderer::addPoints(const std::vector<DataPoint>& dataPoints)
{
	addPointsImpl<DataPoint>(dataPoints);
}

template <class T>
void PointCloudRenderer::addPointsImpl(const std::vector<T>& points)
{
	int width = m_image->getWidth();
	int height = m_image->getHeight();
	int nc = m_image->getNumComponents();
	int rowSize = width * nc;
	unsigned char * pixels = m_image->getPixels();

	int halfWidth = width * 0.5f;
	int halfHeight = height * 0.5f;
	for (size_t iPt = 0; iPt < points.size(); iPt++)
	{
		const ColoredPoint * colorPt = getPoint(points[iPt]);

		Eigen::Vector4f vec = m_transform * Eigen::Vector4f(-colorPt->x, colorPt->y, colorPt->z, 1.0f);

		if (vec(3) < 0)
		{
			float depth = vec(2);
			vec /= vec(3);

			int x = vec(0) * width + halfWidth;
			int y = vec(1) * height + halfHeight;

			if (x >= 0 && x < width && y >= 0 && y < height)
			{
				setPixel(pixels, x, y, m_pixelRadius, width, height, rowSize, nc, depth, colorPt->r, colorPt->g, colorPt->b);
			}
		}
	}
}

void PointCloudRenderer::setPixel(unsigned char * pixels, int sx, int sy, int pixelRad, int width, int height, int rowSize, int nc, real depth, unsigned char r, unsigned char g, unsigned char b)
{
	if (pixelRad == 0)
	{
		if (m_depthBuffer[sy * width + sx] > depth)
		{
			pixels[rowSize * sy + sx * nc] = r;
			pixels[rowSize * sy + sx * nc + 1] = g;
			pixels[rowSize * sy + sx * nc + 2] = b;

			m_depthBuffer[sy * width + sx] = depth;
		}
	}
	else
	{
		for (int x = sx - pixelRad; x < sx + pixelRad; x++)
		{
			if (x >= 0 && x < width)
			{
				for (int y = sy - pixelRad; y < sy + pixelRad; y++)
				{
					if (y >= 0 && y < height && m_depthBuffer[y * width + x] > depth)
					{
						pixels[rowSize * y + x * nc] = r;
						pixels[rowSize * y + x * nc + 1] = g;
						pixels[rowSize * y + x * nc + 2] = b;
						m_depthBuffer[y * width + x] = depth;
					}
				}
			}
		}
	}
}

Image * PointCloudRenderer::getImage()
{
	return m_image;
}

}
