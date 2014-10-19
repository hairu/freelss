/*
 ****************************************************************************
 *  Copyright (c) 2014 Uriah Liggett <hairu526@gmail.com>                   *
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

namespace scanner
{

class NeutralFileReader;

class StlWriter
{
public:
	StlWriter();


	void write(const std::string& filename, const std::vector<NeutralFileRecord>& results, bool connectLastFrameToFirst);

private:
	void writeHeader(std::ofstream& fout);
	void populateBuffer(NeutralFileRecord * records, int numRecords, NeutralFileRecord ** buffer, int maxNumRecords);
	void writeTrianglesForColumn(const std::vector<NeutralFileRecord>& currentFrame, const std::vector<NeutralFileRecord>& lastFrame, std::ofstream& fout, uint32& numTriangles);
	void writeTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3, std::ofstream& fout);

	/**
	 * Reduce the number of result rows and filter out some of the noise
	 * @param maxNumRows - The number of rows in the image the produced the frame.
	 * @param numRowBins - The total number of row bins in the entire image, not necessarily what is returned by this function.
	 */
	void lowpassFilter(std::vector<NeutralFileRecord>& output, std::vector<NeutralFileRecord>& frame, unsigned maxNumRows, unsigned numRowBins);

	/**
	 * Computes the average of all the records in the bin.
	 */
	void computeAverage(const std::vector<NeutralFileRecord>& bin, NeutralFileRecord& out);

	bool isValidTriangle(const ColoredPoint& pt1, const ColoredPoint& pt2, const ColoredPoint& pt3);

	bool readNextStep(std::vector<NeutralFileRecord>& frameC, const std::vector<NeutralFileRecord>& results, size_t & resultIndex);

	/** The max triangle edge distance in mm sq */
	real32 m_maxEdgeDistMmSq;

	real32 m_normal[3];
	uint16 m_attribute;
};

}
