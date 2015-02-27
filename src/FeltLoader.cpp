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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "FeltLoader.h"
#include "FeltFile.h"
#include <wdb/errors.h>
#include <wdb/LoaderDatabaseConnection.h>
#include <wdbLogHandler.h>
#include <GridGeometry.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <algorithm>
#include <functional>
#include <cmath>
#include <sstream>
#include <pqxx/util>

using namespace std;
using namespace wdb;
using namespace wdb::load;
using namespace boost::posix_time;
using namespace boost::filesystem;

namespace {

path getConfigFile( const path & sysPath, const path & fileName )
{
	path confPath;
	if (sysPath.empty())
		confPath = SYSCONFDIR/fileName;
	else
		confPath = sysPath/fileName;
	return confPath;
}

}

namespace felt
{

FeltLoader::FeltLoader(	LoaderDatabaseConnection & connection,
						const wdb::load::LoaderConfiguration::LoadingOptions & loadingOptions,
						const wdb::load::LoaderConfiguration::MetadataOptions & metadataOptions,
						wdb::WdbLogHandler & logHandler )
	: connection_(connection),
	  loadingOptions_(loadingOptions),
	  logHandler_(logHandler)
{
	felt2DataProviderName_.open( getConfigFile(metadataOptions.path, "dataprovider.conf").string() );
	felt2ValidTime_.open( getConfigFile(metadataOptions.path, "validtime.conf").string() );
	felt2ValueParameter_.open( getConfigFile(metadataOptions.path, "valueparameter.conf").string() );
	felt2LevelParameter_.open( getConfigFile(metadataOptions.path, "levelparameter.conf").string() );
	felt2LevelAdditions_.open( getConfigFile(metadataOptions.path, "leveladditions.conf").string() );
}

FeltLoader::~FeltLoader()
{
	// NOOP
}

void FeltLoader::load(const FeltFile & file)
{
	WDB_LOG & log = WDB_LOG::getInstance( "wdb.feltLoad.load.file" );
	log.debugStream() << file.information();
    int objectNumber = 0;
    for ( FeltFile::const_iterator it = file.begin(); it != file.end(); ++ it )
    {
    	logHandler_.setObjectNumber(objectNumber ++);
		load(**it);
    }
}

namespace
{
	std::string toString(const boost::posix_time::ptime & time )
	{
		if ( time == boost::posix_time::ptime(neg_infin) )
			return "-infinity"; //"1900-01-01 00:00:00+00";
		else if ( time == boost::posix_time::ptime(pos_infin) )
			return "infinity";//"2100-01-01 00:00:00+00";
		// ...always convert to zulu time
		std::string ret = to_iso_extended_string(time) + "+00";
		return ret;
	}
}

void FeltLoader::load(const felt::FeltField & field)
{
	WDB_LOG & log = WDB_LOG::getInstance( "wdb.feltloader.load.field" );
    std::string unit;
	try
	{
		if ( field.gridSize() < 1 )
			throw std::runtime_error("Grid have size 0 - will not load");

		if ( field.gridSize() != field.xNum() * field.yNum() )
			throw std::runtime_error("Internal inconsistency in file - grid size does not match x-number * y-number");

	    std::string dataProvider = dataProviderName( field );
	    std::string place = placeName( field );
	    std::string valueParameter = valueParameterName( field );
	    std::vector<wdb::load::Level> levels;
	    levelValues( levels, field );
	    std::vector<float> data;
	    getValues(data, field);
	    for ( unsigned int i = 0; i<levels.size(); i++ ) {
	    	connection_.write ( & data[0],
								data.size(),
								dataProvider,
								place,
								toString( referenceTime( field ) ),
								toString( validTimeFrom( field ) ),
								toString( validTimeTo( field ) ),
								valueParameter,
								levels[i].levelParameter_,
								levels[i].levelFrom_,
								levels[i].levelTo_,
								dataVersion( field ),
								confidenceCode( field ) );
	    }
	}
	catch ( wdb::ignore_value &e )
	{
		log.infoStream() << e.what() << " Data field not loaded.";
	}
	catch ( std::out_of_range &e )
	{
		wdb::load::registerError(wdb::load::FieldFailedToLoad);
		log.warnStream() << "Metadata missing for data value. " << e.what() << " Data field not loaded.";
	}
	/* Note supported < pqxx 3.0.0
	catch (pqxx::unique_violation &e) {
		// Duplicate key violations - downgraded to warning level
		log.warnStream() << e.what() << " Data field not loaded.";
	}
	*/
	catch ( std::exception & e )
	{
		wdb::load::registerError(wdb::load::FieldFailedToLoad);
		log.warnStream() << e.what() << " Data field not loaded.";
	}
}

std::string FeltLoader::dataProviderName(const FeltField & field)
{
	stringstream keyStr;
	keyStr << field.producer() << ", "
		   << field.gridArea();
	std::string ret = felt2DataProviderName_[keyStr.str()];
	return ret;
}

std::string FeltLoader::placeName(const FeltField & field)
{
	FeltGridDefinitionPtr grid = field.projectionInformation();
	try
	{
		return connection_.getPlaceName( grid->numberX(), grid->numberY(),
										 grid->incrementX(), grid->incrementY(),
										 grid->startX(), grid->startY(),
										 grid->projDefinition() );
	}
	catch ( std::exception & e )
	{
		if ( not loadingOptions_.loadPlaceDefinition )
			throw;
	}

	std::string newPlaceName = loadingOptions_.placeName;
	if ( newPlaceName.empty() )
	{
		std::ostringstream name;
		name << grid->numberX() << ',' << grid->numberY() << " res " << grid->incrementX();
		if ( grid->incrementX() != grid->incrementY() )
			name << ',' << grid->incrementY();
		name << " start " << grid->startX() << ',' << grid->startY();
		newPlaceName = name.str();
	}

	newPlaceName = connection_.addPlaceDefinition(
													newPlaceName,
													grid->numberX(), grid->numberY(),
													grid->incrementX(), grid->incrementY(),
													grid->startX(), grid->startY(),
													grid->projDefinition()
												);
	return newPlaceName;
}

boost::posix_time::ptime FeltLoader::referenceTime(const FeltField & field)
{
	return field.referenceTime();
}

boost::posix_time::ptime FeltLoader::validTimeFrom(const FeltField & field)
{
	stringstream keyStr;
	keyStr << field.parameter();
	std::string modifier;
	try {
		modifier = felt2ValidTime_[ keyStr.str() ];
	}
	catch ( std::out_of_range & e ) {
		return field.validTime();
	}
	// Infinite Duration
	if ( modifier == "infinite" ) {
		return boost::posix_time::neg_infin;
	}
	else
	if ( modifier == "referencetime" ) {
		return field.referenceTime();
	}
	else {
		std::istringstream duration(modifier);
		int hour, minute, second;
		char dummy;
		duration >> hour >> dummy >> minute >> dummy >> second;
		boost::posix_time::time_duration period(hour, minute, second);
		boost::posix_time::ptime ret = field.validTime() + period;
		return ret;
	}
}

boost::posix_time::ptime FeltLoader::validTimeTo(const FeltField & field)
{
	stringstream keyStr;
	keyStr << field.parameter();
	std::string modifier;
	try {
		modifier = felt2ValidTime_[ keyStr.str() ];
	}
	catch ( std::out_of_range & e ) {
		return field.validTime();
	}
	// Infinite Duration
	if ( modifier == "infinite" ) {
		return boost::posix_time::pos_infin;
	}
	else {
		// For everything else...
		return field.validTime();
	}
}


std::string FeltLoader::valueParameterName(const FeltField & field)
{
	WDB_LOG & log = WDB_LOG::getInstance( "wdb.feltloader.valueparametername" );
	stringstream keyStr;
	keyStr << field.parameter() << ", "
		   << field.verticalCoordinate() << ", "
		   << field.level1();
	std::string ret;
	try {
		ret = felt2ValueParameter_[keyStr.str()];
	}
	catch ( std::out_of_range & e ) {
		// Check if we match on any (level1)
		stringstream akeyStr;
		akeyStr << field.parameter() << ", "
				<< field.verticalCoordinate() << ", "
				<< "any";
		log.debugStream() << "Did not find " << keyStr.str() << ". Trying to find " << akeyStr.str();
		ret = felt2ValueParameter_[akeyStr.str()];
	}
	ret = ret.substr( 0, ret.find(',') );
	boost::trim( ret );
	log.debugStream() << "Value parameter " << ret << " found.";
	return ret;
}

std::string FeltLoader::valueParameterUnit(const FeltField & field)
{
	stringstream keyStr;
	keyStr << field.parameter() << ", "
		   << field.verticalCoordinate() << ", "
		   << field.level1();
	std::string ret;
	try {
		ret = felt2ValueParameter_[keyStr.str()];
	}
	catch ( std::out_of_range & e ) {
		// Check if we match on any (level1)
		stringstream akeyStr;
		akeyStr << field.parameter() << ", "
				<< field.verticalCoordinate() << ", "
				<< "any";
		ret = felt2ValueParameter_[akeyStr.str()];
	}
	ret = ret.substr( ret.find(',') + 1 );
	boost::trim( ret );
	return ret;
}

void FeltLoader::levelValues( std::vector<wdb::load::Level> & levels, const FeltField & field )
{
	WDB_LOG & log = WDB_LOG::getInstance( "wdb.feltloader.levelValues" );
	try {
		stringstream keyStr;
		keyStr << field.verticalCoordinate() << ", "
			   << field.level1();
		std::string ret;
		try {
			ret = felt2LevelParameter_[keyStr.str()];
		}
		catch ( std::out_of_range & e ) {
			// Check if we match on any (level1)
			stringstream akeyStr;
			akeyStr << field.verticalCoordinate() << ", any";
			ret = felt2LevelParameter_[akeyStr.str()];
		}
		std::string levelParameter = ret.substr( 0, ret.find(',') );
		boost::trim( levelParameter );
		std::string levelUnit = ret.substr( ret.find(',') + 1 );
		boost::trim( levelUnit );
		float coeff = 1.0;
		float term = 0.0;
		connection_.readUnit( levelUnit, &coeff, &term );
		float lev1 = field.level1();
	    if ( ( coeff != 1.0 )&&( term != 0.0) ) {
   			lev1 =   ( ( lev1 * coeff ) + term );
	    }
	    float lev2;
	    if ( field.level2() == 0 ) {
	    	lev2 = lev1;
	    }
	    else {
			lev2 = field.level2();
		    if ( ( coeff != 1.0 )&&( term != 0.0) ) {
	   			lev2 =   ( ( lev2 * coeff ) + term );
		    }
	    }
	    wdb::load::Level baseLevel( levelParameter, lev1, lev2 );
	    levels.push_back( baseLevel );
	}
	catch ( wdb::ignore_value &e )
	{
		log.infoStream() << e.what();
	}
	// Find additional level
	try {
		stringstream keyStr;
		keyStr << field.parameter() << ", "
			   << field.verticalCoordinate() << ", "
			   << field.level1() << ", "
			   << field.level2();
		log.debugStream() << "Looking for levels matching " << keyStr.str();
		std::string ret = felt2LevelAdditions_[ keyStr.str() ];
		std::string levelParameter = ret.substr( 0, ret.find(',') );
		boost::trim( levelParameter );
		string levFrom = ret.substr( ret.find_first_of(',') + 1, ret.find_last_of(',') - (ret.find_first_of(',') + 1) );
		boost::trim( levFrom );
		string levTo = ret.substr( ret.find_last_of(',') + 1 );
		boost::trim( levTo );
		log.debugStream() << "Found levels from " << levFrom << " to " << levTo;
		float levelFrom = boost::lexical_cast<float>( levFrom );
		float levelTo = boost::lexical_cast<float>( levTo );
		wdb::load::Level level( levelParameter, levelFrom, levelTo );
		levels.push_back( level );
	}
	catch ( wdb::ignore_value &e )
	{
		log.infoStream() << e.what();
	}
	catch ( std::out_of_range &e ) {
		log.debugStream() << "No additional levels found.";
	}
	if ( levels.size() == 0 ) {
		throw wdb::ignore_value( "No valid level key values found." );
	}
}

int FeltLoader::dataVersion( const FeltField & field )
{
		return field.dataVersion();
}

int FeltLoader::confidenceCode( const FeltField & field )
{
		return 0; // Default
}

namespace
{
template<typename T>
struct value_convert : std::unary_function<felt::word, T>
{
	typedef T output_type;

	output_type scaleFactor_;
	output_type coeff_;
	output_type term_;
	output_type undefinedReplacement_;

	value_convert(output_type scaleFactor, output_type coeff, output_type term, output_type undefinedReplacement) :
		scaleFactor_(scaleFactor),
		coeff_(coeff),
		term_(term),
		undefinedReplacement_(undefinedReplacement)
	{}

	output_type operator () (felt::word w) const
	{
		if ( felt::isUndefined(w) )
			return undefinedReplacement_;

		return (((output_type(w) * scaleFactor_) * coeff_) + term_);
	}
};
}

void FeltLoader::getValues(std::vector<float> & out, const FeltField & field)
{
	std::vector<felt::word> rawData;
	field.grid(rawData);
	out.reserve(rawData.size());

	double scale = std::pow(double(10), double(field.scaleFactor()));
	float coeff = 1.0;
	float term = 0.0;
	connection_.readUnit( valueParameterUnit(field), &coeff, &term );

	WDB_LOG & log = WDB_LOG::getInstance( "wdb.feltLoad.feltLoader" );
	log.debugStream() << "Conversion: coefficient: " << coeff <<", term: " << term;
	log.debugStream() << "Example: " << float(rawData[10]) << " -> " <<
			value_convert<float>(scale, coeff, term, connection_.getUndefinedValue())(rawData[10]);

	std::transform(rawData.begin(), rawData.end(), std::back_inserter(out),
			value_convert<float>(scale, coeff, term, connection_.getUndefinedValue()));

	gridToLeftLowerHorizontal( out, field );
}

void
FeltLoader::gridToLeftLowerHorizontal(  std::vector<float> & out, const FeltField & field )
{
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.feltLoad.feltLoader" );
	FeltGridDefinitionPtr projection = field.projectionInformation();
	GridGeometry::Orientation fromMode =  projection->getScanMode();

    unsigned int nI = field.xNum();
    unsigned int nJ = field.yNum();

    if ( out.size() != nI * nJ )
    {
    	std::ostringstream err;
    	err << "Invalid grid size: " << out.size() << " (should be " << nI * nJ << ")";
    	throw std::runtime_error(err.str());
    }

    switch( fromMode )
    {
        case GridGeometry::LeftUpperHorizontal:
            log.debugStream() << "Swapping LeftUpperHorizontal to LeftLowerHorizontal";
            for ( unsigned int j = 1; j <= nJ / 2; j ++ ) {
                for ( unsigned int i = 0; i < nI; i ++ ) {
                    std::swap( out[((nJ - j) * nI) + i], out[((j - 1) * nI) + i] );
                }
            }
            break;
        case GridGeometry::LeftLowerHorizontal:
            log.debugStream() << "Grid was already in requested format";
            break;
        default:
            throw std::runtime_error( "Unsupported field conversion" );
    }
    projection->setScanMode(GridGeometry::LeftLowerHorizontal);
}

}
