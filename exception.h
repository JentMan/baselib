/*
 * exception.h
 *
 *  Created on: 06.04.2009
 *      Author: dee
 */

#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <string>

namespace baselib {

class exception {
	std::string _M_msg;
public:
	enum severity {
		debug, info, warning, error, critical
	};
	exception(const std::string& message) : _M_severity(error) {
		_M_msg = message;
	}
	exception(severity sev, const std::string& message) : _M_severity(sev) {
		_M_msg = message;
	}
	std::string& getmessage() { return _M_msg; }
	severity getseverity() { return _M_severity; }
private:
	severity _M_severity;
};
}

#endif /* EXCEPTION_H_ */
