/*
 * Created by David Lapsley on Mon Jun 6 2011.
 *
 * Copyright 2011 MIT Haystack Observatory 
 *  
 * This file is part of mark6.
 *
 * mark6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mark6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mark6.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

// C includes.
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// C++ includes.
#include <iostream>
#include <iomanip>
#include <sstream>
#include <list>
#include <algorithm>

// Boost includes.
#include <boost/foreach.hpp>

//Local includes.
#include <mark6.h>
#include <logger.h>
#include <file_writer.h>
#include <test_file_writer.h>

using namespace boost;

CPPUNIT_TEST_SUITE_REGISTRATION (TestFileWriter);

void
TestFileWriter :: setUp (void)
{
  // set up test environment (initializing objects)
}

void
TestFileWriter :: tearDown (void)
{
}

void
TestFileWriter::basic(void)
{
  std::cout << "TestFileWriter::basic()" << std::endl;
  const int id = 0;
  const std::string capture_file("/tmp/test_file_writer-basic-1.dat");
  const std::string file_name("/tmp/test1.m6");
  const boost::uint32_t write_block_size(1048576);
  const boost::uint32_t write_blocks(512);
  const boost::uint32_t poll_timeout = 1000; // ms
  const double command_interval = 1; //s

  FileWriter fw(id, write_block_size, write_blocks, capture_file,
		poll_timeout, 0, command_interval);

  fw.open();
  fw.start();

  LOG4CXX_DEBUG(logger, "Started file writer.");
    
  const int NUM_BLOCKS = 100;
  for (boost::uint32_t i=0; i<write_block_size; ++i)
    buf[i] = static_cast<uint8_t>(i);

  for (int i=0; i<NUM_BLOCKS; ++i) {
    fw.write(buf);
  }

  fw.cmd_write_to_disk();

  sleep(10);

  fw.cmd_stop();
  fw.join();

  fw.close();

  LOG4CXX_DEBUG(logger, "Joined file writer.");
}