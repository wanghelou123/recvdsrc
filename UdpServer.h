#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "WriteCmd.h"
#include "AlterConf.h"
#include "Gateway.h"


using boost::asio::ip::udp;
using namespace std;
typedef asio::deadline_timer* timer_pt;


class UdpServer
{
public:
	UdpServer(boost::asio::io_service& io_service, short port, queue_type& m_queue );	
	~UdpServer();

	void handle_receive_from(const boost::system::error_code& error,
		size_t bytes_recvd);
	
	void handle_send_to(const boost::system::error_code& /*error*/,
		size_t /*bytes_sent*/);
	//配置节点入网
	void ConfNodeToNet(const boost::system::error_code& ec, timer_pt async_timer);
	//从节点列表中删除节点
	void DelNodeFromNet(const boost::system::error_code& ec, timer_pt async_timer);
private:
	unsigned int crc_get(unsigned char *data_point, unsigned int data_length);
	unsigned char crc_check(unsigned char *frame_buff,unsigned int recv_num);
private:
	boost::asio::io_service& io_service_;
	udp::socket socket_;
	udp::endpoint sender_endpoint_;
	enum { max_length = 1024 };
	unsigned char data_[max_length];
	timer_pt async_timer;
	int sn;
	Gateway * p_gateway;

private:
	WriteCmd * p_write_cmd;
	AlterConf * p_alter_conf;
	queue_type& m_queue;
};

