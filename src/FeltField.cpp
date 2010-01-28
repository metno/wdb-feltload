/*
 wdb

 Copyright (C) 2007 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 E-mail: wdb@met.no

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 MA  02110-1301, USA
 */

#include "FeltField.h"
#include "FeltFile.h"
#include "feltTypeConversion.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>
#include <stdexcept>

#include <iostream>

using namespace std;

namespace felt
{

FeltField::FeltField(const FeltFile & ff, size_t index)
	: feltFile_(ff), index_(index)
{
//	if ( index_ >= feltFile_.size() )
//		throw out_of_range("Felt file does not have enough entries.");

	FeltFile::size_type blockNo = (index / (blockWords/16)) + offsetToContentDefinition;
	FeltFile::Block b = feltFile_.getBlock_(blockNo);

	FeltFile::size_type indexInBlock = (index % (blockWords/16)) * 16;

	copy(b.get() + indexInBlock, b.get() + indexInBlock + 16, header_.begin());
}


FeltField::~FeltField()
{
}

boost::posix_time::ptime FeltField::referenceTime() const
{
	return parseTime(header_.data() + 2);
}

boost::posix_time::ptime FeltField::validTime() const
{
	return referenceTime() + boost::posix_time::hours(header_[9]);
}

int FeltField::parameter() const
{
	int param = parameterUnmodified_();

	// Parameter is part of an ensemble run
	if ( param >= 4000 and param < 5000 )
		param -= 4000;

	return param;
}


FeltGridDefinitionPtr FeltField::projectionInformation() const
{
	const std::vector<word> & gh = getGridHeader_();
	std::vector<short> extraGeometrySpec;
	getExtraGeometrySpecification_(extraGeometrySpec);
	int gType = gridType();
	if ( gType > 999 )
		gType /= 1000;
	FeltGridDefinitionPtr ret = FeltGridDefinitionPtr(new FeltGridDefinition(gType,
														xNum(),
														yNum(),
														static_cast<short int>(getGridHeader_()[14]),
														static_cast<short int>(getGridHeader_()[15]),
														static_cast<short int>(getGridHeader_()[16]),
														static_cast<short int>(getGridHeader_()[17]),
														extraGeometrySpec));
	return ret;
}

//	contentSummary(std::cout, * ret);

int FeltField::xNum() const
{
	short int r = static_cast<short int>(getGridHeader_()[9]);
	return r;
}
int FeltField::yNum() const
{
	short int r = static_cast<short int>(getGridHeader_()[10]);
	return r;
}

int FeltField::scaleFactor() const
{
	short int r = static_cast<short int>(getGridHeader_()[19]);
	return r;
}



std::string FeltField::information() const
{
	std::ostringstream cont;
//	cont << index_ << ":\n";
	cont << "\tProducer\t" << header_[0] << "\n";
	cont << "\tGrid area\t" << header_[1] << "\n";
	cont << "\tTime\t\t" << referenceTime() << "\n";
	cont << "\tN - block no\t" << header_[5] << "\n";
	cont << "\tWord no\t\t" << header_[6] << "\n";
	cont << "\tData length\t" << header_[7] << "\n";
	cont << "\tData type\t" << header_[8] << "\n";
	cont << "\tTime parameter\t" << header_[9] << "\n";
	cont << "\tVertical coord\t" << header_[10] << "\n";
	cont << "\tParameter\t" << header_[11] << "\n";
	cont << "\tLevel 1\t\t" << header_[12] << "\n";
	cont << "\tLevel 2\t\t" << header_[13] << "\n";
	cont << "\tGrid type\t" << header_[14] << "\n";
	cont << "\tLX * 100 + NX\t" << header_[15] << "\n";

	return cont.str();
}

std::string FeltField::gridInformation() const
{
//	FeltFile::Block b = feltFile_.getBlock_(startingGridBlock());
	const std::vector<word> & b = getGridHeader_();

	std::ostringstream cont;
	cont << "Starting block " << startingGridBlock() << "\n";
	cont << "\tProducer\t" << b[0] << "\n";
	cont << "\tGrid area\t" << b[1] << "\n";
	cont << "\tData type\t" << b[2] << "\n";
	cont << "\tTime parameter\t" << b[3] << "\n";
	cont << "\tVertical coord\t" << b[4] << "\n";
	cont << "\tParameter\t" << b[5] << "\n";
	cont << "\tLevel 1\t\t" << b[6] << "\n";
	cont << "\tLevel 2\t\t" << b[7] << "\n";
	cont << "\tGrid type\t" << b[8] << "\n";
	cont << "\txNum\t\t" << b[9] << "\n";
	cont << "\tyNum\t\t" << b[10] << "\n";
	cont << "\tTime\t\t" << b[11]<<'-'<< b[12]/100<<'-'<<b[12]%100 << 'T'
			<< b[13]/100<<':'<<b[13]%100 <<":00\n";
	cont << "\torigo lat\t" << felt::get<short int>(b[14])/100.0 << "\n"; 	// assume geographic grid for now:
	cont << "\torigo lon\t" << felt::get<short int>(b[15])/100.0 << "\n";
	cont << "\tGrid distance B\t" << felt::get<short int>(b[16])/100.0 << "\n";
	cont << "\tGrid distance L\t" << felt::get<short int>(b[17])/100.0 << "\n";
	cont << "\tMisc\t\t" << b[18] << "\n";
	cont << "\tScale\t\t" << scaleFactor() << "\n";
	cont << "\n\tGrid size\t" << gridSize() << "\n";
	cont << "{ ";
	vector<short> extraGs;
	getExtraGeometrySpecification_(extraGs);
	for ( vector<short>::const_iterator it = extraGs.begin(); it != extraGs.end(); ++ it )
		cont << * it << " ";
	cont << " }\n";
	return cont.str();
}

bool FeltField::isSane() const
{
	const std::vector<word> & b = getGridHeader_();
	return b[0] == header_[0] && b[1] == header_[1];
}

int FeltField::level1() const
{
	int ret = header_[12];
	if ( ret == 1000 and verticalCoordinate() == 2 )
		ret = 0;
	return ret;
}

int FeltField::level2() const
{
	if ( isEpsSingleRunParameter() )
		return level1();
	return header_[13];
}


void FeltField::grid(std::vector<word> & out) const
{
	size_t from = (startingGridBlock() * blockWords) + 20;
	size_t size = gridSize();
	feltFile_.get_(out, from, size);
}


size_t FeltField::startingGridBlock() const
{
	int N = header_[5];
	if (N == -32767)
		return 0;
	-- N;

	size_t NX = header_[15] % 100;
	return N + (NX * 32767);
}

size_t FeltField::gridSize() const
{
	int L = header_[7];
	if ( L == -32767 )
		return 0;
	size_t LX = header_[15] / 100;

	// 20 is size of header
	// if gridType > 1000, then gridType()%1000 is size of
	// extra data appended to the grid
	size_t extraData = 0;
	if (gridType() > 1000)
		extraData = (gridType() % 1000);
	size_t size = L + (LX*32767) - (20 + extraData);

	return size;
}

int FeltField::dataVersion() const
{
	if ( isEpsSingleRunParameter() )
		return header_[13]; // level2 unmodified
	return 0;
}


const std::vector<word> & FeltField::getGridHeader_() const
{
	if ( gridHeader_.empty() )
	{
		size_t startingBlock = startingGridBlock();
		size_t readFrom = startingBlock * blockWords;
		feltFile_.get_(gridHeader_, readFrom, 20);
	}
	return gridHeader_;
}

void FeltField::getExtraGeometrySpecification_(std::vector<short int> & out) const
{
	int gt = gridType();
	if ( gt < 1000 ) // No extra spec
		return;
	size_t readSize = gt % 1000; // last three digits is size of appended data
	out.resize(readSize);

	size_t readFrom = (startingGridBlock() * blockWords) + gridSize() + 20;
	std::vector<word> gs;
	feltFile_.get_(gs, readFrom, readSize);

//	for_each(gs.get(), gs.get() + readSize, swapByteOrder);

//	for ( int i = 0; i < readSize; ++ i )
//		std::cout << "_"<<gs[i];
//	std::cout << "_  ";

	copy(gs.begin(), gs.end(), out.begin());
}


}
