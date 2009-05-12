/*
 * logger.cpp
 *
 *  Created on: 06.04.2009
 *      Author: dee
 */

#include "logger.h"
#include "thread.h"
#include <ios>
#include <fstream>
#include <stdarg.h>
#include <sstream>
#include <iostream>

namespace baselib {
	logger *logger::_M_instance = 0;
	mutex::object logmutex;
}

using namespace baselib;

std::string logger::time() {
	time_t now = ::time(NULL);
	char strtime[32];
	strftime(strtime, sizeof(strtime), "%b %d %H:%M:%S ", localtime(&now));
	return std::string(&strtime[0]);
}

std::string logger::prefix() {
	std::ostringstream buf;
	buf << this->time() << _M_ident << "[" << getpid() << "]: ";
	return buf.str();
}

logger::logger() : _M_ident("") {}

void logger::configure(const char *ident, exception::severity sev,
				const char* filename) {
	mutex::locker lock(logmutex);
	if (!lock.islocked()) return;
	if (!_M_instance)
		_M_instance = new logger();

	if (ident) _M_instance->_M_ident = ident;
	if (sev != _M_instance->_M_severity)
		_M_instance->_M_severity = sev;

	if (filename) {
		if (_M_instance->_M_stream.is_open())
			_M_instance->_M_stream.close();
		_M_instance->_M_stream.open(filename, std::ios_base::out | std::ios_base::app);
		if (!_M_instance->_M_stream)
			throw new exception("Logger: failed to open.");
	}
}

void logger::write(const char *text, exception::severity sev) {
	std::ostringstream buf;
	mutex::locker lock(logmutex);
	if (!lock.islocked())
		return;
	if (!_M_instance)
		return;
	buf << _M_instance->prefix() << text << std::endl;
	if (sev >= _M_instance->_M_severity) {
		if (!_M_instance->_M_stream.bad() &&
			_M_instance->_M_stream.is_open()) {
			_M_instance->_M_stream << buf.str() << std::endl;
			_M_instance->_M_stream.flush();
		}
		// to stderr
		std::cerr << buf.str() << std::endl;
	}
}

void logger::write(exception& exception) {
	write(exception.getmessage().c_str(), exception.getseverity());
}

