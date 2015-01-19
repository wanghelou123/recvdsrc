#ifndef WORK_HPP
#define WORK_HPP
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/timer.hpp>
#include <boost/asio.hpp>
#include "debug.h"

template<typename Queue>
class worker
{
public:
	typedef Queue queue_type;
	typedef typename Queue::job_type job_type;
	typedef boost::function<bool(job_type&)> func_type;
private:
	queue_type& m_queue;
	func_type m_func;
	int m_thread_num;
	boost::thread_group m_threads;
public:
	template<typename Func>
	worker(queue_type& q,Func func,int n=1):m_queue(q),m_func(func),m_thread_num(n){;}
	worker(queue_type& q,int n=50):m_queue(q),m_thread_num(n){;}

	void start()
	{
		for(int i=0;i<m_thread_num;i++)
		{
			m_threads.create_thread(boost::bind(&worker::do_work,this));
		}
	
	}

	template<typename Func>
	void start(Func func)
	{
		m_func=func;
		start();
	}

	void run()
	{
		cout<<"hello run"<<endl;
		start();
		m_threads.join_all();
	}

	void stop()
	{
		m_func=0;
	}

private:
	void do_work()
	{
		timer t;
		unsigned count=0;
		for(;;)
		{
		    job_type job;
			m_queue.wait_and_pop(job);
			if(!m_func || !m_func(job)) break;
			
#ifdef DEBUG
			count++;
			if(count % 128 == 0){
				printf("operate 2 gateway  use %f s\n", t.elapsed());
				cout << "queue size is: "<< m_queue.size()<<endl;
				t.restart();
				count=0;
			}
#endif
			delete job;
		}
	}

};

#endif