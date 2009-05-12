/*
 * daemon.h
 *
 *  Created on: 06.04.2009
 *      Author: dee
 */

#ifndef DAEMON_H_
#define DAEMON_H_

#include "socket.h"
#include <list>
#include <string>

namespace baselib {
namespace daemon {

typedef void (*accept_handler)(socket::tcpstream& stream,
		char* shutdown, void* userdata);

extern pid_t g_parentpid;
void fork(int waitsecs);

class tcpdaemon;

class mgr {
	friend class tcpdaemon;
	static void signal_exit(int sig);
	static void signal_hangup(int sig);
	static void accept_thread(void *param);
private:
	int _M_connectioncount;
	int _M_signal;
	void adddaemon(tcpdaemon* daemon);
	void remdaemon(tcpdaemon* daemon);
	static mgr *_M_instance;
public:
	typedef std::list<tcpdaemon*> daemons_t;
//	std::list<tcpdaemon*> _M_daemons;
	daemons_t _M_daemons;
	mgr();
	virtual ~mgr();

	int start();
	int process(int timeout);
	int run(int timeout = 32);
	void hangup();
	void stop();
public:
	static void run_forked(tcpdaemon& daemon);
	static void run_threaded(tcpdaemon& daemon);
	static void run(void* param);
//	static void accept_thread(void *param);
};

class tcpdaemon {
	friend class mgr;
public:
	enum type {
		forked, threaded
	};
protected:
	mgr& _M_mgr;
	socket::tcpstream _M_socket;
	unsigned long _M_threadid;

	std::string _M_addr;
	int _M_port;
	int _M_backlog;

	char _M_shutdown;

	accept_handler _M_accept_handler;
	void* _M_userdata;
	type _M_type;
protected:
	virtual socket::tcpstream* newsocket();
	virtual int listen();
	virtual int accept(socket::tcpstream* socket, int seconds);
public:
	tcpdaemon(mgr& mgr, type type, accept_handler accept_handler,
			void* userdata, const char* addr, int port, int backlog = 5);
	virtual ~tcpdaemon();
	socket::tcpstream& getsocket() { return _M_socket;	}
//	mgr& mgr() { return _M_mgr; }

	//TODO: change it to bool
	char isshutdown() { return _M_shutdown; }
	void setshutdown(const char value) { _M_shutdown = value; }
	socket::tcpstream socket() { return _M_socket; }
};
}
}

#endif /* DAEMON_H_ */
