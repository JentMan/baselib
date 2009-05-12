/*
 * daemon.cpp
 *
 *  Created on: 06.04.2009
 *      Author: dee
 */

#include "daemon.h"
#include "exception.h"
#include "thread.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>

namespace baselib {

namespace daemon {
	mgr *mgr::_M_instance = 0;
	mutex::object g_mgr_mutex;
	pid_t g_parentpid;
}
}

using namespace baselib;
using namespace baselib::daemon;

void signal_success_exit(int sig) {
	_exit(0);
}

void signal_failure_exit(int sig) {
	_exit(sig);
}

void mgr::signal_exit(int sig) {
	mutex::locker lock(g_mgr_mutex);
	if (!lock.islocked())
		return;
	if (_M_instance != NULL) {
		if (!_M_instance->_M_signal)
			_M_instance->_M_signal = sig;
		_M_instance->stop();
	}
}

void mgr::signal_hangup(int sig) {
	mutex::locker lock(g_mgr_mutex);
	if (!lock.islocked())
		return;
	if (_M_instance != NULL) {
		if (!_M_instance->_M_signal)
			_M_instance->_M_signal = sig;
		_M_instance->hangup();
	}
}

void killparent() {
	if (g_parentpid)
		kill(g_parentpid, SIGUSR1);
}

void mgr::run_forked(tcpdaemon& daemon) {
	socket::tcpstream* stream = NULL;
	while (!daemon.isshutdown()) {
		if (daemon.socket().blockforread(64) != 1)
			continue;
		if (!stream)
			stream = daemon.newsocket();
		if (daemon.accept(stream, 8)) {
			// server->_M_closeonexec = 0;
			// server->config();

			// ***** double-forking to eliminate zombies
			pid_t pid = ::fork();
			if (!pid) { // in child
				daemon._M_socket.close();
				pid = ::fork();
				if (!pid) { // in child again
					daemon._M_accept_handler(*stream, NULL, daemon._M_userdata);
					stream->close();
				}
				//    				else
				//    				  stream->closeonly();

				delete stream;
				exit(0);
			} else {
				stream = NULL;
				::waitpid(pid, NULL, 0);
			}
		}
	}
	daemon._M_socket.close();
}

struct thread_info {
	socket::tcpstream* _M_stream;
	tcpdaemon* _M_daemon;
	accept_handler _M_accept_handler;
};

void mgr::accept_thread(void *param) {
	thread_info* info = (thread_info*) param;
	if (info) {
		info->_M_daemon->_M_mgr._M_connectioncount++;
		if (info->_M_accept_handler)
			info->_M_accept_handler(*info->_M_stream,
					&info->_M_daemon->_M_shutdown, info->_M_daemon->_M_userdata);
		info->_M_stream->close();
		delete info->_M_stream;
		info->_M_daemon->_M_mgr._M_connectioncount--;
		delete info;
	}
	thread::destroy();
}

void mgr::run_threaded(tcpdaemon& daemon) {
	socket::tcpstream* stream = NULL;
	while (!daemon._M_shutdown) {
		if (!stream)
			stream = daemon.newsocket();

		if (daemon.accept(stream, 64)) {
			thread_info* info = new thread_info;
			info->_M_accept_handler = daemon._M_accept_handler;
			info->_M_stream = stream;
			info->_M_daemon = &daemon;
			stream = NULL;
			thread::create(accept_thread, info);
		}
	}
	daemon.socket().close();
}

void mgr::run(void* param) {
	if (param) {
		tcpdaemon* daemon = (tcpdaemon*) param;
		daemon->_M_threadid = thread::self();
		if (daemon->_M_type == daemon::tcpdaemon::forked)
			run_forked(*daemon);
		else if (daemon->_M_type == daemon::tcpdaemon::threaded)
			run_threaded(*daemon);
		daemon->_M_threadid = 0;
	}
	thread::destroy();
}

///////////////////////////////////////////////////////////////////////////////

void daemon::fork(int waitsecs) {
	int null = open("/dev/null", O_RDWR);
	dup2(null, 0);
	dup2(null, 1);
	dup2(null, 2);
	close(null);

	signal(SIGUSR1, signal_success_exit);
	signal(SIGUSR2, signal_failure_exit);

	g_parentpid = getpid();

	pid_t cpid = ::fork();
	if (!cpid) // do if child
	{
		atexit(killparent);
		return;
	}
	g_parentpid = 0;
	sleep(waitsecs); // sleep while waiting for signal to exit...if no signal comes, exit with default error
	_exit(-1);
}

daemon::mgr::mgr() {
	_M_signal = 0;
	_M_connectioncount = 0;
	mutex::locker lock(g_mgr_mutex);
	if (lock.islocked() && _M_instance == NULL)
		_M_instance = this;
}

daemon::mgr::~mgr() {
	mutex::locker lock(g_mgr_mutex);
	if (lock.islocked() && _M_instance == this) {

		_M_instance = NULL;
		//TODO: does std::list own elements ?
		for(daemons_t::iterator it = _M_daemons.begin();
			it != _M_daemons.end(); it++) {
			tcpdaemon *pd = (*it);
			if (pd) delete pd;
		}
	}
	if (g_parentpid)
		kill(g_parentpid, SIGUSR1);
}

void daemon::mgr::adddaemon(daemon::tcpdaemon* newdaemon) {
	if(!newdaemon) return;
	mutex::locker mgrlock(g_mgr_mutex);
	if(!mgrlock.islocked()) return;

	for(daemons_t::iterator it = _M_daemons.begin();
		it != _M_daemons.end(); it++) {
		tcpdaemon *pd = (*it);
		if (newdaemon == *it) return;
	}
	_M_daemons.push_back(newdaemon);
}

void daemon::mgr::remdaemon(daemon::tcpdaemon* olddaemon) {
	if(!olddaemon) return;
	mutex::locker mgrlock(g_mgr_mutex);
	if(!mgrlock.islocked()) return;

//	int index, count = _M_daemons.getcount();
//	for(index = 0;index < count;index++) {
//		daemon::tcpdaemon* daemon = (daemon::tcpdaemon*)m_daemons.getitemat(index);
//		if(daemon == olddaemon) {
//			_M_daemons.removeitemat(index);
//			return;
//		}
//	}
	_M_daemons.remove(olddaemon);
}

int daemon::mgr::start() {
	time_t timestart = time(NULL);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGABRT, signal_exit);
	signal(SIGINT, signal_exit);
	signal(SIGTERM, signal_exit);
	signal(SIGQUIT, signal_exit);
	signal(SIGHUP, signal_hangup);

	mutex::locker mgrlock(g_mgr_mutex);
	if (!mgrlock.islocked())
		return -400;

//	int index, count = _M_daemons.getcount();
//	if (count <= 0)
//		throw new exception(exception::failure, "there no daemons");
	if(_M_daemons.empty())
		throw new exception(exception::error, "there no daemons");

//	for (index = 0; index < count; index++) {
//		daemon::tcpdaemon* daemon = (daemon::tcpdaemon*) _M_daemons.getitemat(index);
//		if (!daemon)
//			continue;
//		int ret = daemon->listen();
//		if (ret)
//			return ret;
//	}
	for(daemons_t::iterator it = _M_daemons.begin();
		it != _M_daemons.end(); it++) {
		tcpdaemon *pd = (*it);
		if (!pd) continue;
		int rc = pd->listen();
		if (rc)
			return rc;
	}
	return 0;
}

int daemon::mgr::process(int timeout) {
	mutex::locker mgrlock(g_mgr_mutex);
	if (!mgrlock.islocked())
		return -400;

	//list::dword threadlist;
	std::list<unsigned long> threadlist;
	time_t timestart = time(NULL);

//	int index, count = _M_daemons.getcount();
//	for (index = 0; index < count; index++) {
//		tcpdaemon* daemon = (tcpdaemon*) m_daemons.getitemat(index);
//		if (!daemon)
//			continue;
//		unsigned long threadid = thread::create(daemon::run, daemon);
//		threadlist.additem(threadid);
//	}
	for(daemons_t::iterator it = _M_daemons.begin();
		it != _M_daemons.end(); it++) {
		tcpdaemon *pd = (*it);
		if (pd) {
			unsigned long threadid =  thread::create(run, pd);
			threadlist.push_back(threadid);
		}
	}
//	tcpdaemon* daemon = count > 0 ? (tcpdaemon*) m_daemons.getitemat(0) : NULL;
	mgrlock.unlock();
	if (g_parentpid)
		kill(g_parentpid, SIGUSR1);
	while (!threadlist.empty()) {
		unsigned long threadid = *threadlist.begin();
		thread::waitfor(threadid);
		threadlist.remove(threadid);
	}
	return _M_signal;
}

int daemon::mgr::run(int timeout) {
	try {
		_M_signal = 0;
		time_t timestart = time(NULL);
		int ret = start();
		if (ret) {
			if (g_parentpid)
				kill(g_parentpid, SIGUSR2);
			return ret;
		}
		return process(timeout);
	} catch (exception* exception) {
		if (g_parentpid)
			kill(g_parentpid, SIGUSR2);
		throw;
	}
}

///
/// Closes an each daemon's socket and sets shutdown flag on.
///
void daemon::mgr::hangup() {
	mutex::locker mgrlock(g_mgr_mutex);
	if (!mgrlock.islocked())
		return;
	for(daemons_t::iterator it = _M_daemons.begin();
		it != _M_daemons.end(); it++) {
		tcpdaemon *pd = (*it);
		if ((!pd) || (pd->_M_threadid != thread::self()))
			continue;
		//pd->_M_socket.signalclose();
		pd->_M_socket.close();
		pd->_M_shutdown = 1;
	}
}

///
/// Sends a SIGHUP to each thread.
///
void daemon::mgr::stop() {
	mutex::locker mgrlock(g_mgr_mutex);
	if (!mgrlock.islocked())
		return;

	for(daemons_t::iterator it = _M_daemons.begin();
		it != _M_daemons.end(); it++) {
		tcpdaemon *pd = (*it);
		if (pd && pd->_M_threadid)
			pthread_kill(pd->_M_threadid, SIGHUP);
	}
}

///////////////////////////////////////////////////////////////////////////////

daemon::tcpdaemon::tcpdaemon(daemon::mgr& mgr, type t,
		daemon::accept_handler accept_handler, void* userdata,
		const char* addr, int port, int backlog) :
	_M_mgr(mgr) {
	_M_threadid = 0;
	_M_type = t;
	_M_accept_handler = accept_handler;
	_M_userdata = userdata;
	_M_addr = addr;
	_M_port = port;
	_M_backlog = backlog;
	_M_shutdown = 0;
	_M_mgr.adddaemon(this);
}

daemon::tcpdaemon::~tcpdaemon() {
	_M_mgr.remdaemon(this);
}

socket::tcpstream* daemon::tcpdaemon::newsocket() {
	return new socket::tcpstream();
}

int daemon::tcpdaemon::listen() {
	if (!_M_accept_handler)
		return -100;
	//_M_socket._M_linger = 8;	// this is value by default
	try {
		_M_socket.create();
		_M_socket.bind(_M_addr.c_str(), _M_port);
		_M_socket.listen(_M_backlog);
	} catch(exception &e) {
		return -1;
		throw;
	}
	return 0;
}

int daemon::tcpdaemon::accept(socket::tcpstream* socket, int seconds) {
	return socket->accept(&_M_socket, seconds);
}
