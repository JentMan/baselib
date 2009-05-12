/*
 * logger.h
 *
 *  Created on: 06.04.2009
 *      Author: dee
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <fstream>
#include "exception.h"

namespace baselib {

class exception;

	class logger {
	private:
		static logger *_M_instance;
		const char *_M_ident;
		logger();
	protected:
		std::ofstream _M_stream;
		exception::severity _M_severity;
		std::string time();
		std::string prefix();
		void setfilename(const char *);
		void setident(const char *);
		void setseverity(exception::severity);
	public:
		static void configure(const char *ident = "",
				exception::severity lowest = exception::info,
				const char* filename = 0);
		static void write(const char *message, exception::severity sev = exception::info);
		static void write(exception &exception);
	};
}

#endif /* LOGGER_H_ */
