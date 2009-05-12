/*
 * testsocket.cpp
 *
 *  Created on: Apr 5, 2009
 *      Author: dee
 */

#include "socket.h"
#include "exception.h"
#include <iostream>
#include <string.h>

using namespace baselib;

int main() {
	socket::tcpstream s;
//	s._M_blocking = true;
	char *buf = new char[256];
	int len = 0;
	try {
		s.connect("127.0.0.1", 80, 30);
		const char *test = "get index.htm\n\n";
		s.write(test, strlen(test));
	}
	catch(exception &e) {
		std::cout << e.getmessage() << std::endl;
		delete [] buf;
		throw;
	}
}
