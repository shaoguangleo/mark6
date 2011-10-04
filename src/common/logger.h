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

#ifndef _LOGGER_H_
#define _LOGGER_H_

//! This module manages the single, central LOG4CXX logger instance.
//! The logger instance is used by all classes for logging.
//! \todo Remove LOG4CXX and replace with more robust solution (perhaps
//! syslog?).
//! \todo Encapsulate LOG4CXX as a singleton.
//! \todo Implement my own LOG4CXX?

// C++ includes
#include <string>

// Framework includes.
#include <log4cxx/logger.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/propertyconfigurator.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

//! The single logger instance.
extern LoggerPtr logger;

//! Initialize logger.
extern void init_logger(const std::string log_config);

#endif // _LOGGER_H_