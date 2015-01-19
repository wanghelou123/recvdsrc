#ifndef _IOSERVICE_H_
#define _IOSERVICE_H_
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>


using namespace std;
using namespace boost;
using boost::asio::ip::tcp;

class io_service_pool
	: public boost::noncopyable
{
public:

	explicit io_service_pool(std::size_t pool_size)
		: next_io_service_(0)
	{
		for (std::size_t i = 0; i < pool_size; ++ i)
		{
			io_service_sptr io_service(new boost::asio::io_service);
			work_sptr work(new boost::asio::io_service::work(*io_service));
			io_services_.push_back(io_service);
			work_.push_back(work);
		}
	}

	void start()
	{
		for (std::size_t i = 0; i < io_services_.size(); ++ i)
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(
				boost::bind(&boost::asio::io_service::run, io_services_[i])));
			threads_.push_back(thread);
		}
	}

	void join()
	{
		for (std::size_t i = 0; i < threads_.size(); ++ i)
		{
			threads_[i]->join();
		}
		
	}

	void stop()
	{
		for (std::size_t i = 0; i < io_services_.size(); ++ i)
		{
			io_services_[i]->stop();	
		}		
	}

	boost::asio::io_service& get_io_service()
	{
		boost::mutex::scoped_lock lock(mtx);
		boost::asio::io_service& io_service = *io_services_[next_io_service_];
		++ next_io_service_;
		if (next_io_service_ == io_services_.size())
		{
			next_io_service_ = 0;
		}

		return io_service;
	}

private:
	typedef boost::shared_ptr<boost::asio::io_service> io_service_sptr;
	typedef boost::shared_ptr<boost::asio::io_service::work> work_sptr;
	typedef boost::shared_ptr<boost::thread> thread_sptr;

	boost::mutex mtx;

	std::vector<io_service_sptr> io_services_;
	std::vector<work_sptr> work_;
	std::vector<thread_sptr> threads_;
	std::size_t next_io_service_;
};

/*
int main()
{

	//boost


	io_service_pool io_service_pool_(50);
		io_service_pool_.start();
		io_service_pool_.join();


	while(true)
	{

this_thread::sleep(posix_time::seconds(2));

	}

	return 0;
}
*/
#endif