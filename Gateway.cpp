#include <iostream>
#include <boost/lexical_cast.hpp>
#include "Gateway.h"
#include "log.h"
#include "debug.h"
#include "io_service_pool.cpp"
#include <list>
#include <boost/timer.hpp>
using namespace std;
using namespace boost;
using namespace boost::asio::detail::socket_ops;

#define PACK_HEAD	9
extern io_service_pool io_service_pool_;
extern list<class Gateway* > gateway_object_list;
extern list<struct gateway_conf> gateway_conf_list;


	
Gateway::Gateway(boost::asio::io_service& io, queue_type& q )
:ios(io),write_flag(0), work_status(0),sockfd(NULL), async_timer(NULL),
timer_flag(0),wait_timer(NULL), m_queue(q),m_strand(io) 
{	

		buf_query[0]=0x84;buf_query[1]=0x95;	//事务单元标识符
		buf_query[2]=0x00;buf_query[3]=0x00;	//协议单元标识符
		buf_query[4]=0x00;buf_query[5]=0x06;	//长度
		buf_query[6]=0x01;						//单元标识符——即哪一个传感器节点
		buf_query[7]=0x03;						//功能码
		buf_query[8]=0x00;buf_query[9]=0x00;	//起始地址
		buf_query[10]=0x00;buf_query[11]=0x40;	//读多数个寄存器	

		//15 01    00 00    00 06   41   03      00 14     00  51 
		//查询新节点命令
		memcpy(buf_newnode, buf_query, 12);
		buf_newnode[6] = 0x41;
		buf_newnode[9] = 0x20;
		buf_newnode[11] = 0x51;

		//15 01    00 00    00 06    ff 01          55 55      00 40//0x40  表示读64个线圈
		//查询在线节点命令
		memcpy(buf_online_send, buf_query, 12);
		buf_online_send[6]=0xff;
		buf_online_send[7]=0x01;
		buf_online_send[8]=0x55;buf_online_send[9]=0x55;
		buf_online_send[11]=0xf7;	//0xf4 表示读取247个节圈
		sn=0;
		
		sockfd = new asio::ip::tcp::socket(ios);
		async_timer = new asio::deadline_timer( ios, posix_time::seconds(1) );
		wait_timer = new asio::deadline_timer( ios, posix_time::seconds(1) );

		//定时校准时间
		targetCorrectTime.target_hour = 23;
		targetCorrectTime.target_min = 60;
}

Gateway::~Gateway()
{

	if(1==work_status){
		work_status=0;

	//	sockfd->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	//	sockfd->close();
	//	async_timer->cancel();
	//	wait_timer->cancel();
		delete sockfd;
		delete async_timer;
		delete wait_timer;
	}

#ifdef DEBUG
	cout << __LINE__  << "	" << __FUNCTION__ << " " <<gateway_id<<endl;
#endif
}


void Gateway::run_client()
{

#ifdef DEBUG
cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif
		if(dns() == false) { 
#ifdef DEBUG
			cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<"DNS failure!" <<endl;
#endif
		}
		sockfd->async_connect(ep, m_strand.wrap(bind(&Gateway::OnConnect, this, asio::placeholders::error)));
}


bool Gateway::dns(void)
{
#ifdef DEBUG
cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif
	asio::ip::tcp::resolver resolver(ios);
	boost::system::error_code ec;  
	asio::ip::tcp::resolver::query query(gateway_id, boost::lexical_cast<std::string, unsigned short>(port));
	asio::ip::tcp::resolver::iterator iter = resolver.resolve(query, ec); 
	asio::ip::tcp::resolver::iterator end; 
	if (iter == end || ec != 0) return false;
	ep = *iter;
#ifdef DEBUG
	cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ << "  "<<gateway_id<<" :  IP: "<<ep.address()<<endl;
	//log_output("Gateway::dns: %s	: IP: %s", gateway_id, ep.address());
#endif
	return true;
}


void Gateway::OnConnect(const boost::system::error_code& ec)
{
//cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
		if (ec)
		{ 
#ifdef DEBUG
	cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__<<gateway_id <<"IP: "<< ep.address()<< " "<< boost::system::system_error(ec).what() <<endl;
#endif
				if(dns() == false) { 
#ifdef DEBUG
						cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ << "DNS failure!"<<endl;
#endif			
				}

				wait_time();
				return;
		}
		work_status=1;		
		//async_timer->async_wait(m_strand.wrap(bind(&Gateway::JudgeGatewayType, this, asio::placeholders::error)));
		JudgeGatewayType();
}

void Gateway::wait_time(void)
{ 
	wait_timer->expires_from_now(posix_time::seconds( timeout ) );
	wait_timer->async_wait(m_strand.wrap(bind(&Gateway::wait_time_callback_fun, this, asio::placeholders::error)));
}

void Gateway::wait_time_callback_fun(const boost::system::error_code& ec)
{
	if(ec){
		log_output("%s: %d: %s: %s\n", __FILE__, __LINE__, __FUNCTION__, boost::system::system_error(ec).what());
		restart(ec);
		return;  
	} 
	sockfd->async_connect(ep, m_strand.wrap(bind(&Gateway::OnConnect, this, asio::placeholders::error))); 
}

void Gateway::run_server()
{
/*
	tcp_message_ptr ref = m_msg_pool.construct(					//内存池创建消息对象
		shared_from_this(),										//传递Gateway共享指针
		bind(&object_pool_type::destroy,boost::ref(m_msg_pool),_1));	//销毁器,一定要是boost::ref(),否则与tcp_message_ptr ref 重名，编译出错！！！
*/;
	//tcp_message_ptr ref = m_msg_pool.construct(bind(&object_pool_type::destroy,boost::ref(m_msg_pool),_1));
	tcp_message_ptr	ref = new struct tcp_message;
	sockfd->async_read_some(asio::buffer(ref->data, 22), m_strand.wrap(bind(&Gateway::shake_handler, this,\
		asio::placeholders::error, asio::placeholders::bytes_transferred, ref)));	
}


void Gateway::shake_handler(const boost::system::error_code& ec,std::size_t bytes_transferred, tcp_message_ptr ref )
{
#ifdef DEBUG
cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif
	work_status=1;
	if(ec)  
	{  
		cout << "shake_handler" << boost::system::system_error(ec).what() << endl;
		delete this;
		return;  
	}


	//检测协议单元标识符和长度是否正确
	if (ref->data[2]!=0x22||ref->data[3]!=0x22||ref->data[4]!=0x00||(ref->data[5]!=0x08 && ref->data[5]!=0x10) ) {
#ifdef DEBUG
		int i = 0;
		cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
		cout << "bytes_transferred: " << bytes_transferred <<endl;

		for(i = 0; i < 22; i++)
		{
			printf("%.2x ", ref->data[i]);
		}
		printf("\n");
#endif		

		ref->data[5]=(char)0x01;
		ref->data[6]=(char)0x01;
#ifdef DEBUG
cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif
	try{
		sockfd->write_some(asio::buffer(ref->data,7));
	}catch(...){
#ifdef DEBUG
cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif
		cout << "write_some error "  <<endl;
		restart(ec);
		return;
	}
		delete this;
		return;
	}



	// 解析数据内容，保存设备序列号
	int n = 0;
	char gateway_logo[50];
	memset(gateway_logo, '\0', 50);

	if(20 == bytes_transferred||14 == bytes_transferred){//兼容8字节老序列号
		for(n=0;n<8;n++){
			sprintf(gateway_logo+n*2,"%.2x",ref->data[6+n]);
		}
	}else if(22 == bytes_transferred){//16字节新序列号
			memcpy(gateway_logo, &ref->data[6], 16);
	}else{
		delete this;
		return;
	}


	/*在网关配置表中查找序列号*/
	list<struct gateway_conf>::iterator theIterator;
	for( theIterator = gateway_conf_list.begin(); theIterator != gateway_conf_list.end(); theIterator++ ){
		if(0 == strcmp(gateway_logo,theIterator->gateway_id)){
			this->work_mode=theIterator->work_mode;
			this->port=theIterator->port;
			strcpy(this->gateway_id, gateway_logo);
			this->timeout=theIterator->timeout;
			this->pool_interval = theIterator->pool_interval;
			break;
		}
	}
	if(theIterator == gateway_conf_list.end()){//没有该网关的配置
		#ifdef DEBUG
		cout<<__FUNCTION__<<": cant not find the gateway:"<< gateway_logo<<endl;
		#endif
		delete this;
		return;
	}



	//响应客户端的握手包,给客户端发送7个字节
	ref->data[5]=(char)0x01;
	ref->data[6]=(char)0x80;
	try{
		sockfd->write_some(asio::buffer(ref->data,7));
	}catch(...){
#ifdef DEBUG
cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif
		cout << "write_some error " <<endl;
		restart(ec);
		return;
	}

	//gateway_object_list.push_back(this);
	gateway_object_list.push_front(this);
	
	time(&time_start);		
	//async_timer->async_wait(m_strand.wrap(bind(&Gateway::JudgeGatewayType, this, asio::placeholders::error)));

	if(0x60 == ref->data[1]){//主动上报方式		
		tcp_message_ptr	ref = new struct tcp_message;
		sockfd->async_read_some(asio::buffer(ref->data, 137), m_strand.wrap(bind(&Gateway::recvInitiativeDataReturn, \
			this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, ref)));


		//当前时间
		time_t timenow = time(NULL);
		struct tm *ptm = localtime(&timenow);
		int duration_min = abs(targetCorrectTime.target_hour - ptm->tm_hour)*60 + abs(targetCorrectTime.target_min - ptm->tm_min);

		//启动自动校准定时器
		async_timer->expires_from_now(posix_time::minutes(duration_min) );
		async_timer->async_wait(m_strand.wrap(bind(&Gateway::selfCorrectTime, this, asio::placeholders::error)));


	}else{//查询应答方式
		JudgeGatewayType();
	}	
}


void Gateway::restart(const boost::system::error_code& ec)
{
#ifdef DEBUG
printf("%s: %d: %s: %s: %s\n", __FILE__, __LINE__, __FUNCTION__, gateway_id,boost::system::system_error(ec).what());
#endif   
	if(1 == work_status){
			if(0 == work_mode ){//网关是服务器
				sockfd->shutdown(boost::asio::ip::tcp::socket::shutdown_both, const_cast<boost::system::error_code&>(ec));
				sockfd->close();
				async_timer->cancel();
				//wait_timer->cancel();
				work_status=0;
				run_client();
			}else{
#ifdef DEBUG
				printf("%s: %d: %s: %s: %s\n", __FILE__, __LINE__, __FUNCTION__, gateway_id,boost::system::system_error(ec).what());
#endif 
				gateway_object_list.remove(this);
				delete this;
			}			
	}
}

//判断网关的类型：是zgibee型的还是RFID型的
void Gateway::JudgeGatewayType()
{
	PR_DEBUG();
	timer_flag=0;
	sockfd->async_write_some(asio::buffer(buf_online_send,sizeof(buf_online_send)), m_strand.wrap(bind(&Gateway::JudgeGatewayTypeVerification, this, boost::asio::placeholders::error)));
}

void Gateway::JudgeGatewayTypeVerification(const boost::system::error_code& ec)
{
	PR_DEBUG();
	if(ec){  
#ifdef DEBUG
		printf("%s: %d: %s: %s\n", __FILE__, __LINE__, __FUNCTION__, boost::system::system_error(ec).what());
#endif        
		restart(ec);
		return;  
	}
	memset(buf_online_recv, '\0', sizeof(buf_online_recv));
	sockfd->async_read_some(asio::buffer(buf_online_recv, sizeof(buf_online_recv)), m_strand.wrap(bind(&Gateway::RecvJudgeGatewayTypeReturn,\
		this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void Gateway::RecvJudgeGatewayTypeReturn(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	PR_DEBUG();
	//15 01 00 00 00 0B FF 01 08 FF FF FF FF FF FF FF FF
	if(ec){  
#ifdef DEBUG
		cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__<<gateway_id<< boost::system::system_error(ec).what() <<endl;
#endif
		restart(ec);
		return;  
	}
	
	if((9 == bytes_transferred) && buf_online_recv[7]==0x81 && buf_online_recv[8]==0x02){/*zigBee64点网关*/
		NODE_NUM=64;
		buf_online_send[11]=0x40;
	}else{	/*RFID 247点网关*/
		NODE_NUM=247;
		buf_online_send[11]=0xf7;
	}

	sockfd->async_write_some(asio::buffer(buf_online_send,sizeof(buf_online_send)), m_strand.wrap(bind(&Gateway::SendOnLineCmdVerification, this, boost::asio::placeholders::error)));

	//async_timer->async_wait(m_strand.wrap(bind(&Gateway::SendOnLineCmd, this, asio::placeholders::error)));
}
//发送请求节点在线状态命令
void Gateway::SendOnLineCmd(const boost::system::error_code& ec)
{
//15 01 00 00 00 06 ff 01 55 55 00 40
    if(ec){ 
#ifdef DEBUG
		printf("%s: %d: %s: %s\n", __FILE__, __LINE__, __FUNCTION__, boost::system::system_error(ec).what());
#endif

		if(0 == work_status){
			restart(ec);
			return;  
		}

		if(1 == write_flag){
			sockfd->async_write_some(asio::buffer(write_buff,6+write_buff[5]), m_strand.wrap(bind(&Gateway::SendWriteCmdVerification, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
			return;
		}
    } 

	timer_flag=0;
	sockfd->async_write_some(asio::buffer(buf_online_send,sizeof(buf_online_send)), m_strand.wrap(bind(&Gateway::SendOnLineCmdVerification, this, boost::asio::placeholders::error)));
}

//发送请求节点在线状态命令异处理函数，如果发送正确，注册读
void Gateway::SendOnLineCmdVerification(const boost::system::error_code& ec)
{
	if(ec){  
#ifdef DEBUG
		printf("%s: %d: %s: %s\n", __FILE__, __LINE__, __FUNCTION__, boost::system::system_error(ec).what());
#endif        
		restart(ec);
        return;  
    }
	memset(buf_online_recv, '\0', sizeof(buf_online_recv));
	sockfd->async_read_some(asio::buffer(buf_online_recv, sizeof(buf_online_recv)), m_strand.wrap(bind(&Gateway::RecvOnlineReturn,\
		this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

//读取节点在线状态异步处理函数，如果无错误，则发送0x03请求命令
void Gateway::RecvOnlineReturn(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
//15 01 00 00 00 0B FF 01 08 FF FF FF FF FF FF FF FF
	if(ec){  
#ifdef DEBUG
	cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__<<gateway_id<< boost::system::system_error(ec).what() <<endl;
#endif
        restart(ec);
        return;  
    }

	/*请求节点在线状态命令返回错误 15 01 00 00 00 03 FF 81 01 */
	if((buf_online_recv[5] == 0x03) && buf_online_recv[7] == 0x81){
		timer_flag=1;
		time(&time_start);
		async_timer->expires_from_now(posix_time::seconds( pool_interval ) );
		async_timer->async_wait(m_strand.wrap(bind(&Gateway::SendOnLineCmd, this, asio::placeholders::error)));
		return;
	}

	cmd_j=0;
	int online_flag=0;
	while(cmd_j<NODE_NUM){
		online_flag=buf_online_recv[9+cmd_j/8];
		if(((online_flag>>(cmd_j%8)) & 0x01) == 0x00){//如果节点不在线
			cmd_j++;
			continue;
		}
		break;
	}

	if(NODE_NUM == cmd_j){
		timer_flag=1;
		time(&time_start);
		async_timer->expires_from_now(posix_time::seconds( pool_interval ) );
		async_timer->async_wait(m_strand.wrap(bind(&Gateway::SendOnLineCmd, this, asio::placeholders::error)));
		return;
	}

	buf_query[6] = cmd_j+1;	 
	sockfd->async_write_some(asio::buffer(buf_query,sizeof(buf_query)), m_strand.wrap(bind(&Gateway::SendRequestCmdVerification, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

//发送0x03命令请求的异步处理函数，如果无误，则注册0x03命令读函数
void Gateway::SendRequestCmdVerification(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if(ec){   
#ifdef DEBUG
	cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__<<gateway_id<< boost::system::system_error(ec).what() <<endl;
#endif
        restart(ec);
        return;  
    }
	//tcp_message_ptr ref = m_msg_pool.construct(bind(&object_pool_type::destroy,boost::ref(m_msg_pool),_1));
	tcp_message_ptr	ref = new struct tcp_message;
	memset(ref, '\0', sizeof(struct tcp_message));

	sockfd->async_read_some(asio::buffer(ref->data, 137), m_strand.wrap(bind(&Gateway::RecvRequestReturn, \
		this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, ref)));
}

//0x10命令的异步处理函数
void Gateway::SendWriteCmdVerification(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if(ec){
#ifdef DEBUG
		cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif
		log_output("%s %d %s %s %s \n", __FILE__, __LINE__, __FUNCTION__,gateway_id, boost::system::system_error(ec).what() );
	}
	memset(buf_recv, '\0', sizeof(buf_recv));
	sockfd->async_read_some(asio::buffer(buf_recv, 137), m_strand.wrap(bind(&Gateway::RecvWriteCmdReturn, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));

}

//处理0x10命令的返回
void Gateway::RecvWriteCmdReturn(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if(ec){
	#ifdef DEBUG
		cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
	#endif
		log_output("%s %d %s %s %s \n", __FILE__, __LINE__, __FUNCTION__,gateway_id, boost::system::system_error(ec).what() );
	}

	UpdateStatus(buf_recv,sn);
	write_flag=0;
	if(i_sec>0 && timer_flag == 1){
		async_timer->expires_from_now(posix_time::seconds(  pool_interval - (int)i_sec  ) );
		async_timer->async_wait(m_strand.wrap(bind(&Gateway::SendOnLineCmd, this, asio::placeholders::error)));
		return ;
	}

	/*0x03*/
	cmd_j++;
	int online_flag=0;
	while(cmd_j<NODE_NUM){
		online_flag=buf_online_recv[9+cmd_j/8];
		if(((online_flag>>(cmd_j%8)) & 0x01) == 0x00){//如果节点不在线
			cmd_j++;
			continue;
		}
		break;
	}
	if(NODE_NUM == cmd_j){
		timer_flag=1;
		time(&time_start);
		async_timer->expires_from_now(posix_time::seconds( pool_interval ) );
		async_timer->async_wait(m_strand.wrap(bind(&Gateway::SendOnLineCmd, this, asio::placeholders::error)));
		
		return;
	}
	buf_query[6] = cmd_j+1;	 
	sockfd->async_write_some(asio::buffer(buf_query,sizeof(buf_query)), m_strand.wrap(bind(&Gateway::SendRequestCmdVerification, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));


}
//0x03读命令的异步处理函数，如果无误，则注册下发0x03命令函数
void Gateway::RecvRequestReturn(const boost::system::error_code& ec, std::size_t bytes_transferred, tcp_message_ptr ref)
{
	//timer t;
	if(ec){    
	#ifdef DEBUG
		cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
	#endif
		log_output("%s %d %s %s %s \n", __FILE__, __LINE__, __FUNCTION__,gateway_id, boost::system::system_error(ec).what() );
        restart(ec);
        return;  
    }
	
	if(bytes_transferred != 9){
		ref->size=bytes_transferred;
		memset(ref->gateway_id, '\0', sizeof(ref->gateway_id));
		strcpy(ref->gateway_id,gateway_id);
		m_queue.push(ref);
	}

#ifdef DEBUG
	/*
	int n=0;
	for(n=0;n<12;n++){
		log_output("%.2x ",buf_query[n]);
	}log_output("\n");

	for(n=0;n<137;n++){
		log_output("%.2x ",ref->data[n]);
	}log_output("\n");
*/
	printf("%s\t%d\n",ref->gateway_id, ref->data[6] );
#endif	
	


	/********************************下发查询命令 start ***************************************/
	/*0x10*/
	if(1 == write_flag){
#ifdef DEBUG
		printf(">>>>>>>>>>>>>>>>>>>hello 0x10\n");

		int i=0;
		for(i=0; i< 6+write_buff[5];i++){
			
			printf("%.2x ", write_buff[i]);
		}
		printf("\n");
#endif
		sockfd->async_write_some(asio::buffer(write_buff,6+write_buff[5]), m_strand.wrap(bind(&Gateway::SendWriteCmdVerification, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
		return;
	}
		
	/*0x03*/
	cmd_j++;

	int online_flag=0;
	while(cmd_j<NODE_NUM){
		online_flag=buf_online_recv[9+cmd_j/8];
		if(((online_flag>>(cmd_j%8)) & 0x01) == 0x00){//如果节点不在线
			cmd_j++;
			continue;
			}
		break;
	}
	if(NODE_NUM == cmd_j){
		if(192000 ==m_queue.size() )
			cout << m_queue.size()<<endl;
		timer_flag=1;
		time(&time_start);		
		async_timer->expires_from_now(posix_time::seconds( pool_interval ) );
		async_timer->async_wait(m_strand.wrap(bind(&Gateway::SendOnLineCmd, this, asio::placeholders::error)));
		
		return;
	}

	buf_query[6] = cmd_j+1;	 
	sockfd->async_write_some(asio::buffer(buf_query,sizeof(buf_query)), m_strand.wrap(bind(&Gateway::SendRequestCmdVerification, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
	/********************************下发查询命令 end ***************************************/
}

int Gateway::UpdateStatus(unsigned char *buf_recv,int sn)
{
#ifdef DEBUG
	cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif

		char strTime[32] = {0}; 
		char sql[4096] = {0};
		struct tm *pst_time = NULL;
		time_t timenow;
		time(&timenow);
		pst_time = localtime(&timenow);
		strftime(strTime, 32, "%Y-%m-%d %H:%M:%S", pst_time);	

		if(0x10 == buf_recv[7]){
#ifdef DEBUG
				printf("0x10 cmd  success! >>>>>>>>>>>>>>>>>\n");
#endif
				snprintf(sql, sizeof(sql) - 1, "UPDATE write_table SET cmd_status=1, reval=1, update_time = '%s' WHERE sn=%d;", strTime, sn); 

				if(0x31 == buf_recv[9]){
					log_output("%s: correct time success\n", this->gateway_id);
				}
		}else if(0x90 == buf_recv[7]){
#ifdef DEBUG
				printf("0x10 cmd failure! >>>>>>>>>>>>>>>>>\n");
#endif
				snprintf(sql, sizeof(sql) - 1, "UPDATE write_table SET cmd_status=1, reval=2, update_time = '%s' WHERE sn=%d;", strTime, sn); 
		}

	if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
#ifdef DEBUG
		cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
		cout << sql<< endl;
		log_output("%s/n", sql);
#endif
	}else{	
		db.UpdateData(sql);
		db.DisConnectDB();
	}

	return 0;
}



#if 0
 
//处理配置新点的返回，包括验证信息，查询新节点命令，配置新点命令
int Gateway::ConfNode()
{
		if(0x10 == buf_recv[7] && 0x41 == buf_recv[6] && (0x71 == buf_recv[9] || 0x9b == buf_recv[9] )){	//配置节点入网/ 或删除返回正确
				memset(write_buff, '\0', sizeof(write_buff));
				write_flag=0;
				reval = 1;
				cout << "ConfNode():  0x10 >>>>>>>>>>>>>>>>>\n";

		} else if(0x90 == buf_recv[7] && 0x41 == buf_recv[6]){	//配置节点入网或删除返回错误
				memset(write_buff, '\0', sizeof(write_buff));
				write_flag=0;
				reval = 2;
				cout << "ConfNode():  0x90 >>>>>>>>>>>>>>>>>\n";

		}

		return 0;
}


//把查询到的节点存到数据库表中
int Gateway::AddNewNodeListToDB()
{
		char strTime[32] = {0}; 
		char sql[4096] = {0};
		struct tm *pst_time = NULL;
		time_t timenow;
		time(&timenow);
		pst_time = localtime(&timenow);
		strftime(strTime, 32, "%Y-%m-%d %H:%M:%S", pst_time);
		char node_addr[17];
		unsigned char *p=NULL;
		int i = 0;
		DBresult *res = NULL;
		int new_node = buf_recv[10];
		cout << "the gateway  " << gateway_id << " has " << new_node << "	new Node \n";
		for(i=0; i<new_node; i++){
				memset(node_addr, '\0', sizeof(node_addr));
				p = buf_recv+11 + i*16;
				memcpy(node_addr, p, 16);
				snprintf(sql, sizeof(sql) - 1, "select * from  new_node where gateway_logo = '%s' and node_addr = '%s';", gateway_id, node_addr);

				res = db.SelectData(sql);                										        
				memset(sql, '\0', sizeof(sql));			
				if(db.DBntuples(res)>0){
						cout << "already has node addr!!!" << endl;
				}else{
						cout << "db.DBntuples(res) = " << db.DBntuples(res) << endl; 
						snprintf(sql, sizeof(sql) - 1, "insert into new_node (gateway_logo, node_addr, status, date) values ('%s', '%s',0, to_timestamp('%s', 'YYYY-MM-DD HH24:MI:SS'));", gateway_id, node_addr, strTime);

						db.InsertData(sql);
						log_output("%s\n", sql);
						cout << "insert new node ok!!!" << endl;
				}
		}

		db.DBclear(res);

		return 0;

}

#endif


//主动上报方式
void Gateway::recvInitiativeData(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
if(ec){   
#ifdef DEBUG
	cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__<<gateway_id<< boost::system::system_error(ec).what() <<endl;
#endif
	restart(ec);
	return;  
	}
cout << "bytes send: " << bytes_transferred<<endl;
	tcp_message_ptr	ref = new struct tcp_message;
	memset(ref, '\0', sizeof(struct tcp_message));

	sockfd->async_read_some(asio::buffer(ref->data, 137), m_strand.wrap(bind(&Gateway::recvInitiativeDataReturn, \
		this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, ref)));
}

void Gateway::recvInitiativeDataReturn(const boost::system::error_code& ec, std::size_t bytes_transferred, tcp_message_ptr ref)
{
	if(ec){    
#ifdef DEBUG
		cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif
		log_output("%s %d %s %s %s \n", __FILE__, __LINE__, __FUNCTION__,gateway_id, boost::system::system_error(ec).what() );
		restart(ec);
		return;  
	}
#if 0
//#ifdef	DEBUG
	int i =0;
	for(i=0; i<bytes_transferred; i++)
		printf("%.2x ", ref->data[i]);
	printf("\n");
	printf("bytes_transferred = %d\n", bytes_transferred);
#endif

	if((0x60 == ref->data[7]) && (bytes_transferred == ref->data[8]+9) && (0x00 == ref->data[2] && 0x00 == ref->data[3])){
		//返回正确响应包			
		unsigned char responseInitiativeBuf[8]={0x15, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01,0x60};

		sockfd->async_write_some(asio::buffer(responseInitiativeBuf,sizeof(responseInitiativeBuf)),\
			m_strand.wrap(bind(&Gateway::recvInitiativeData,this, boost::asio::placeholders::error,\
			boost::asio::placeholders::bytes_transferred)));	

		//将接收到的正确的数据包放到队列中
		if(bytes_transferred != 9){
			ref->size=bytes_transferred;
			memset(ref->gateway_id, '\0', sizeof(ref->gateway_id));
			strcpy(ref->gateway_id,gateway_id);
			m_queue.push(ref);
			}	

	}else if(bytes_transferred == 12 && ref->data[0]==0x15 && ref->data[1] == 0x01 && ref->data[5] == 0x06 &&\
			ref->data[6] == 0xff && ref->data[7] == 0x10 && ref->data[9] == 0x31&& ref->data[11] == 0x06){   /*对时命令成功*/
				//正确响应包：15 01  00 00  00 06  FF   10   00 31   00 0A
				printf("correction timer success!\n");
				UpdateStatus(ref->data,sn);

				boost::system::error_code ec; 
				recvInitiativeData(ec, 0);

	}else if(bytes_transferred == 9 && ref->data[0]==0x15 && ref->data[1] == 0x01 && ref->data[5] == 0x03 && \
				ref->data[6] == 0xff && ref->data[7] == 0x90 && ref->data[8] == 0x01){ /*对时命令失败*/
					//错误响应包：15 01  00 00  00 03 FF 90 01（02/03/04）
					printf("correction timer failure!\n");
					UpdateStatus(ref->data,sn);
					boost::system::error_code ec; 
					recvInitiativeData(ec, 0);

	}else{
		//返回错误响应包
		unsigned char responseInitiativeBuf[8]={0x15, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01,0xE0};

		sockfd->async_write_some(asio::buffer(responseInitiativeBuf,sizeof(responseInitiativeBuf)),\
			m_strand.wrap(bind(&Gateway::recvInitiativeData,\
			this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
	}
}

//发送请求节点在线状态命令
void Gateway::selfCorrectTime(const boost::system::error_code& ec)
{
	//15 01 00 00 00 06 ff 01 55 55 00 40
	if(ec){ 
#ifdef DEBUG
		printf("%s: %d: %s: %s\n", __FILE__, __LINE__, __FUNCTION__, boost::system::system_error(ec).what());
#endif
		return;
	} 

	//15 01   00 00   00 13   FF  10   00 31    00 06   0C    F3 00 0E 06 F4 00 19 12 F5 00 24 2D
	unsigned char send_buf[25] = {0x15, 0x01, 0x00, 0x00, 0x00, 0x13, 0xFF, 0x10, 0x00, 0x31, 0x00, 0x06, 0x0C, \
		0xF3, 0x00, 0x00, 0x00,0xF4,0x00, 0x00, 0x00, 0xF4,0x00, 0x00, 0x00};

	time_t timenow = time(NULL);
	struct tm *ptm = localtime(&timenow);
	send_buf[15] = (ptm->tm_year+1900) % 100;//year
	send_buf[16] = ptm->tm_mon+1;//month
	send_buf[19] = ptm->tm_mday;//day
	send_buf[20] = ptm->tm_hour;//hour
	send_buf[23] = ptm->tm_min;//min
	send_buf[24] = ptm->tm_sec;//second

	int ret=0;
	try{
		ret = sockfd->write_some(asio::buffer(send_buf,25));

	}catch(...){
		cout << "write_some error " <<endl;
		restart(ec);
		return;
	}

	int duration_min = abs(targetCorrectTime.target_hour - ptm->tm_hour)*60 + abs(targetCorrectTime.target_min - ptm->tm_min);

	//启动自动校准定时器
	async_timer->expires_from_now(posix_time::minutes(duration_min) );
	async_timer->async_wait(m_strand.wrap(bind(&Gateway::selfCorrectTime, this, asio::placeholders::error)));

}
