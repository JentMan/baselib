/*
 * thread.cpp
 *
 *  Created on: Apr 5, 2009
 *      Author: dee
 */

#include "thread.h"
#include <pthread.h>

namespace baselib {

namespace thread {

	struct func_param {
		thread::func func;
		void *param;
	};

	void *pthread_func(void *param);
}
}

using namespace baselib;

void *thread::pthread_func(void *param) {
	thread::func_param *fp = (thread::func_param *)param;
	fp->func(fp->param);
	delete fp;
	thread::destroy();
	return 0;
}

unsigned long thread::create(thread::func func, const void *param) {
	unsigned long id = 0;
	thread::func_param *fp = new thread::func_param;
	fp->func = func;
	fp->param = const_cast<void*>(param);

	pthread_t tid;
	if (pthread_create(&tid, NULL, pthread_func, (void *)fp) == 0)
		id = (unsigned long)tid;
	return id;
}

unsigned long thread::self() {
	return (unsigned long)pthread_self();
}

void thread::destroy() {
	pthread_exit(NULL);
}

void thread::waitfor(unsigned long threadid) {
	pthread_join(threadid, NULL);
}

///////////////////////////////////////////////////////////////////////////////

unsigned long mutex::create() {
	pthread_mutex_t* t = new pthread_mutex_t;
	if (pthread_mutex_init(t, NULL) != 0) {
		delete t;
		return 0;
	}
	//t->__m_kind = PTHREAD_MUTEX_RECURSIVE_NP;
	return (unsigned long) t;
}

void mutex::destroy(unsigned long mutex) {
	pthread_mutex_destroy((pthread_mutex_t*)mutex);
}

char mutex::own(unsigned long mutex) {
	if(pthread_mutex_lock((pthread_mutex_t*)mutex) != 0)
	    return 0;
	return 1;
}

void mutex::disown(unsigned long mutex)
{
	pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

mutex::object::object()
{
	_M_mutex = mutex::create();
}

mutex::object::~object()
{
	mutex::destroy(_M_mutex);
}

char mutex::object::lock()
{
	return mutex::own(_M_mutex);
}

void mutex::object::unlock()
{
	mutex::disown(_M_mutex);
}

mutex::locker::locker(mutex::object& mutex)
{
	_M_islocked = 0;
	_M_mutex = mutex._M_mutex;

	lock();
}

mutex::locker::~locker()
{
	unlock();
}

char mutex::locker::lock()
{
	if(_M_islocked)
		return _M_islocked;
	if(mutex::own(_M_mutex))
		_M_islocked = 1;
	return _M_islocked;
}

void mutex::locker::unlock()
{
	if(!_M_islocked)
		return;
	mutex::disown(_M_mutex);
	_M_islocked = 0;
}

///////////////////////////////////////////////////////////////////////////////

event::object::object()
{
	_M_signalled = 0;
	pthread_cond_init(&_M_cond, NULL);
}

event::object::~object()
{
	pthread_cond_destroy(&_M_cond);
}

void event::object::signal()
{
	mutex::locker lock(_M_mutex);
	_M_signalled = 1;
	pthread_cond_broadcast(&_M_cond);
}

void event::object::clear()
{
	mutex::locker lock(_M_mutex);
	_M_signalled = 0;
}

void event::object::pulse()
{
	signal();
	clear();
}

void event::object::wait()
{
	mutex::locker lock(_M_mutex);
	if(!_M_signalled)
		pthread_cond_wait(&_M_cond, (pthread_mutex_t*)(void*)_M_mutex._M_mutex);
}

char event::object::wait(int seconds)
{
	struct timespec timeout;
	timeout.tv_sec = time(NULL) + seconds;
	timeout.tv_nsec = 0;

	mutex::locker lock(_M_mutex);
	if(!_M_signalled)
		pthread_cond_timedwait(&_M_cond, (pthread_mutex_t*)(void*)_M_mutex._M_mutex, &timeout);

	return _M_signalled;
}
