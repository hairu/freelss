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
#include "Calibrator.h"
#include "PresetManager.h"
#include "Camera.h"
#include "Laser.h"
#include "ImageProcessor.h"
#include "Setup.h"
#include "PresetManager.h"

namespace freelss
{

real Calibrator::computeCameraZ(real pixelY)
{
	// Read the Y-value for the camera from Database
	Setup * setup = Setup::get();

	Camera * camera = Camera::getInstance();
	real cameraY = setup->cameraLocation.y;

	// Camera focal length
	real focalLength = camera->getFocalLength();

	// Use trig to detect the distance to the camera
	real pixelSpacing = (real)camera->getSensorHeight() / (real)camera->getImageHeight();

	std::cout << "cameraY: " << cameraY << " mm" << std::endl;
	std::cout << "pixelY: " << pixelY << " row" << std::endl;
	std::cout << "Pixel Spacing: " << pixelSpacing << " mm" << std::endl;

	real sy = (pixelY - (camera->getImageHeight() / 2.0)) * pixelSpacing;

	std::cout << "sy on sensor: " << sy << " mm" << std::endl;
	std::cout << "focal length: " << focalLength << " mm" << std::endl;

	real theta = atan(sy / focalLength);
	std::cout << "angle is : " << (theta / (PI * 2)) * 360.0 << " degrees" << std::endl;
	real z = cameraY  / tan(theta);

	std::cout << "distance is : " << z << " mm" << std::endl;

	return z;
}

bool Calibrator::detectLaserX(real * laserX, PixelLocation& topLocation, PixelLocation& bottomLocation, Laser * laser, Laser::LaserSide side)
{
	Camera * camera = Camera::getInstance();

	// Turn the lasers off and take a picture
	laser->turnOff(Laser::ALL_LASERS);
	Image baseImage;
	camera->acquireImage(&baseImage);

	// Take a picture with this laser on
	laser->turnOn(side);
	Image laserImage;
	camera->acquireImage(&laserImage);

	// Turn the laser back off
	laser->turnOff(side);

	//
	// Detect the laser from the image pixels
	//
	bool detected = false;
	ImageProcessor imageProcessor;
	int firstRowLaserCol = 0;
	int numBad1, numBad2;
	int numLocations = 0;
	int maxNumLocations = camera->getImageHeight();
	PixelLocation * laserLocations = new PixelLocation[maxNumLocations];

	try
	{
		numLocations = imageProcessor.process(baseImage,
				laserImage,
				NULL,
				laserLocations,
				maxNumLocations,
				firstRowLaserCol,
				numBad1,
				numBad2,
				NULL);

		//
		// Find the laser location closest to the bottom of the image
		//

		// TODO: Improve this by doing a best fit line among all the laser returns
		PixelLocation * maxLocation = NULL;
		bool foundTopLocation = false;
		for (int iLoc = 0; iLoc < numLocations; iLoc++)
		{
			PixelLocation& location = laserLocations[iLoc];

			// Only look at the last 100 or so pixels in the image so that we don't
			// detect the laser reflecting off walls and things in the background
			// TODO: Make this detection range ie. 300 and 100 a Setting
			if (!foundTopLocation && location.y > (maxNumLocations - 300) && location.y < (maxNumLocations - 100))
			{
				topLocation = location;
				foundTopLocation = true;
			}

			if (maxLocation == NULL || location.y > maxLocation->y)
			{
				maxLocation = &location;
				bottomLocation = location;
			}
		}

		// Detect the laser location using this XY pixel value
		if (maxLocation != NULL)
		{
			real cameraZ = Setup::get()->cameraLocation.z;

			* laserX = computeLaserX(cameraZ, (real) maxLocation->x, (real)maxLocation->y);
			detected = true;
		}
		else
		{
			std::cerr << "Could not detect laser line for calibration purposes." << std::endl;
		}
	}
	catch (...)
	{
		std::cerr << "Error detecting laser location" << std::endl;
		// Catch all
	}

	delete [] laserLocations;
	laserLocations = NULL;

	return detected;
}

real Calibrator::computeLaserX(real cameraZ, real xPixel, real yPixel)
{
	//
	// Compute ray going from camera through the 3D scene at the clicked location
	//

	Setup * setup = Setup::get();

	real cameraX = setup->cameraLocation.x;
	real cameraY = setup->cameraLocation.y;

	Camera * camera = Camera::getInstance();
	real imageHeight = camera->getImageHeight();
	real imageWidth = camera->getImageWidth();
	real sensorWidth = camera->getSensorWidth();
	real sensorHeight = camera->getSensorHeight();
	real focalLength = camera->getFocalLength();

	// Compute the location on the sensor in world coordinates
	real pctX = xPixel / imageWidth;
	real pctY = (imageHeight - yPixel) / imageHeight;
	real worldX = cameraX + (pctX * sensorWidth) - sensorWidth * 0.5;
	real worldY = cameraY + (pctY * sensorHeight) - sensorHeight * 0.5;
	real worldZ = cameraZ - focalLength;

	Ray ray;
	ray.origin.x = cameraX;
	ray.origin.y = cameraY;
	ray.origin.z = cameraZ;

	ray.direction.x = worldX - cameraX;
	ray.direction.y = worldY - cameraY;
	ray.direction.z = worldZ - cameraZ;
	ray.direction.normalize();

	//
	// Intersect the XZ plane
	//

	Plane xzPlane;
	xzPlane.normal.x = 0;
	xzPlane.normal.y = 1;
	xzPlane.normal.z = 0;
	xzPlane.point.x = 0;
	xzPlane.point.y = 0;
	xzPlane.point.z = 0;

	Vector3 intersection;

	if (intersectPlane(ray, xzPlane, &intersection) == false)
	{
		throw Exception("XY plane was not intersected");
	}

	//
	// Intersect the camera image plane to get the laser location
	//
	Plane camPlane;
	camPlane.normal.x = 0;
	camPlane.normal.y = 0;
	camPlane.normal.z = -1;
	camPlane.point.x = cameraX;
	camPlane.point.y = cameraY;
	camPlane.point.z = cameraZ;

	// The laser is assumed to be aligned to the center of the turn table
	Ray camPlaneRay;
	camPlaneRay.origin.x = 0;
	camPlaneRay.origin.y = 0;
	camPlaneRay.origin.z = 0;
	camPlaneRay.direction = intersection;
	camPlaneRay.direction.normalize();

	if (intersectPlane(camPlaneRay, camPlane, &intersection) == false)
	{
		throw Exception("Camera plane was not intersected");
	}

	std::cout << "Detected laser at (" << intersection.x / 25.4 << "," << intersection.y / 25.4 << ","
			  << intersection.z / 25.4 << ") in. from pixel ("
			  << xPixel << "," << yPixel << ") and camera Z of " << cameraZ / 25.4 << "in. "
			  << std::endl;

	return intersection.x;
}

bool Calibrator::intersectPlane(const Ray& inRay, const Plane& plane, Vector3 * intersection)
{
	// Reference: http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-plane-and-ray-disk-intersection/
	// d = ((p0 - l0) * n) / (l * n)

	Ray ray = inRay;

	// Negate Z since this is a RH coordinate system
	ray.direction.z *= -1;

	// If dn is close to 0 then they don't intersect.  This should never happen
	real denominator = ray.direction.dot(plane.normal);
	if (ABS(denominator) < 0.00001)
	{
		return false;
	}

	Vector3 v;
	v.x = plane.point.x - ray.origin.x;
	v.y = plane.point.y - ray.origin.y;
	v.z = -1 * (plane.point.z - ray.origin.z);

	real numerator = v.dot(plane.normal);

	// Compute the distance along the ray to the plane
	real d = numerator / denominator;
	if (d < 0)
	{
		return false;
	}

	// Extend the ray out this distance
	intersection->x = inRay.origin.x + (inRay.direction.x * d);
	intersection->y = inRay.origin.y + (inRay.direction.y * d);
	intersection->z = inRay.origin.z + (inRay.direction.z * d);

	return true;
}
}
