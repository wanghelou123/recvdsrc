#ifndef MAIN_H_
#define MAIN_H_
#include "Gateway.h"
using namespace std;
using namespace boost;

list<struct gateway_conf> gateway_conf_list;
list<class Gateway* > gateway_object_list;
static GatewayDB db;

//数据加接时需要的变量
int log_level;
char dbaddr[50];
int dbport ; 
char dbname[50];
char dbuser[50];
char dbpwd[50];
char dbtype[50];
int	tcpport;
int	udpport;
int record_channel_num_flag;

void init_db_conf();
void init_gateway_object_list(queue_type& m_queue);
int  init_gateway_conf();
void start_client();


//创建200个线程
io_service_pool io_service_pool_(30);
#endif
