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
#pragma once

#include "Image.h"

namespace freelss
{

/** Represents a single image in the store */
struct MmalImageStoreItem
{
	MmalImageStoreItem(unsigned width, unsigned height, unsigned numComponents);
	Image image;
	bool available;
	MMAL_BUFFER_HEADER_T * buffer;
};

/**
 * Manages the availability and memory of images for a Camera object.
 */
class MmalImageStore
{
public:
	MmalImageStore(int numImages, unsigned width, unsigned height, unsigned numComponents);
	~MmalImageStore();

	/** Returns the first available image and makes it unavailable */
	MmalImageStoreItem * reserve();

	/** Returns the first available image for the given buffer and makes it unavailable */
	MmalImageStoreItem * reserve(MMAL_BUFFER_HEADER_T * buffer);

	/** Unlocks any releases any buffers associated with the image and makes it available  */
	void release(Image * image);

private:

	/** Unlocks any releases any buffers associated with the item and makes it available  */
	void release(MmalImageStoreItem * item);

	std::vector<MmalImageStoreItem *> m_items;
};

}
