#include <fstream>
#include "Server.h"
#include "io_service_pool.cpp"
#include "Log.h"

extern io_service_pool io_service_pool_;

	Server::Server(boost::asio::io_service &io, queue_type& q, int port )
:acceptor(io, ip::tcp::endpoint(ip::tcp::v4(), port)),m_queue(q)  
{ 
	DEBUG("Tcp Server start, port is:"<< port);
	start(); 
}  

Server::~Server()
{ }

void Server::start()
{
	//gateway_ptr p_gateway = gateway_m_msg_pool.construct(io_service_pool_.get_io_service(), m_queue);
	Gateway *p_gateway=new Gateway(io_service_pool_.get_io_service(), m_queue);		
	acceptor.async_accept(*p_gateway->sockfd, bind(&Server::accept_handler, this, placeholders::error, p_gateway));	
}

void Server::accept_handler(const boost::system::error_code& ec, Gateway* p_gateway)
{

	start();
	p_gateway->run_server();

	if(ec)  
	{
		WARNING(__func__<< ":"<< boost::system::system_error(ec).what());
		return;  
	}		
}
