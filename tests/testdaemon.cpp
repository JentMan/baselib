/*
 * testdaemon.cpp
 *
 *  Created on: Apr 7, 2009
 *      Author: dee
 */

#include "../daemon.h"
#include "../logger.h"
#include "../socket.h"

using namespace baselib;

void accept_handler(socket::tcpstream& stream,
		char* shutdown, void* userdata) {
	logger::write("accept_handler");
	char *buf = new char[256];
	for(int i=0; i<256; i++)
		buf[i] = 0;
	stream.read(buf, 255);
	logger::write(buf);
	delete [] buf;
}

int main() {
	logger::configure("daemontest", exception::info, "");
	daemon::mgr m;
	daemon::tcpdaemon td(m, daemon::tcpdaemon::threaded, &accept_handler, 0, "127.0.0.1", 3128, 5);
	m.run(0);
	return 0;
}
