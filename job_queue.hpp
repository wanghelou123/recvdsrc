#ifndef JOB_QUEUE_HPP
#define JOB_QUEUE_HPP
#include <queue>					//标准容器
#include <boost/thread/mutex.hpp>	//boost基本工具
#include <boost/thread/condition_variable.hpp>	
template<typename Data>
class job_queue
{
public:
	typedef Data job_type;
private:
    std::queue<job_type> the_queue;
	//在C++中，mutable也是为了突破const的限制而设置的。
	//被mutable修饰的变量，将永远处于可变的状态，即使在一个const函数中。
    mutable boost::mutex the_mutex;//can be changed by a const function
    boost::condition_variable the_condition_variable;
public:
    void push(Data const& data)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        the_queue.push(data);
		lock.unlock();
        the_condition_variable.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_queue.empty();
    }

    bool try_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        if(the_queue.empty())
        {
            return false;
        }
        
        popped_value=the_queue.front();
        the_queue.pop();
		lock.unlock();
        return true;
    }

    void wait_and_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        while(the_queue.empty())
        {
            the_condition_variable.wait(lock);
        }
        
        popped_value=the_queue.front();
        the_queue.pop();	
		lock.unlock();
    }
	
	unsigned int size()
	 {
		boost::mutex::scoped_lock lock(the_mutex);
		return the_queue.size();
	 }

};


#endif 
