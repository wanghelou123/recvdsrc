#ifndef WORK_HPP
#define WORK_HPP
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/timer.hpp>
#include <boost/asio.hpp>
#include "Log.h"

template<typename Queue>
class worker
{
	public:
		typedef Queue queue_type;
		typedef typename Queue::job_type job_type;
		typedef boost::function<bool(job_type&, GatewayDB&)> func_type;
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
			GatewayDB db;
			int record_num;
			time_t record_time,cur_time;
			double  i_sec;

			for(;;)
			{
				job_type job;
				record_num = m_queue.size()>10000?10000:m_queue.size();//一次事务最多提交10000条记录
				cout << "record_num=>"<< record_num<<endl;
				if(record_num == 0) {
					::sleep(2);
					continue;
				}

				if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
					FATAL("connect database failed!");
					//如果失败则存到sqlte中 
				}

				time(&record_time);

				//db.ExecTransaction("BEGIN");
				//db.ExecTransaction("begin transaction");

				while(m_queue.try_pop(job) && record_num-- > 0  ) {
					if(!m_func(job, db)) {
						//存到sqlite中
						cout << "m_func error"<<endl;
					}
				}
				//if(!m_func || !m_func(job, db)) break;
				delete job;

				//db.ExecTransaction("END");
				///db.ExecTransaction("commit");
				time(&cur_time);
				i_sec = difftime( cur_time, record_time);
				cout << "Use "<< i_sec<< " Seconds."<<endl;
				db.DisConnectDB();
			}

		}

};

#endif
