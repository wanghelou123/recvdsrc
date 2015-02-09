#include <string.h>
#include <stdlib.h>
#include <list>
#include "Log.h"
#include "AlterConf.h"
#include "io_service_pool.cpp"

extern list<class Gateway* > gateway_object_list;
extern list<struct gateway_conf> gateway_conf_list;
extern io_service_pool io_service_pool_;
extern int polling_unit;

AlterConf::AlterConf(boost::asio::io_service& io){


}

int AlterConf::AddGateway(char * tmp_gateway_logo, queue_type& m_queue)
{
	DEBUG(__func__ << "gateway_logo:"<< tmp_gateway_logo);
	char gateway_logo[50];
	strcpy(gateway_logo,tmp_gateway_logo);
	int i,t,k;
	GatewayDB db;
	DBresult *res;
	char sql[4096] = {0};
	snprintf(sql, sizeof(sql) - 1, "select * from gateway_conf where gateway_logo='%s';", gateway_logo);  
	if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
		FATAL("Connect database failed!");
		return -1;		
	}
	res = db.SelectData(sql);
	/**取得查询的结果的记录的数量*/ 
	i = db.DBntuples(res);
	/**取得字段数量*/
	t = db.DBnfields(res);
	if(1 != i){
		WARNING("gateway_conf table has not the gateway:"<< gateway_logo);
		db.DisConnectDB();
		return -1;
	}


	Gateway *p_gateway=NULL;
	char gateway_id[50];
	memset(gateway_id, 0, sizeof(gateway_id));
	strcpy(gateway_id, db.DBgetvalue(res,0,3));
	for(k=0;k<50;k++){
		if(gateway_id[k]==0x20)
			gateway_id[k] = 0x00;
	}

	list<struct gateway_conf>::iterator theIterator;
	for( theIterator = gateway_conf_list.begin(); theIterator != gateway_conf_list.end(); theIterator++ )
		if(0 == strcmp(gateway_logo,theIterator->gateway_id))break;


	if(theIterator != gateway_conf_list.end()){//如果网关链表是已经存在了该网关
		FATAL("Add Gateway error, list already has the gatewayway:"<< gateway_id);
		return -1;
	}

	struct gateway_conf  *p_gateway_conf=new struct gateway_conf;

	p_gateway_conf->work_mode = atoi(db.DBgetvalue(res,0,1));
	strcpy(p_gateway_conf->gateway_id,gateway_id);
	p_gateway_conf->port = atoi(db.DBgetvalue(res,0,2));
	if(0 == polling_unit) {
		p_gateway_conf->pool_interval=60*(atoi(db.DBgetvalue(res,0,4)));
	}else {
		p_gateway_conf->pool_interval=atoi(db.DBgetvalue(res,0,4));
	}
	p_gateway_conf->timeout=atoi(db.DBgetvalue(res,0,5));

	/*将新网关配置添加到网关配置链表*/
	gateway_conf_list.push_back(*p_gateway_conf);

	db.DBclear(res);
	db.DisConnectDB();

	if(0 == p_gateway_conf->work_mode){
		p_gateway = new Gateway(io_service_pool_.get_io_service(), m_queue);
		p_gateway->work_mode = p_gateway_conf->work_mode;
		strcpy(p_gateway->gateway_id,p_gateway_conf->gateway_id);
		p_gateway->port = p_gateway_conf->port;
		p_gateway->pool_interval=p_gateway_conf->pool_interval;
		p_gateway->timeout=p_gateway_conf->timeout;
		gateway_object_list.push_back(p_gateway);
		p_gateway->run_client();
		NOTICE("new gateway:"<< p_gateway->gateway_id << " start!");
	}
	NOTICE("Add the gateway:"<<gateway_id << " sucess!");

	return 0;
}

int	AlterConf::ModiGateway(char * tmp_gateway_logo)
{
	DEBUG(__func__ <<" gateway_logo:"<< tmp_gateway_logo);
	char gateway_logo[50];
	strcpy(gateway_logo,tmp_gateway_logo);

	list<struct gateway_conf>::iterator theIterator;
	for( theIterator = gateway_conf_list.begin(); theIterator != gateway_conf_list.end(); theIterator++ )
		if(0 == strcmp(gateway_logo,theIterator->gateway_id))break;
	if(theIterator == gateway_conf_list.end()){
		FATAL("Not has the gateway:"<< gateway_logo << " Modify error!");
		return -1;
	}

	int i,t;
	GatewayDB db;
	DBresult *res;
	char sql[4096] = {0};
	snprintf(sql, sizeof(sql) - 1, "select * from gateway_conf where gateway_logo='%s';", gateway_logo);  
	if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
		FATAL("Connect database error!");
		return -1;		
	}
	res = db.SelectData(sql);
	/**取得查询的结果的记录的数量*/ 
	i = db.DBntuples(res);
	/**取得字段数量*/
	t = db.DBnfields(res);

	if(1 != i){
		WARNING("gateway_conf table has not the gateway:"<< gateway_logo);
		db.DisConnectDB();
		return -1;
	}

	theIterator->port=atoi(db.DBgetvalue(res,0,2));
	theIterator->pool_interval=60*(atoi(db.DBgetvalue(res,0,4)));
	theIterator->timeout=atoi(db.DBgetvalue(res,0,5));


	list<class  Gateway* >::iterator theIterator2;
	for( theIterator2 = gateway_object_list.begin(); theIterator2 != gateway_object_list.end(); (theIterator2)++ ){
		if(0 == strcmp(theIterator->gateway_id, (*theIterator2)->gateway_id))break;
	}
	if(theIterator2 == gateway_object_list.end()){//没有该网关的配置
		return -1;
	}else{
		(*theIterator2)->port=theIterator->port;
		(*theIterator2)->pool_interval=theIterator->pool_interval;
		(*theIterator2)->timeout=theIterator->timeout;
	}
	NOTICE("Moidfy the gateway:"<<gateway_logo << " sucess!");

	return 0;
}

int AlterConf::DelGateway(char * tmp_gateway_logo)
{
	DEBUG(__func__);
	char gateway_logo[50];
	strcpy(gateway_logo,tmp_gateway_logo);


	/*网关对象表*/
	list<class  Gateway* >::iterator theIterator;
	for( theIterator = gateway_object_list.begin(); theIterator != gateway_object_list.end(); (theIterator)++ ){
		if(0 == strcmp(gateway_logo, (*theIterator)->gateway_id))break;
	}
	if(theIterator != gateway_object_list.end()){//网关对象链表中有该网关
		delete	*theIterator;
		/*在链表中删除该对象*/
		gateway_object_list.remove(*theIterator);
	}

	/*网关配置表*/
	list<struct gateway_conf>::iterator  theIterator2;
	for( theIterator2 = gateway_conf_list.begin(); theIterator2 != gateway_conf_list.end(); theIterator2++ ){
		if(0 == strcmp(gateway_logo,theIterator2->gateway_id))break;
	}
	if(theIterator2 != gateway_conf_list.end()){
		gateway_conf_list.erase(theIterator2);
	}else{
		FATAL("Detele error!, not ahs the gateway:"<< gateway_logo);
		return -1;
	}
	NOTICE("Delete the gateway:"<<gateway_logo << " sucess!");

	return 0;
} 



