#include "AlterConf.h"
#include <string.h>
#include <stdlib.h>
#include "log.h"
#include "log.h"
#include  "debug.h"
#include "io_service_pool.cpp"
#include <list>

extern list<class Gateway* > gateway_object_list;
extern list<struct gateway_conf> gateway_conf_list;
extern io_service_pool io_service_pool_;

AlterConf::AlterConf(boost::asio::io_service& io){


}

int AlterConf::AddGateway(char * tmp_gateway_logo, queue_type& m_queue)
{
#ifdef DEBUG
cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
printf("gateway_logo = %s\n", tmp_gateway_logo);
#endif
	char gateway_logo[50];
	strcpy(gateway_logo,tmp_gateway_logo);
	int i,t,k;
	GatewayDB db;
	DBresult *res;
	char sql[4096] = {0};
	snprintf(sql, sizeof(sql) - 1, "select * from gateway_conf where gateway_logo='%s';", gateway_logo);  
	if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
		cout<< __FILE__ << __FUNCTION__ << "ConnectDB() failed" << endl;
		return -1;		
	}
	res = db.SelectData(sql);
	/**取得查询的结果的记录的数量*/ 
	i = db.DBntuples(res);
	/**取得字段数量*/
	t = db.DBnfields(res);
	if(1 != i){
#ifdef DEBUG
		printf("gateway_conf table has not the gateway:	%s\n", gateway_logo);
#endif
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
#ifdef DEBUG
		printf("%d:%s:%s:Already has the gateway:%s\n",__LINE__, __FILE__, __FUNCTION__, gateway_id);
#endif
		log_output("Add Gateway error %d:%s:%s:Already has the gateway:%s\n",__LINE__, __FILE__, __FUNCTION__, gateway_id);
		return -1;
	}

	struct gateway_conf  *p_gateway_conf=new struct gateway_conf;
	
	p_gateway_conf->work_mode = atoi(db.DBgetvalue(res,0,1));
	strcpy(p_gateway_conf->gateway_id,gateway_id);
	p_gateway_conf->port = atoi(db.DBgetvalue(res,0,2));
	p_gateway_conf->pool_interval=60*(atoi(db.DBgetvalue(res,0,4)));
	p_gateway_conf->timeout=atoi(db.DBgetvalue(res,0,5));

	/*将新网关配置添加到网关配置链表*/
	gateway_conf_list.push_back(*p_gateway_conf);

	db.DBclear(res);
	db.DisConnectDB();

	if(0 == p_gateway_conf->work_mode){
		printf("%d:%s:%s:Already has the gateway:%s\n",__LINE__, __FILE__, __FUNCTION__, gateway_id);

		p_gateway = new Gateway(io_service_pool_.get_io_service(), m_queue);
		p_gateway->work_mode = p_gateway_conf->work_mode;
		strcpy(p_gateway->gateway_id,p_gateway_conf->gateway_id);
		p_gateway->port = p_gateway_conf->port;
		p_gateway->pool_interval=p_gateway_conf->pool_interval;
		p_gateway->timeout=p_gateway_conf->timeout;
		gateway_object_list.push_back(p_gateway);
		p_gateway->run_client();
#ifdef DEBUG
		printf("new gateway:	%s	start\n", p_gateway->gateway_id);
#endif
	}
	printf("%d:%s:%s:Add has the gateway:%s success\n",__LINE__, __FILE__, __FUNCTION__, gateway_id);

	
	log_output("%d:%s:%s:Add the gateway:%s success\n",__LINE__, __FILE__, __FUNCTION__, gateway_id);

	return 0;
}

int	AlterConf::ModiGateway(char * tmp_gateway_logo)
{
#ifdef DEBUG
cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
printf("gateway_logo=%s\n",tmp_gateway_logo );
#endif
		char gateway_logo[50];
		strcpy(gateway_logo,tmp_gateway_logo);

		list<struct gateway_conf>::iterator theIterator;
		for( theIterator = gateway_conf_list.begin(); theIterator != gateway_conf_list.end(); theIterator++ )
			if(0 == strcmp(gateway_logo,theIterator->gateway_id))break;
		if(theIterator == gateway_conf_list.end()){
#ifdef DEBUG
			cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif			
			log_output("%d:%s:%s:Not has the gateway:%s,Modify error!\n",__LINE__, __FILE__, __FUNCTION__, gateway_logo);

			return -1;
		}

		int i,t;
		GatewayDB db;
		DBresult *res;
		char sql[4096] = {0};
		snprintf(sql, sizeof(sql) - 1, "select * from gateway_conf where gateway_logo='%s';", gateway_logo);  
		if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
			cout<< __FILE__ << __FUNCTION__ << "ConnectDB() failed" << endl;
			return -1;		
		}
		res = db.SelectData(sql);
		/**取得查询的结果的记录的数量*/ 
		i = db.DBntuples(res);
		/**取得字段数量*/
		t = db.DBnfields(res);

		if(1 != i){
#ifdef DEBUG
			printf("gateway_conf table has not the gateway:	%s\n", gateway_logo);
#endif			
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
		log_output("%d:%s:%s:the gateway:%s,Modify ok!\n",__LINE__, __FILE__, __FUNCTION__, gateway_logo);

	return 0;
}

int AlterConf::DelGateway(char * tmp_gateway_logo)
{
#ifdef DEBUG
cout << __FILE__<< "	" << __LINE__  << "	" << __FUNCTION__ <<endl;
#endif
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

		#ifdef DEBUG		
			printf("%d:%s:%s:Not has the gateway:%s,Delete error!",__LINE__, __FILE__, __FUNCTION__, gateway_logo);
		#endif	
			log_output("%d:%s:%s:Not has the gateway:%s,Delete error!\n",__LINE__, __FILE__, __FUNCTION__, gateway_logo);

			return -1;
	}
	
#ifdef DEBUG	
	printf("%d:%s:%s:Delete the gateway:%s  success!\n",__LINE__, __FILE__, __FUNCTION__, gateway_logo);
#endif
	log_output("%d:%s:%s:Delete the gateway:%s  success!\n",__LINE__, __FILE__, __FUNCTION__, gateway_logo);

	return 0;
} 



