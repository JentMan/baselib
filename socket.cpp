/*
 * socket.cpp
 *
 *  Created on: Apr 4, 2009
 *      Author: dee
 */

#include "socket.h"
#include "exception.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

using namespace baselib;
using namespace baselib::socket;

//streambase::streambase() {
//}
//
//void streambase::reset() {
//}

tcpstream::tcpstream() : _M_socket(-1), _M_blocking(false), _M_linger(8), _M_closeonexec(true) {
	if (isvalid())
		close();
	create();
}

tcpstream::tcpstream(const char *address, int port) :
	_M_socket(-1), _M_blocking(false), _M_linger(8), _M_closeonexec(true) {
	int result = 0;
	struct sockaddr_in addr;
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(address);
	int len = sizeof(address);

	_M_socket = ::socket(AF_INET, SOCK_STREAM, 0);
	result = ::bind(_M_socket, (struct sockaddr*)&address, len);
	if (result != 0)
		throw new exception("Bind failed.");
	config();
}

tcpstream::tcpstream(int sock) : _M_socket(sock), _M_blocking(false), _M_linger(8), _M_closeonexec(true) {
	config();
}

tcpstream::~tcpstream() {
	close();
}

void tcpstream::config() {
	if (isvalid()) {
		unsigned long nonblock = _M_blocking ? 0 : 1;
		ioctl(_M_socket, FIONBIO, &nonblock);

		struct linger l;
		l.l_onoff = _M_linger > 0 ? 1 : 0;
		l.l_linger = _M_linger > 0 ? _M_linger : 0;
		setsockopt(_M_socket, SOL_SOCKET, SO_LINGER, (const char*)(void*)&l, sizeof(struct linger));

		int reuse = 1;
		setsockopt(_M_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)(void*)&reuse, sizeof(reuse));

		int nodelay = 1;
		setsockopt(_M_socket, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

	    int val = fcntl(_M_socket, F_GETFD, 0);
		if (_M_closeonexec)
			val = val | FD_CLOEXEC;
		else
			val = !((!val) | FD_CLOEXEC);
		fcntl(_M_socket, F_SETFD, val);
	}
}

int tcpstream::listen(int backlog) {
	if (isvalid()) {
		if(::listen(_M_socket, backlog) != 0)
			return 0;
		return 1;
	}
	return 0;
}

bool tcpstream::blockforread(int seconds) {
	if ((seconds < 0) || (!isvalid()))
		return false;
    timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	fd_set fs;
	FD_ZERO(&fs);
	FD_SET(_M_socket, &fs);

	int ret = ::select(_M_socket + 1, &fs, NULL, NULL, &tv);

	if (ret == 0)
		return false;
	else if (ret < 0) {
		close();
		return false;
	}

	if (!FD_ISSET(_M_socket, &fs))
		return false;

	int error;
	unsigned int len;
	len = sizeof(error);
	if (getsockopt(_M_socket, SOL_SOCKET, SO_ERROR, &error, &len) != 0)
		return false;

	if (error)
		return false;

	return true;
}

void tcpstream::blockforwrite(int seconds) {
	if (isvalid()) {

		if (seconds <= 0) seconds = 0;

		timeval tv;
		tv.tv_sec = seconds;
		tv.tv_usec = 0;
		fd_set fs;
		FD_ZERO(&fs);
		FD_SET(_M_socket, &fs);
		int ret = ::select(_M_socket + 1, NULL, &fs, NULL, &tv);
		if (ret == 0)
			throw new exception("blockforwrite: select returned 0.");
		if (ret < 0) {
			close();
			throw new exception("blockforwrite: select returned -1.");
		}

		if (!FD_ISSET(_M_socket, &fs))
			throw new exception("FD_ISSET failed.");
		int error;
		unsigned int len;
		len = sizeof(error);
		if (getsockopt(_M_socket, SOL_SOCKET, SO_ERROR, &error, &len) != 0) {
			throw new exception("blockforwrite: getsockopt failed.");
		}
		if (error)
			throw new exception("blockforwrite: general error.");
	}
}

void tcpstream::close() {
	if (isvalid()) {
		shutdown();
		closesocket();
	}
}

void tcpstream::shutdown() {
	::shutdown(_M_socket, 1);
	::shutdown(_M_socket, 2);
}

void tcpstream::closesocket() {
	::close(_M_socket);
	_M_socket = -1;
}

bool tcpstream::accept(tcpstream *listenserver, int seconds) {

	if (!listenserver || !listenserver->isvalid())
		return false;

	if (!listenserver->blockforread(seconds))
		return false;

	sockaddr asa;
	unsigned int nlen = sizeof(asa);
	int sock = ::accept(listenserver->getsocket(), &asa, &nlen);

	if (sock != -1) {
		close();
		_M_socket = sock;
		config();

		blockforwrite(seconds);
	}
	return isvalid();
}

void tcpstream::create() {
    //close();
    _M_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    config();
    if (!isvalid())
    	throw new exception("Create failed.");
}

void tcpstream::connect(const char *addr, port_t port, int seconds) {
	if (isvalid()) {
		struct sockaddr_in sa;
		memset(&sa, 0, sizeof(sa));
		if (addr && *addr) {
			struct hostent *hp;
			if ((hp = gethostbyname(addr)) == NULL)
				throw new exception("connect: gethostbyname failed.");
			memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
				sa.sin_family = AF_INET;
		} else
			sa.sin_family = 0;
		sa.sin_port = htons((u_short) port);
		if (::connect(_M_socket, (sockaddr*) &sa, sizeof(sa)) != 0) {
			if (errno != EINPROGRESS)
				throw new exception("connect: connect failed.");
			blockforwrite(seconds);
		}
	}
}

void tcpstream::bind(const char *addr, port_t port) {
	if (isvalid()) {
		struct hostent *hp;
		struct sockaddr_in sa;
		if((hp = gethostbyname(addr)) == NULL)
			throw new exception("gethostbyname failed.");
		memset(&sa, 0, sizeof(sa));
		memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
		sa.sin_family = AF_INET;
		sa.sin_port = htons((u_short)port);
		if(::bind(_M_socket, (sockaddr*)&sa, sizeof(sa)) != 0)
			throw new exception("bind: bind failed.");
	}
}

int tcpstream::write(const char *buffer, int len) {
	if (!isvalid())
		throw new exception("Write: socket not valid.");
	int err, sent;
	while(true) {
		sent = ::send(_M_socket, buffer, len, 0);
		err = errno;
		if (sent < 0)
			if (err == EAGAIN)
				continue;
			else
				throw new exception("Write : error.");
		else
			break;
	}
	return sent;
}

int tcpstream::read(char *buffer, int len) {
	if (!isvalid())
		throw new exception("Read: socket not valid.");
	int read, err;
	while(true) {
		read = ::recv(_M_socket, buffer, len, 0);
		err = errno;
		if (read < 0)
			if (err == EAGAIN)
				continue;
			else
				throw new exception("Read : error.");
		else
			break;
	}
	return read;
}
