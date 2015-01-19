#include <stdio.h>
#include <iostream>
#include <boost/thread.hpp>
#include <list>
#include "IniFile.h"
#include "log.h"
#include "pgdb.h"
#include "Server.h"
#include "WriteCmd.h"
#include "UdpServer.h"
#include "InitDaemo.h"
#include "debug.h"
#include "io_service_pool.cpp"
#include "job_queue.hpp"
#include "work.hpp"

#include "main.h"
#include "DataProcess.h"



bool func_int(int tmp)
{
	cout<<"tmp:"<<tmp<<endl;
	return true;
}

//windows平台
#ifdef WIN32
	#ifdef DEBUG 
		int main(int argc, char * argv[])//windows平台调试模式
	#else
		int Startup()//替代main()函数, windows平台正常模式
	#endif
#else
int main(int argc, char * argv[])
#endif

{
#ifdef DEBUG	
#ifdef WIN32
init_path();//初始化配置文件的路径,windows平台模式
#endif
#endif

//linux平台
#ifndef WIN32
	init_path();//初始化配置文件的路径
	#ifndef DEBUG
		InitDaemo(argv[0]);
	#endif
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
			log_output("init gateway config  failure!\n");
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
	start_client();

	/*启动服务器，接收网关客户端上来的连接*/
	Server MbsServer(io_service_pool_.get_io_service(), q, tcpport);

	/*启动UDP 服务器*/
	UdpServer s(io_service_pool_.get_io_service(), udpport, q);

	log_output("recv-data-plaform Service start ok!\n" );


	worker<queue_type> w(q, handle_msg,10);
	w.start();

	io_service_pool_.join();//等待线程退出

	return 0;
}




//初始化数据库变量
void init_db_conf(){
	PR_DEBUG();

	IniFile config(CONFIG_PATH);
#ifdef ODBC
	log_level = config.ReadInteger("log", "LOG_LEVEL", 1); 
	strcpy(dbaddr, config.ReadString("db", "DB_DSN", ""));
	dbport = 0; 
	strcpy(dbname, config.ReadString("db", "DB_NAME", ""));
	strcpy(dbuser, config.ReadString("db", "DB_USER", ""));
	strcpy(dbpwd, config.ReadString("db", "DB_PWD", ""));
	strcpy(dbtype, config.ReadString("db", "DB_TYPE", ""));
	tcpport = config.ReadInteger("db", "TCP_PORT", 502);
	udpport = config.ReadInteger("db", "UDP_PORT", 8000);
	record_channel_num_flag = config.ReadInteger("db", "RECORD_CHANNEL_NUM_FLAG", 0);
#else
	log_level = config.ReadInteger("log", "LOG_LEVEL", 1); 
	strcpy(dbaddr, config.ReadString("db", "DB_ADDR", ""));
	dbport = config.ReadInteger("db", "DB_PORT", 0); 
	strcpy(dbname, config.ReadString("db", "DB_NAME", ""));
	strcpy(dbuser, config.ReadString("db", "DB_USER", ""));
	strcpy(dbpwd, config.ReadString("db", "DB_PWD", ""));
	tcpport = config.ReadInteger("db", "TCP_PORT", 502);
	udpport = config.ReadInteger("db", "UDP_PORT", 8000);
	record_channel_num_flag = config.ReadInteger("db", "RECORD_CHANNEL_NUM_FLAG", 0);
#endif
}


//查询网关配置，并保存到相应的对象中
void  init_gateway_object_list(queue_type& m_queue){
	PR_DEBUG();

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


//从数据库中读出网关的配置，并保存到链表中 
int  init_gateway_conf(){
	PR_DEBUG();

	if (db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60) == -1){				
		log_output("main.cpp  ConnectDB() failed\n");
		std::cout<<"main.cpp  connect postgresDB Failed!"<< std::endl;
		db.DisConnectDB();
		return -1;
	}

	int i,t,s,k;
	DBresult *res;                 

	res = db.SelectData("select * from gateway_conf;");

	i = db.DBntuples(res);
	/**取得查询的结果的记录的数量*/ 
	t = db.DBnfields(res);
	/**取得字段数量*/

#ifdef	GATEWAY_LIMIT
	if(i>GATEWAY_LIMIT)
		i=GATEWAY_LIMIT;
#endif
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
		p_gateway_conf->pool_interval=60*(atoi(db.DBgetvalue(res,s,4)));
		p_gateway_conf->timeout=atoi(db.DBgetvalue(res,s,5));

		gateway_conf_list.push_back(*p_gateway_conf);
	}
	db.DBclear(res);
	db.DisConnectDB();

	return 0;
}



//启动客户端，连接到网关服务器
void start_client()
{
	PR_DEBUG();
	list<class  Gateway* >::iterator theIterator;
	for( theIterator = gateway_object_list.begin(); theIterator != gateway_object_list.end(); theIterator++ ){
		cout << __LINE__<<endl;
		(*theIterator)->run_client();
	}
	cout << __LINE__<<endl;
}

