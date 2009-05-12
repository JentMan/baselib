/*
 * socket.h
 *
 *  Created on: Apr 4, 2009
 *      Author: dee
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include <string>

namespace baselib {

namespace socket {

typedef unsigned int port_t;

class tcpstream {

public:
	tcpstream();
	tcpstream(const char *address, int port);
	tcpstream(int sock);
	bool isvalid() { return (_M_socket != -1); }
	void create();
	void bind(const char *addr, port_t port);
	void config();
	int listen(int backlog);
	bool accept(tcpstream *listenserver, int seconds);
	bool blockforread(int seconds);
	void blockforwrite(int seconds);
	int getsocket() { return _M_socket; }
	void connect(const char *addr, port_t port, int seconds);
	int write(const char *buffer, int len);
	int read(char *buffer, int len);
	void close();
	void closesocket();
	~tcpstream();
protected:
	void shutdown();
private:
	int _M_socket;
	bool _M_blocking;
	int _M_linger;
	bool _M_closeonexec;
};

}
}

#endif /* SOCKET_H_ */
