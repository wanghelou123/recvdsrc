#ifndef GATEWAY_H
#define GATEWAY_H

#include <boost/iostreams/stream.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/array.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/checked_delete.hpp>
#include <boost/enable_shared_from_this.hpp>//从this 指针创建shared_ptr
#include "pgdb.h"
#include "IniFile.h"
#include "job_queue.hpp"

//#define LIGHTSYS//上海照明灯系统

using namespace std;
using namespace boost;

typedef asio::ip::tcp::socket* sock_pt;
typedef asio::deadline_timer* timer_pt;
typedef job_queue< struct  tcp_message*> queue_type;
typedef boost::object_pool< struct  tcp_message> object_pool_type;
typedef struct  tcp_message*	tcp_message_ptr;
typedef boost::asio::io_service::strand   strand_type;


struct gateway_conf{
	int work_mode;
	int port;		
	char gateway_id[50];	//每个网关的唯一标识
	int pool_interval;
	int timeout;
	int work_status;
};

struct correctTime{
	int target_hour;
	int target_min;

};

#define MAX_BUF_SIZE	300
struct tcp_message{
	unsigned char data[MAX_BUF_SIZE];
	int size;
	char gateway_id[50];	//每个网关的唯一标识
};

class Gateway:public boost::enable_shared_from_this<Gateway>
{
public:
	Gateway(boost::asio::io_service& io, queue_type& q);
	~Gateway();

	bool dns(void);
	void OnConnect(const boost::system::error_code& ec);
	void run_client();
	void run_server();
	void shake_handler(const boost::system::error_code& ec,std::size_t bytes_transferred, tcp_message_ptr ref);
	void wait_time(void);
	void wait_time_callback_fun(const boost::system::error_code&);
	void restart(const boost::system::error_code& ec);
	void JudgeGatewayType();
	void JudgeGatewayTypeVerification(const boost::system::error_code& ec);
	void RecvJudgeGatewayTypeReturn(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void SendOnLineCmd(const boost::system::error_code& ec);
	void SendOnLineCmdVerification(const boost::system::error_code& ec);
	void RecvOnlineReturn(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void SendRequestCmdVerification(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void RecvRequestReturn(const boost::system::error_code& ec, std::size_t bytes_transferred, tcp_message_ptr ref);
	void SendWriteCmdVerification(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void RecvWriteCmdReturn(const boost::system::error_code& ec, std::size_t bytes_transferred);
	int UpdateStatus(unsigned char *buf_recv,int sn);
	void myrecv(const boost::system::error_code& ec,std::size_t bytes_transferred, tcp_message_ptr ref);
	void mysendVerification(const boost::system::error_code& ec, std::size_t bytes_transferred);


	//主动上报方式
	void recvInitiativeData(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void recvInitiativeDataReturn(const boost::system::error_code& ec, std::size_t bytes_transferred, tcp_message_ptr ref);
	void selfCorrectTime(const boost::system::error_code& ec);

public:
	int work_mode;
	int port;		
	char gateway_id[50];	//每个网关的唯一标识
	int pool_interval;
	int timeout;
	int work_status;
	int cmd_j;
	timer_pt wait_timer;
	int NODE_NUM;	//zigbee为64；RFID为247  ;add by whl 2013-11-29

public:		//写操作相关
	unsigned char write_buff[150];
	int write_flag;
	int reval; //1表示返回正确，2表示返回错误
	int sn;
 
public:
	unsigned char buf_newnode[12];
	unsigned char buf_query[12];
	unsigned char buf_recv[200];
	unsigned char buf_online_send[12];
	unsigned char buf_online_recv[50];

public:
	sock_pt sockfd;	
	timer_pt async_timer;
	int timer_flag;//用于判断定时器是否处于定时状态，1处于定时，0非定时状态
	time_t time_start;
	double i_sec;


	boost::asio::io_service& ios;
	asio::ip::tcp::endpoint ep;

public:
	int dataName[32];       //通道的名称
	float dataValue[32];    //通道的值				

private:
	GatewayDB db;
	queue_type& m_queue;
	object_pool_type m_msg_pool;
	strand_type	m_strand;

	struct correctTime targetCorrectTime;

private://上海照明灯系统
	unsigned char mysend_buf_success[8];
	unsigned char mysend_buf_error[8];
};

#endif               
