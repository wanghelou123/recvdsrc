#include "Server.h"
#include "io_service_pool.cpp"
#include "debug.h"
#include <fstream>

extern io_service_pool io_service_pool_;

Server::Server(boost::asio::io_service &io, queue_type& q, int port )
:acceptor(io, ip::tcp::endpoint(ip::tcp::v4(), port)),m_queue(q)  
{ 
#ifdef DEBUG
cout << __FILE__<< "	" << __FUNCTION__ <<endl;
cout <<"Tcp Server start, port is: " << port << "."<<endl;
#endif
	gateway_count=0;
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

		gateway_count++;
		std::ofstream fout("count.txt");
		fout <<gateway_count<<std::endl; // fout用法和cout一致, 不过是写到文件里面去


		if(ec)  
		{
				#ifdef DEBUG
				cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<"   accept_handler error." \
					<< boost::system::system_error(ec).what() <<endl;
				#endif				
				return;  
		}		
}