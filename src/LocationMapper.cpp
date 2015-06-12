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
#include "LocationMapper.h"
#include "PresetManager.h"
#include "Camera.h"

namespace freelss
{

LocationMapper::LocationMapper(const Vector3& laserLoc, const Vector3& cameraLoc) :
	m_laserX(laserLoc.x),
	m_laserY(laserLoc.y),
	m_laserZ(laserLoc.z),
	m_cameraX(cameraLoc.x),
	m_cameraY(cameraLoc.y),
	m_cameraZ(cameraLoc.z),
	m_groundPlaneHeight(0)
{
	Camera * camera = Camera::getInstance();
	m_imageHeight = camera->getImageHeight();
	m_imageWidth = camera->getImageWidth();
	m_focalLength = camera->getFocalLength();
	m_sensorWidth = camera->getSensorWidth();
	m_sensorHeight = camera->getSensorHeight();

	m_maxObjectSize = PresetManager::get()->getActivePreset().maxObjectSize;
	m_groundPlaneHeight = PresetManager::get()->getActivePreset().groundPlaneHeight;

	calculateLaserPlane();
}

void LocationMapper::mapPoints(PixelLocation * laserLocations,
		                       Image * image,
		                       ColoredPoint * points,
		                       int numLocations,
	                           int & outNumLocations)
{
	real maxXZDistFromOriginSq = (m_maxObjectSize / 2) * (m_maxObjectSize / 2);
	int numIntersectionFails = 0;
	int numDistanceFails = 0;

	Ray ray;
	unsigned char * pixels = NULL;
	int rowStep = 0;
	int numComponents = 0;
	if (image != NULL)
	{
		pixels = image->getPixels();
		numComponents = image->getNumComponents();
		rowStep = image->getWidth() * numComponents;
	}

	bool haveImage = pixels != NULL;

	int pixelStart = -1;

	// Initialize our output variable
	outNumLocations = 0;
	for (int iLoc = 0; iLoc < numLocations; iLoc++)
	{
		// Compute the back projection ray
		calculateCameraRay(laserLocations[iLoc], &ray);

		// Intersect the laser plane and populate the XYZ
		ColoredPoint * point = &points[outNumLocations];
		if(intersectLaserPlane(ray, point, laserLocations[iLoc]))
		{
			// The point must be above the turn table and less than the max distance from the center of the turn table
			real distXZSq = point->x * point->x + point->z * point->z;

			if (point->y >= m_groundPlaneHeight && distXZSq < maxXZDistFromOriginSq && point->y < m_maxObjectSize)
			{
				// Set the color
				if (haveImage)
				{
					// TODO: Do we need to round this x and y value here?
					pixelStart = rowStep * ROUND(laserLocations[iLoc].y) + ROUND(laserLocations[iLoc].x) * numComponents;
					point->r = pixels[pixelStart];
					point->g = pixels[pixelStart + 1];
					point->b = pixels[pixelStart + 2];
				}

				// Make sure we have the correct laser location
				laserLocations[outNumLocations] = laserLocations[iLoc];
				outNumLocations++;
			}
			else
			{
				numDistanceFails++;
			}
		}
		else
		{
			numIntersectionFails++;
		}
	}

	if (numIntersectionFails > 0)
	{
		std::cout << "!! " << numIntersectionFails << " laser plane intersection failures." << std::endl;
	}

	if (numDistanceFails > 0)
	{
		std::cout << "!! " << numDistanceFails << " object bounds failures. " << std::endl;
	}
}

void LocationMapper::setLaserPlaneNormal(const Vector3& planeNormal)
{
	m_laserPlane.normal = planeNormal;
	m_laserPlane.normal.normalize();
}

void LocationMapper::calculateLaserPlane()
{
	// The origin is a point in the plane
	m_laserPlane.point.x = m_laserX;
	m_laserPlane.point.y = m_laserY;
	m_laserPlane.point.z = m_laserZ;

	Vector3 edge1 (m_laserX, m_laserY, m_laserZ);
	Vector3 edge2 (m_laserX, 0, m_laserZ);

	edge1.cross(m_laserPlane.normal, edge2);

	m_laserPlane.normal.normalize();
}

void LocationMapper::calculateCameraRay(const PixelLocation& imagePixel,  Ray * ray)
{
	// Performance Note: Most of this could be pre-computed
	// and all the division could be removed.
	
	// and distance to camera
	
	// We subtract by one because the image is 0 indexed
	real x = imagePixel.x / (real)(m_imageWidth - 1);
	
	// Subtract the height so it goes from bottom to top
	real y = (m_imageHeight - imagePixel.y) / (real)(m_imageHeight - 1);
	
	// The center of the sensor is at 0 in the X dimension
	x = (x * m_sensorWidth) - (m_sensorWidth * 0.5) + m_cameraX;
	y = (y * m_sensorHeight) - (m_sensorHeight * 0.5) + m_cameraY;
	real z = m_cameraZ - m_focalLength;

	// Store the ray origin
	ray->origin.x = x;
	ray->origin.y = y;
	ray->origin.z = z;
	
	// Compute the ray direction
	ray->direction.x = x - m_cameraX;
	ray->direction.y = y - m_cameraY;
	ray->direction.z = z - m_cameraZ;
	ray->direction.normalize();
}

bool LocationMapper::intersectLaserPlane(const Ray& ray, ColoredPoint * inPoint, const PixelLocation& pixel)
{
	return intersectPlane(m_laserPlane, ray, inPoint, pixel);
}

bool LocationMapper::intersectPlane(const Plane& plane, const Ray& ray, ColoredPoint * point, const PixelLocation& pixel)
{
	// Reference: http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-plane-and-ray-disk-intersection/
	// d = ((p0 - l0) * n) / (l * n)
	
	// If dn is close to 0 then they don't intersect.  This should never happen
	real denominator = ray.direction.dot(plane.normal);
	if (ABS(denominator) < 0.000001)
	{
		std::cerr << "!!! Ray never hits laser plane, pixel=" << pixel.x << ", " << pixel.y
				  << ", laserX=" << m_laserX
				  << ", denom=" << denominator << std::endl;

		return false;
	}
	
	Vector3 v;
	v.x = plane.point.x - ray.origin.x;
	v.y = plane.point.y - ray.origin.y;
	v.z = plane.point.z - ray.origin.z;
	
	real numerator = v.dot(plane.normal);
	
	// Compute the distance along the ray to the plane
	real d = numerator / denominator;
	if (d < 0)
	{
		// The ray is going away from the plane.  This should never happen.
		std::cerr << "!!! Back projection ray is going the wrong direction!  Ray Origin = ("
				  << ray.origin.x << "," << ray.origin.y << "," << ray.origin.z << ") Direction = ("
				  << ray.direction.x << "," << ray.direction.y << "," << ray.direction.z << ")"
				  << std::endl;

		return false;
	}
	
	// Extend the ray out this distance
	point->x = ray.origin.x + (ray.direction.x * d);
	point->y = ray.origin.y + (ray.direction.y * d);
	point->z = ray.origin.z + (ray.direction.z * d);
	point->normal.x = m_laserX - point->x;
	point->normal.y = m_laserY - point->y;
	point->normal.z = m_laserZ - point->z;
	point->normal.normalize();
	
	return true;
}

}
