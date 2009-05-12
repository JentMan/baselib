/*
 * testlogger.cpp
 *
 *  Created on: 06.04.2009
 *      Author: dee
 */

#include "../logger.h"
#include <iostream>

using namespace baselib;
using namespace baselib::log;

int main(int argc, char **argv) {
	try {
		logger::instance()->setfilename("/tmp/test1.log");
		logger::instance()->setident("test1");
		logger::instance()->setseverity(exception::info);
		logger::instance()->write("Test");
	} catch(exception *e) {
		std::cout << e->getmessage() << std::endl;
	}
	return 0;
}
