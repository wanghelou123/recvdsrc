#include <list>
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "UdpServer.h"
#include "pgdb.h"
#include "Log.h"
#include "io_service_pool.cpp"

using boost::asio::ip::udp;
using namespace std;

extern list<class Gateway* > gateway_object_list;
extern list<struct gateway_conf> gateway_conf_list;
extern io_service_pool io_service_pool_;

	UdpServer::UdpServer(boost::asio::io_service& io_service, short port, queue_type& q)
: io_service_(io_service),m_queue(q),
	socket_(io_service, udp::endpoint(udp::v4(), port)),sn(0),p_gateway(NULL)
{
	DEBUG("UdpServer start, port is:"<< port);
	memset(data_, '\0', sizeof(data_));
	socket_.async_receive_from(
			boost::asio::buffer(data_, max_length), remote_endpoint_,
			boost::bind(&UdpServer::handle_receive_from, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

	p_alter_conf = new AlterConf(io_service_pool_.get_io_service());
	p_write_cmd = new WriteCmd( io_service_pool_.get_io_service() );
	async_timer = new asio::deadline_timer( io_service, posix_time::seconds(1) );

}

void UdpServer::handle_send_to(const boost::system::error_code& ec,
		size_t bytes_send)
{
	if (ec)
	{
		FATAL(__func__<<":"<<boost::system::system_error(ec).what());
	}

	DEBUG("udp send "<< bytes_send<< " bytes");

}

void UdpServer::handle_receive_from(const boost::system::error_code& ec,
		size_t bytes_sent)
{
	
	if (ec) {
		FATAL(__func__<<":"<<boost::system::system_error(ec).what());
		socket_.async_receive_from(
			boost::asio::buffer(data_, max_length), remote_endpoint_,
			boost::bind(&UdpServer::handle_receive_from, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
		
	}
	DEBUG(bytes_sent << " bytes received ");

	GatewayDB db;

	if(crc_check(data_, bytes_sent)==0){
		FATAL(__func__<<"CRC CHECK ERROR!");
		return;
	}

	data_[bytes_sent-2]='\0';
	switch (data_[0]){
		case 0x01://收到通知：write_table中有命令需要下发	
			switch(data_[1]){
				case 0x01:
					NOTICE("write_table has write cmd\n");
					p_write_cmd->GetCmdFromDB();
					NOTICE("execute complete the write cmd\n");
					break;

				case 0x02:
					cout << "correction time" << endl;
					NOTICE("correction time cmd");
					p_write_cmd->correctTime();
					break;
			}
			break;

		case 0x02:
			switch(data_[1]){
				/*新增加网关*/
				case 0x01:{
							  char *p_gateway_logo = ( char *)&data_[2];
							  p_alter_conf->AddGateway(p_gateway_logo , m_queue);
							  break;
						  }
						  /*修改网关*/
				case 0x02:{
							  char *p_gateway_logo = ( char *)&data_[2];
							  p_alter_conf->ModiGateway(p_gateway_logo);
							  break;
						  }


						  /*删除网关*/
				case 0x03:{
							  char *p_gateway_logo = ( char *)&data_[2];
							  p_alter_conf->DelGateway(p_gateway_logo);
							  break;
						  }
			}
			break;	

			/*获得软件内部信息*/
		case 0x03:
			int gateway_conf_num = gateway_conf_list.size();
			int gateway_object_num = gateway_object_list.size();
			int queue_record_num = m_queue.size();
			//NOTICE("conf num:"<<gateway_conf_num);
			//NOTICE("object num:"<<gateway_object_num);
			//NOTICE("record num:"<< queue_record_num);
			int len = 4+4+4+(gateway_object_num)*40; 
			char *p = (char *)malloc(len);
			if(p == NULL){
				FATAL("malloc failed!");
				break;
			}
			memset(p, 0, len);
			char *pserial_no_buf=p+12;
			memcpy(p, &gateway_conf_num, 4);
			memcpy(p+4, &gateway_object_num, 4);
			memcpy(p+8, &queue_record_num, 4);
			list<class  Gateway* >::iterator theIterator;
			for( theIterator = gateway_object_list.begin(); \
				theIterator != gateway_object_list.end()&& gateway_object_num-->0; 
				(theIterator)++ ) {
				strcat(pserial_no_buf, (*theIterator)->gateway_id);
				strcat(pserial_no_buf, ":");
				strcat(pserial_no_buf, ((*theIterator)->sockfd->remote_endpoint().address().to_string().c_str()));
				strcat(pserial_no_buf, "-");
			}
			//发送信息给另一端
			socket_.async_send_to(
				boost::asio::buffer(p, 12+strlen(pserial_no_buf)), remote_endpoint_,
				boost::bind(&UdpServer::handle_send_to, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));


			//NOTICE(pserial_no_buf);
			free(p);
			break;

	}

	memset(data_, '\0', sizeof(data_));
	socket_.async_receive_from(
		boost::asio::buffer(data_, max_length), remote_endpoint_,
		boost::bind(&UdpServer::handle_receive_from, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

UdpServer::~UdpServer()
{
	DEBUG(__func__);
}


unsigned int UdpServer::crc_get(unsigned char *data_point, unsigned int data_length)
{
	unsigned int crc_register,temp_data,i,j;
	crc_register=0xffff;
	for(i=0;i<data_length;i++){
		crc_register^=*data_point;
		for(j=0;j<8;j++){ temp_data=crc_register&0x0001; crc_register>>=1; if(temp_data) crc_register^=0xa001;}
		data_point++;
	}   
	return(crc_register);
}


unsigned char UdpServer::crc_check(unsigned char *frame_buff,unsigned int recv_num)
{
	unsigned int crc,org;
	org =  *(frame_buff+recv_num-1);
	org <<= 8;
	org +=  *(frame_buff+recv_num-2);
	crc=crc_get(frame_buff,recv_num-2);
	if (org==0x5555 ) return 1;
	if (crc==org) return 1; else return 0;
}
