#include <boost/iostreams/stream.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include "Gateway.h"

using namespace boost;
using namespace boost::asio;
using namespace std;


//typedef boost::object_pool<class Gateway> gateway_object_pool_type;
//typedef class Gateway*	gateway_ptr;

class Server
{
private:
	//io_service &ios;
	ip::tcp::acceptor acceptor;
	queue_type& m_queue;
	
	int gateway_count;

	//gateway_object_pool_type gateway_m_msg_pool;


public:

	Server(io_service& io,queue_type& q, int port );

	~Server();

	void start();
	
	void accept_handler(const boost::system::error_code& ec, Gateway* p_gateway);	
};
