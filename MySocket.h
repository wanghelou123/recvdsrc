
#include <boost/iostreams/stream.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "io_service_pool.cpp"
typedef asio::ip::tcp::socket* sock_pt;
extern io_service_pool io_service_pool_;

class MySocket{
public:
	MySocket(){
		boost::asio::io_service & myios(io_service_pool_.get_io_service());
		sock = (new boost::asio::ip::tcp::socket(myios));
	}
public:
	
	sock_pt sock;
	unsigned char buff_shake[22];
};