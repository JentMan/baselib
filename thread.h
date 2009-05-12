/*
 * thread.h
 *
 *  Created on: Apr 5, 2009
 *      Author: dee
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>

namespace baselib {

namespace thread {
	typedef void (*func)(void *param);
	unsigned long create(thread::func func, const void *param);
	unsigned long self();
	void destroy();
	void waitfor(unsigned long threadid);
}
namespace event {
	class object;
}

namespace mutex {
	unsigned long create();
	void destroy(unsigned long mutex);
	char own(unsigned long mutex);
	void disown(unsigned long mutex);

    class object
    {
    friend class locker;
    friend class event::object;
    private:
    	unsigned long _M_mutex;
    public:
    	object();
    	virtual ~object();

    	char lock();
    	void unlock();
    };

    class locker
    {
    private:
    	char _M_islocked;
    	unsigned long _M_mutex;
    public:
    	locker(mutex::object& mutex);
    	virtual ~locker();

    	char lock();
    	void unlock();

    	char islocked(){return _M_islocked;};
    };
}
namespace event
{
  class object
  {
  private:
  	char _M_signalled;
    pthread_cond_t _M_cond;
    mutex::object _M_mutex;
  public:
  	object();
    virtual ~object();

    void signal();
    void clear();
    void pulse();

    void wait();
  	char wait(int seconds);
  };
};
}

#endif /* THREAD_H_ */
