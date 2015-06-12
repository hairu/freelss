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
#include "LocationMapper.h"

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


void Calibrator::calculateLaserPlane(Plane& outPlane, PixelLocation& outTop, PixelLocation& outBottom, Laser * laser, Laser::LaserSide side, const Vector3& laserLocation)
{
	Camera * camera = Camera::getInstance();
	int maxNumLocations = camera->getImageHeight();
	int numLocations = 0;
	const double acquisitionDelay = 1.0;
	std::vector<PixelLocation> pixelLocations(maxNumLocations);

	// Turn the lasers off and take a picture
	laser->turnOff(Laser::ALL_LASERS);
	camera->setAcquisitionDelay(acquisitionDelay);

	Image * image1 = NULL;
	Image * image2 = NULL;

	try
	{
		// Take picture with laser off
		image1 = camera->acquireImage();

		// Take a picture with this laser on
		laser->turnOn(side);
		camera->setAcquisitionDelay(acquisitionDelay);

		image2 = camera->acquireImage();

		// Turn the laser back off
		laser->turnOff(side);
		camera->setAcquisitionDelay(acquisitionDelay);

		// Run image processing
		ImageProcessor imageProcessor;
		int firstRowLaserCol = 0;
		int numBad1, numBad2;

		numLocations = imageProcessor.process(* image1,
				* image2,
				NULL,
				&pixelLocations.front(),
				maxNumLocations,
				firstRowLaserCol,
				numBad1,
				numBad2,
				NULL);

		if (numLocations == 0)
		{
			throw Exception("Error detecting laser line");
		}

		camera->releaseImage(image1);
		camera->releaseImage(image2);
	}
	catch (...)
	{
		std::cerr << "Error detecting laser location" << std::endl;

		camera->releaseImage(image1);
		camera->releaseImage(image2);

		throw;
	}

	// Determine which pixels came from the object sitting on the XY plane as opposed to the background
	int idx1 = -1;
	int idx2 = -1;
	int maxPixelsFromCenter = 100;  // Don't use pixels that are further than this from the center of the image
	real centerOfImage = camera->getImageWidth() * 0.5;
	real xStart = centerOfImage - (maxPixelsFromCenter * 0.5);
	real xEnd = centerOfImage + (maxPixelsFromCenter * 0.5);
	real yStart = 0;
	real yEnd = 1700; // Don't consider the bottom of the image because the table is down there

	for (int iLoc = 1; iLoc < numLocations - 1; iLoc++)
	{
		if (pixelLocations[iLoc].x >= xStart &&
			pixelLocations[iLoc].x <= xEnd &&
			pixelLocations[iLoc].y >= yStart &&
			pixelLocations[iLoc].y <= yEnd)
		{
			// Look for some consistency first
			if (ABS(pixelLocations[iLoc].y - pixelLocations[iLoc - 1].y) <= 1.5)
			{
				if (idx1 == -1)
				{
					idx1 = iLoc;
				}
				else
				{
					idx2 = iLoc;
				}
			}
		}
	}

	if (idx1 == -1 || idx2 == -1)
	{
		throw Exception("Could not positively detect which pixels corresponded to the XY plane");
	}


	// Set the output variables
	outTop = pixelLocations[idx1];
	outBottom = pixelLocations[idx2];

	real avgDistFromCenterPx = (ABS(pixelLocations[idx1].x - centerOfImage) + ABS(pixelLocations[idx2].x - centerOfImage)) / 2.0f;
	std::cout << pixelLocations[idx1].x << " " << centerOfImage << " " << pixelLocations[idx2].x << " " << idx1 << " " << idx2 << " " << xStart << " " << xEnd << std::endl;
	std::cout << "Avg Dist From Center of Image: " << avgDistFromCenterPx << " px" << std::endl;

	outPlane = calculateLaserPlane(laserLocation, pixelLocations[idx1], pixelLocations[idx2]);
	std::cout << "Plane Normal: (" << outPlane.normal.x << "," << outPlane.normal.y << "," << outPlane.normal.z << ")" << std::endl;
}

Plane Calibrator::calculateLaserPlane(const Vector3& laserLocation, const PixelLocation& pixel1, const PixelLocation& pixel2)
{
	Setup * setup = Setup::get();

	std::cout << "Calculating laser plane for pt (" << pixel1.x << "," << pixel1.y << ") (" << pixel2.x << "," << pixel2.y << ")" << std::endl;

	//
	// Map the 2D image points to 3D points
	//

	Ray ray1, ray2;
	LocationMapper locMapper(laserLocation, setup->cameraLocation);
	locMapper.calculateCameraRay(pixel1, &ray1);
	locMapper.calculateCameraRay(pixel2, &ray2);

	ColoredPoint pt1, pt2;

	Plane xyPlane;
	xyPlane.point.x = 0;
	xyPlane.point.y = 0;
	xyPlane.point.z = 0;
	xyPlane.normal.x = 0;
	xyPlane.normal.y = 0;
	xyPlane.normal.z = -1;

	if (!locMapper.intersectPlane(xyPlane, ray1, &pt1, pixel1))
	{
		throw Exception("Error intersecting XY plane at point 1");
	}

	if (!locMapper.intersectPlane(xyPlane, ray2, &pt2, pixel2))
	{
		throw Exception("Error intersecting XY plane at point 2");
	}

	std::cout << "Intersection pt1: " << pt1.x << "," << pt1.y << "," << pt1.z << std::endl;
	std::cout << "Intersection pt2: " << pt2.x << "," << pt2.y << "," << pt2.z << std::endl;

	//
	// Calculate the plane
	//
	Plane plane;
	plane.point.x = pt1.x;
	plane.point.y = pt1.y;
	plane.point.z = pt1.z;

	Vector3 laserDir1 (laserLocation.x - pt1.x, laserLocation.y - pt1.y, laserLocation.z - pt1.z);
	Vector3 laserDir2 (laserLocation.x - pt2.x, laserLocation.y - pt2.y, laserLocation.z - pt2.z);

	laserDir1.cross(plane.normal, laserDir2);
	plane.normal.normalize();

	return plane;
}

bool Calibrator::detectLaserX(real * laserX, PixelLocation& topLocation, PixelLocation& bottomLocation, Laser * laser, Laser::LaserSide side)
{
	Camera * camera = Camera::getInstance();

	bool detected = false;
	PixelLocation * laserLocations = NULL;

	// Turn the lasers off and take a picture
	laser->turnOff(Laser::ALL_LASERS);
	Image * baseImage = NULL;
	Image * laserImage = NULL;

	try
	{
		baseImage = camera->acquireImage();

		// Take a picture with this laser on
		laser->turnOn(side);

		laserImage = camera->acquireImage();

		// Turn the laser back off
		laser->turnOff(side);

		//
		// Detect the laser from the image pixels
		//

		ImageProcessor imageProcessor;
		int firstRowLaserCol = 0;
		int numBad1, numBad2;
		int numLocations = 0;
		int maxNumLocations = camera->getImageHeight();
		laserLocations = new PixelLocation[maxNumLocations];

		numLocations = imageProcessor.process(* baseImage,
				* laserImage,
				NULL,
				laserLocations,
				maxNumLocations,
				firstRowLaserCol,
				numBad1,
				numBad2,
				NULL);

		camera->releaseImage(baseImage);
		camera->releaseImage(laserImage);

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

		camera->releaseImage(baseImage);
		camera->releaseImage(laserImage);
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
