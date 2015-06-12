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

#include "Laser.h"

namespace freelss
{

class Calibrator
{
public:

	/**
	 * Computes the Z value (distance in XZ plane) from the camera to the
	 * center of the turn table.  This is the automated portion of the camera
	 * calibration and assumes that the camera orientation and Y-coordinate are
	 * already manually aligned.
	 * @param yImagePtForTableCenter - The y coordinate of the point in the image that contains the
	 *     center of the turn table.
	 */
	static real computeCameraZ(real yImagePtForTableCenter);
	static real computeLaserX(real cameraZ, real xImagePtForLaser, real yImagePtForLaser);

	static void calculateLaserPlane(Plane& outPlane, PixelLocation& outTop, PixelLocation& outBottom, Laser * laser, Laser::LaserSide side, const Vector3& laserLocation);

	/**
	 * Calculates a laser plane that compensate for laser misalignment (rotation and roll).
	 * @param pt1 - The top (or topish) laser pixel detected on an image of an flat object sitting on the XY plane.
	 * @param pt2 - The bottom (or bottomish) laser pixel detected on an image of an flat object sitting on the XY plane.
	 */
	static Plane calculateLaserPlane(const Vector3& laserLocation, const PixelLocation& pt1, const PixelLocation& pt2);

	/**
	 * Automatically detects the laser's X value by using image processing to detect the laser location
	 * on the XY plane and then calling into computeLaserX to generate the X location.
	 */
	static bool detectLaserX(/* out */ real * laserX , PixelLocation& topLocation, PixelLocation& bottomLocation, Laser * laser, Laser::LaserSide side);
	static bool intersectPlane(const Ray& ray, const Plane& plane, Vector3 * intersection);
};

}
