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

#include "feltTypeConversion.h"

namespace felt
{

boost::posix_time::ptime parseTime(const word data[3])
{
	boost::gregorian::date date(data[0], data[1]/100, data[1]%100);
	boost::posix_time::time_duration clock(data[2]/100, data[2]%100, 0);

	return boost::posix_time::ptime(date, clock);
}

boost::posix_time::ptime parseTimeNoThrow(const word * data)
{
	try
	{
		return parseTime(data);
	}
	catch(std::exception &)
	{
		return boost::posix_time::ptime();
	}
}

}
