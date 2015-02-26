/*
 wdb

 Copyright (C) 2007-2009 met.no

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
#include "FeltLoadConfiguration.h"
#include "FeltFile.h"
#include "FeltLoader.h"
#include <wdb/errors.h>
#include <wdb/LoaderDatabaseConnection.h>
#include <wdb/LoaderConfiguration.h>
#include <wdbLogHandler.h>
#include <boost/filesystem/path.hpp>
#include <iostream>
#include <map>
#include <vector>

using namespace std;
using namespace wdb::load;

// Support Functions
namespace
{

/**
 * Write the program version to stream
 * @param	out		Stream to write to
 */
void version( ostream & out )
{
    out << PACKAGE_STRING << endl;
}

/**
 * Write help information to stram
 * @param	options		Description of the program options
 * @param	out			Stream to write to
 */
void help( const boost::program_options::options_description & options, ostream & out )
{
	version( out );
	out << '\n';
    out << "Usage: "PACKAGE_NAME" [OPTIONS] FILES...\n\n";
    out << "Options:\n";
    out << options << endl;
}
} // namespace


int main(int argc, char ** argv)
{
	FeltLoadConfiguration conf;
	try
    {
    	conf.parse( argc, argv );
	} catch (wdb::load::LoadError & e)
	{
		std::clog << e.what() << std::endl;
		return wdb::load::exitStatus();
	}
	if ( conf.general().help )
	{
		help( conf.shownOptions(), cout );
		return 0;
	}
	if ( conf.general().version )
	{
		version( cout );
		return 0;
	}

	wdb::WdbLogHandler logHandler( conf.logging().loglevel, conf.logging().logfile );
    WDB_LOG & log = WDB_LOG::getInstance( "wdb.feltload.main" );
    log.debug( "Starting feltLoad" );

    // Get list of files
    const std::vector<std::string> & file = conf.input().file;
	std::vector<boost::filesystem::path> files;
	copy( file.begin(), file.end(), back_inserter(files) );

	try
	{
		// List contents of files
		if ( conf.output().list )
		{
			for ( std::vector<boost::filesystem::path>::const_iterator it = files.begin(); it != files.end(); ++ it )
			{
				std::cout << it->string() << std::endl;
				felt::FeltFile f(*it);
				std::cout << "Size:\t" << f.size() << std::endl;
				for ( felt::FeltFile::const_iterator fit = f.begin(); fit != f.end(); ++ fit )
				{
					try
					{
						std::cout << (*fit)->information() << std::endl;
					}
					catch( std::exception & e )
					{
						std::cout << "ERROR: Unable to read file " << it->string() << "\n" << std::endl;
					}
				}
			}
		}
		// Write Files into Database
		else
		{
			wdb::load::LoaderDatabaseConnection dbConnection( conf );
			felt::FeltLoader loader( dbConnection, conf.loading(), conf.metadata(), logHandler );
			for ( std::vector<boost::filesystem::path>::const_iterator it = files.begin(); it != files.end(); ++ it )
			{
				try
				{
					felt::FeltFile feltFile(* it);
					loader.load(feltFile);
				}
				catch ( felt::FeltFile::ReadError & e )
				{
					wdb::load::registerError(wdb::load::ErrorWhenReadingFile);
					log.errorStream() << wdb::load::getErrorMessage();
				}
				catch ( std::exception & e )
				{
					wdb::load::registerError(wdb::load::UnknownError);
					log.errorStream() << "Unable to load file " << it->string();
					log.errorStream() << "Reason: " << e.what();
				}
			}
		}
	}
	catch ( wdb::load::LoadError & e )
	{
		WDB_LOG & log = WDB_LOG::getInstance("wdb.load.felt");
		log.fatal(e.what());
		return wdb::load::exitStatus();
	}
	catch ( std::exception & e )
	{
		log.fatalStream() << e.what();
		return int(wdb::load::UnknownError);
	}

	return wdb::load::exitStatus();

}
