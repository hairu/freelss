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
#include "Image.h"

namespace freelss
{

class LocationMapper
{
public:

	LocationMapper(const Vector3& laserLocation,
			       const Vector3& cameraLocation);
	
	/** Lookup the 3D points for each pixel location */
	void mapPoints(PixelLocation * laserLocations,
			       Image * image,
			       ColoredPoint * points,
			       int numLocations,
			       int & outNumLocations);
	
	void setLaserPlaneNormal(const Vector3& planeNormal);

	/** 
	 * Calculates a camera back projection Ray for the given image point.  The ray
	 * will begin at globalPt and extend out into the scene with a vector that would also
	 * take it through the camera focal point.
	 * @param globalPt - Point on the camera sensor in global coordinates
	 * @param ray - The output ray.
	 */
	void calculateCameraRay(const PixelLocation& imagePixel, Ray * global);

	/** Calculate where the ray will hit the plane and write it to @p point */
	bool intersectPlane(const Plane& plane, const Ray& ray, ColoredPoint * inPoint, const PixelLocation& pixel);

	/** Calculate where the ray will hit the laser plane and write it to @p point */
	bool intersectLaserPlane(const Ray& ray, ColoredPoint * inPoint, const PixelLocation& pixel);

	/** Calculate the plane equation for the plane that the laser is in */
	void calculateLaserPlane();
		
private:

	Plane m_laserPlane;
	real m_laserX;
	real m_laserY;
	real m_laserZ;
	real m_cameraX;
	real m_cameraY;
	real m_cameraZ;
	unsigned m_imageWidth;
	unsigned m_imageHeight;
	real m_focalLength;
	real m_sensorWidth;
	real m_sensorHeight;
	real m_maxObjectSize;
	real m_groundPlaneHeight;
};

}
