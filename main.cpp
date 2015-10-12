#include <stdio.h>
#include <iostream>
#include <boost/thread.hpp>
#include <list>
#include "IniFile.h"
#include "pgdb.h"
#include "Server.h"
#include "WriteCmd.h"
#include "UdpServer.h"
#include "InitDaemo.h"
#include "io_service_pool.cpp"
#include "job_queue.hpp"
#include "work.hpp"
#include "main.h"
#include "DataProcess.h"
#include "Log.h"
extern int appender_object;
bool func_int(int tmp)
{
		cout<<"tmp:"<<tmp<<endl;
		return true;
}

#ifdef WIN32 //=====>windows
int Startup()
#else //======>linux
int main(int argc, char * argv[])
#endif

{
#ifndef WIN32//linux

		if((argc!=2)||(0 != strcmp(argv[1], "-d") && (0 != strcmp(argv[1], "-D")))){
				cout <<"Linux Daemon mode..." << endl;
				InitDaemo(argv[0]);
		}else {
				appender_object=0;//将日志写到控制台上
				cout << "Linux DEBUG mode..."	<<endl;
		}
		// 打开日志  
		if (!Log::instance().open_log())  
		{   
				std::cout << "Log::open_log() failed" << std::endl;  
				exit(-1);
		}   
#endif

		/*
		//此段代码用于测试工作队列
		job_queue<int> q1;
		q1.push(10);
		q1.push(11);
		q1.push(13);
		q1.push(14);
		int tmp;
		q1.wait_and_pop(tmp);
		cout<<tmp<<endl;
		cout << "queue size is: "<< q1.size()<<endl;
		return 0;

*/

		/*创建队列*/
		queue_type q;

		/*初始化数据库配置*/
		init_db_conf();	
		/*启动50个线程*/
		io_service_pool_.start();
		/*初始化网关配置链表*/
		int count=0;
		while(count < 5){
				if (init_gateway_conf()<0){ 
						FATAL("init gateway config failure!");
				}else break;
				this_thread::sleep(posix_time::seconds(30));
		}
		if(5 == count){
				return -1;
		}

		// 显示这个链表
		/*
		   list<struct gateway_conf>::iterator theIterator;
		   for( theIterator = gateway_conf_list.begin(); theIterator != gateway_conf_list.end(); theIterator++ )
		   cout << theIterator->gateway_id<<" "<< theIterator->work_mode<<" "<< theIterator->port<<" "\
		   << theIterator->timeout<<" "<< theIterator->pool_interval<<endl;			
		   */


		/*初始化网关对象链表*/
		init_gateway_object_list(q);

		/*启动客户端，连接网关服务器*/
#ifdef	RECVD_CLIENT
		start_client();
#endif

		/*启动服务器，接收网关客户端上来的连接*/
		Server MbsServer(io_service_pool_.get_io_service(), q, tcpport);

		/*启动UDP 服务器*/
		UdpServer s(io_service_pool_.get_io_service(), udpport, q);

		NOTICE("收数软件"<< RECVD_TYPE << RECVD_VERSION << "启动成功！");


#ifndef LIGHTSYS
		worker<queue_type> w(q, handle_msg,10);
#else
		worker<queue_type> w(q, handle_msg,1);
#endif
		w.start();

		io_service_pool_.join();//等待线程退出

		return 0;
}

//初始化数据库变量
void init_db_conf(){
		DEBUG(__func__);
		IniFile config(CONFIG_PATH);
#ifdef ODBC
		strcpy(dbaddr, config.ReadString("db", "DB_DSN", ""));
#else
		strcpy(dbaddr, config.ReadString("db", "DB_ADDR", ""));
#endif
		dbport = config.ReadInteger("db", "DB_PORT", 0); 
		strcpy(dbname, config.ReadString("db", "DB_NAME", ""));
		strcpy(dbuser, config.ReadString("db", "DB_USER", ""));
		strcpy(dbpwd, config.ReadString("db", "DB_PWD", ""));
		strcpy(dbtype, config.ReadString("db", "DB_TYPE", ""));
		tcpport = config.ReadInteger("db", "TCP_PORT", 502);
		udpport = config.ReadInteger("db", "UDP_PORT", 8000);
		record_channel_num_flag = config.ReadInteger("db", "RECORD_CHANNEL_NUM_FLAG", 0);
		polling_unit = config.ReadInteger("db", "POLLING_UNIT", 0);
}


//查询网关配置，并保存到相应的对象中
void  init_gateway_object_list(queue_type& m_queue){
		DEBUG(__func__);

		list<struct gateway_conf>::iterator theIterator;
		for( theIterator = gateway_conf_list.begin(); theIterator != gateway_conf_list.end(); theIterator++ ){
				if(0 == theIterator->work_mode){//网关工作在服务器模式
						Gateway *p_gateway=new Gateway(io_service_pool_.get_io_service(), m_queue);
						p_gateway->work_mode = theIterator->work_mode;
						strcpy(p_gateway->gateway_id,theIterator->gateway_id);
						p_gateway->port = theIterator->port;
						p_gateway->pool_interval=theIterator->pool_interval;
						p_gateway->timeout=theIterator->timeout;
						gateway_object_list.push_back(p_gateway);
				}
		}

}
bool is_valid_id(const char *p)
{
		if(strlen(p)!=16)
				return false;
		int i=0;
		for(i=0;i<16;i++) {
				if(*(p+i)>='0'&& *(p+i)<='9')
						continue;
				else
						break;
		}

		return (i==16)? true:false;

}
bool is_valid_ip(const char *ip)
{
		int section = 0;  //每一节的十进制值 
		int dot = 0;       //几个点分隔符 
		int last = -1;     //每一节中上一个字符 
		while(*ip)
		{ 
				if(*ip == '.')
				{ 
						dot++; 
						if(dot > 3)
						{ 
								return 0; 
						} 
						if(section >= 0 && section <=255)
						{ 
								section = 0; 
						}else{ 
								return 0; 
						} 
				}else if(*ip >= '0' && *ip <= '9')
				{ 
						section = section * 10 + *ip - '0'; 
						if(last == '0')
						{ 
								return 0; 
						} 
				}else{ 
						return 0; 
				} 
				last = *ip; 
				ip++;        
		}

		if(section >= 0 && section <=255)
		{ 
				if(3 == dot)
				{
						section = 0; 
						return true;
				}
		} 
		return false; 

}

//从数据库中读出网关的配置，并保存到链表中 
int  init_gateway_conf(){
		DEBUG(__func__);

		if (db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60) == -1){				
				FATAL("connect DataBase failed!");
				return -1;
		}

		int i,t,s,k;
		DBresult *res;                 

		res = db.SelectData("select * from gateway_conf;");

		i = db.DBntuples(res);
		/**取得查询的结果的记录的数量*/ 
		t = db.DBnfields(res);
		/**取得字段数量*/

		for(s=0; s<i;s++)
		{
				struct gateway_conf  *p_gateway_conf=NULL;
				char gateway_id[50];
				memset(gateway_id, 0, sizeof(gateway_id));
				strcpy(gateway_id, db.DBgetvalue(res,s,3));
				for(k=0;k<50;k++){
						if(gateway_id[k]==0x20)
								gateway_id[k] = 0x00;
				}
				p_gateway_conf = new struct gateway_conf;
				p_gateway_conf->work_mode = atoi(db.DBgetvalue(res,s,1));                                 
				strcpy(p_gateway_conf->gateway_id,gateway_id);		
				p_gateway_conf->port = atoi(db.DBgetvalue(res,s,2));
				if(0 == polling_unit) {
						p_gateway_conf->pool_interval=60*(atoi(db.DBgetvalue(res,s,4)));
				} else {
						p_gateway_conf->pool_interval=atoi(db.DBgetvalue(res,s,4));
				}
				p_gateway_conf->timeout=atoi(db.DBgetvalue(res,s,5));

				//验证IP地址是否合法
				if(p_gateway_conf->work_mode == 0) {
						if(!is_valid_ip(p_gateway_conf->gateway_id)) {
								WARNING("device server mode, IP:" << p_gateway_conf->gateway_id << " invalid");
								delete p_gateway_conf;
								continue;
						}
				}else if(p_gateway_conf->work_mode == 1) { /*验证序列号是否全法*/
						if(!is_valid_id(p_gateway_conf->gateway_id)) {
								WARNING("device client mode, serial number " << p_gateway_conf->gateway_id << " invalid");
								delete p_gateway_conf;
								continue;
						}
				}

				gateway_conf_list.push_back(*p_gateway_conf);
		}
		db.DBclear(res);
		db.DisConnectDB();

		return 0;
}



//启动客户端，连接到网关服务器
void start_client()
{
		DEBUG(__func__);
		list<class  Gateway* >::iterator theIterator;
		for( theIterator = gateway_object_list.begin(); theIterator != gateway_object_list.end(); theIterator++ ){
				(*theIterator)->run_client();
		}
}

