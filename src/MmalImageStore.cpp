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
#include "MmalImageStore.h"

namespace freelss
{

MmalImageStoreItem::MmalImageStoreItem(unsigned width, unsigned height, unsigned numComponents) :
	image(width, height, numComponents),
	available(true),
	buffer(NULL)
{
	// Do nothing
}

MmalImageStore::MmalImageStore(int numImages, unsigned width, unsigned height, unsigned numComponents)
{
	for (int i = 0; i < numImages; i++)
	{
		m_items.push_back(new MmalImageStoreItem(width, height, numComponents));
	}
}

MmalImageStore::~MmalImageStore()
{
	for (size_t i = 0; i < m_items.size(); i++)
	{
		release(m_items[i]);
		delete m_items[i];
	}
}

MmalImageStoreItem * MmalImageStore::reserve()
{
	MmalImageStoreItem * item = NULL;
	for (size_t i = 0; i < m_items.size(); i++)
	{
		if (m_items[i]->available)
		{
			item = m_items[i];
			item->available = false;
			break;
		}
	}

	return item;
}

MmalImageStoreItem * MmalImageStore::reserve(MMAL_BUFFER_HEADER_T * buffer)
{
	MmalImageStoreItem * item = reserve();
	if (item == NULL)
	{
		return NULL;
	}

	// Unlock the buffer and assign the pixels
	mmal_buffer_header_mem_lock(buffer);
	item->image.assignPixels(buffer->data);
	item->buffer = buffer;

	return item;
}

void MmalImageStore::release(Image * image)
{
	// Find the item associated with this image
	MmalImageStoreItem * item = NULL;
	for (size_t i = 0; i < m_items.size(); i++)
	{
		if (&m_items[i]->image == image)
		{
			item = m_items[i];
			break;
		}
	}

	if (item == NULL)
	{
		throw Exception("Could not find item associated with image");
	}

	release(item);
}

void MmalImageStore::release(MmalImageStoreItem * item)
{
	item->available = true;
	if (item->buffer != NULL)
	{
		mmal_buffer_header_mem_unlock(item->buffer);
		mmal_buffer_header_release(item->buffer);
		item->buffer = NULL;
	}
}

}
