#include <string.h>
#include <stdlib.h>
#include <list>
#include "WriteCmd.h"
#include "pgdb.h"
#include "Log.h"

extern list<class Gateway* > gateway_object_list;
WriteCmd::WriteCmd(boost::asio::io_service& io)
{
	async_timer = new boost::asio::deadline_timer( io, posix_time::seconds(1) );
	//async_timer->async_wait(bind(&WriteCmd::GetCmdFromDB, this, asio::placeholders::error,async_timer));
	//async_timer->async_wait(bind(&WriteCmd::GetCmdFromDB, this));
}


WriteCmd::~WriteCmd()
{
}

void WriteCmd::Timer(const boost::system::error_code& ec, timer_pt async_timer)
{
	if(ec)  
	{
		cout << "timer error." << boost::system::system_error(ec).what() <<endl;
		return;  
	}
	//	GetCmdFromDB("write_table");
	//	async_timer->expires_at(async_timer->expires_at()+posix_time::seconds( 10 ));
	//	async_timer->async_wait(bind(&WriteCmd::Timer, this, asio::placeholders::error, async_timer));

}


void   WriteCmd::GetCmdFromDB( )
{
	if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
		FATAL("connect Database failed");
		return ;		
	}

	Gateway * p_gateway=NULL;
	char gateway_id[50];
	char gateway_id_bak[50];
	char data_val[150];
	int i,k,s,t;	
	DBresult *res = NULL;
	char sql[4096] = {0};
	char strTime[32] = {0};
	time_t timenow = time(NULL);
	strftime(strTime, 32, "%Y-%m-%d %H:%M:%S", localtime(&timenow));

	snprintf(sql, sizeof(sql) - 1, "SELECT * FROM  write_table where cmd_status=0 OR \
			cmd_status=2 ORDER BY gateway_logo, cmd_status DESC;");	

	memset(gateway_id_bak, '\0', sizeof(gateway_id_bak));				
	res = db.SelectData(sql);

	if(res == NULL) {
		FATAL( "execute sql failed.");		
		db.DisConnectDB();
		return;
	}

	i = db.DBntuples(res);
	t = db.DBnfields(res);

	if(0 == i){
		WARNING("No Command require write!");
		db.DBclear(res);
		db.DisConnectDB();//断开数据库的连接，直接返回不再注册时器了
		return ;	
	}


	for(s=0; s<i;s++){
		memset(gateway_id, '\0', sizeof(gateway_id));
		strcpy(gateway_id, db.DBgetvalue(res,s,1));
		for(k=0;k<50;k++){
			if(gateway_id[k]==0x20)
				gateway_id[k] = '\0';
		}				

		list<class  Gateway* >::iterator theIterator;
		for( theIterator = gateway_object_list.begin(); theIterator != gateway_object_list.end(); (theIterator)++ ){
			if(0 == strcmp(gateway_id, (*theIterator)->gateway_id))break;
		}
		if(theIterator == gateway_object_list.end()){//没有该网关的配置
			UpdateCmdStatus(atoi(db.DBgetvalue(res,s,0)), 1, 3);//如果网关断网，刚更新数据库为超时2013-7-30whl
			WARNING("cant not find the gateway:"<< gateway_id);
			//return;
			continue;//modified by whl 2013-7-30
		}else{
			DEBUG("find the gateway:"<< gateway_id);
			p_gateway=*theIterator;
		}

		//如果命令处于下发状态，判断是否超时
		if(atoi(db.DBgetvalue(res,s,5))==2){
			char str_time[50];
			strcpy(str_time,db.DBgetvalue(res,s,8));
			double i_sec=0;
			int n_year, n_month, n_day, n_hour, n_min, n_sec;
			sscanf(str_time,"%d-%d-%d   %d:%d:%d",   &n_year,&n_month,&n_day,&n_hour,&n_min,&n_sec);
			struct tm t;
			time_t record_time,cur_time;
			t.tm_year=n_year-1900;
			t.tm_mon=n_month-1;
			t.tm_mday=n_day;
			t.tm_hour=n_hour;
			t.tm_min=n_min;
			t.tm_sec=n_sec;
			t.tm_isdst=0;
			record_time=mktime(&t);
			time(&cur_time);
			i_sec=difftime( cur_time, record_time);
			if(i_sec >= 20){
				UpdateCmdStatus(atoi(db.DBgetvalue(res,s,0)), 1, 3);
				p_gateway->sn=0;
				memset(p_gateway->write_buff,'\0',sizeof(p_gateway->write_buff));	
				p_gateway->write_flag=0;		
				WARNING("write cmd to gateway timeout!");
				db.DBclear(res);
				db.DisConnectDB();

				return ;
			}

		}


		//如果是同一个网关，则直接指向下一条记录
		if(!strcmp(gateway_id_bak, gateway_id)){
			strcpy(gateway_id_bak, gateway_id);	
			continue;
		}else{
			strcpy(gateway_id_bak, gateway_id);	
		}		

		if(0 == p_gateway->write_flag){
			p_gateway->write_buff[0]=0x84;//事务处理标识符 –由服务器复制 –通常为 0
			p_gateway->write_buff[1]=0x95;//事务处理标识符 –由服务器复制 –通常为 0
			p_gateway->write_buff[2]=0x00;//协议标识符= 0
			p_gateway->write_buff[3]=0x00;//协议标识符= 0
			p_gateway->write_buff[6]=atoi(db.DBgetvalue(res,s,2));//单元标识符 
			p_gateway->write_buff[7]=0x10;//功能码
			p_gateway->write_buff[8]=(atoi(db.DBgetvalue(res,s,3)) & 0xFF00)>>8;//起始地址高位
			p_gateway->write_buff[9]=(atoi(db.DBgetvalue(res,s,3)) & 0x00FF);//起始地址低位


			//存放数值
			char str_temp[3];
			int n = 0;
			strcpy(data_val,db.DBgetvalue(res,s,4));
			while(data_val[n]!=0x20 && data_val[n]!='\0'){
				str_temp[0]=data_val[n*2+0];
				str_temp[1]=data_val[n*2+1];
				str_temp[2]='\0';
				p_gateway->write_buff[13+n]=strtol(str_temp,NULL,16);
				n++;
			}
			p_gateway->write_buff[4]=((n/2+7)&0xff00)>>8;//长度
			p_gateway->write_buff[5]=(n/2+7)&0x00ff;
			p_gateway->write_buff[10]= ((n/4) & 0xff00) >> 8;//寄存器数量
			p_gateway->write_buff[11]= ((n/4) & 0x00ff);
			p_gateway->write_buff[12]= n/2;				//字节个数

			//置网关写命令标志位为1							
			p_gateway->sn = atoi(db.DBgetvalue(res,s,0));
			p_gateway->write_flag = 1;

			/* 如果网关线程处理定时睡眠状态 则取消定是器 */
			if(p_gateway->timer_flag == 1){
				p_gateway->async_timer->cancel();
				time_t	time_cur;
				time(&time_cur);
				p_gateway->i_sec = difftime(time_cur, p_gateway->time_start);
				DEBUG(__func__<<" i_sec="<< (int)p_gateway->i_sec);
			}

			/* 更新write_table 表中命令状态为2，表示正在处理*/
			UpdateCmdStatus( atoi(db.DBgetvalue(res, s, 0)), 2, 0);
		}		
	}	

	db.DBclear(res);
	db.DisConnectDB();

	async_timer->expires_from_now(posix_time::seconds( 1 ) );
	async_timer->async_wait(bind(&WriteCmd::GetCmdFromDB, this));

	DEBUG(__func__);

	return ;
}



int WriteCmd::UpdateCmdStatus(int sn, int cmd_status, int reval)
{
	DEBUG(__func__);
	GatewayDB db;
	char sql[4096] = {0};
	char strTime[32] = {0}; 
	time_t timenow = time(NULL);
	strftime(strTime, 32, "%Y-%m-%d %H:%M:%S", localtime(&timenow));

	snprintf(sql, sizeof(sql) - 1, "UPDATE write_table SET cmd_status=%d, reval=%d, update_time = '%s' WHERE sn=%d;", \
			cmd_status, reval, strTime, sn); 

	if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
		FATAL("connect database failed!");
		return 0;		
	}

	db.UpdateData(sql); 

	db.DisConnectDB();

	return  0;
}

/*给网关发送对时命令*/

void   WriteCmd::correctTime( )
{
	if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
		FATAL("connect database failed!");
		return ;		
	}

	Gateway * p_gateway=NULL;
	char gateway_id[50];
	char gateway_id_bak[50];
	int i,k,s,t;	
	DBresult *res = NULL;
	char sql[4096] = {0};
	snprintf(sql, sizeof(sql) - 1, "SELECT * FROM  write_table where cmd_status=0 OR cmd_status=2 ORDER BY \
			gateway_logo, cmd_status DESC;");	

		memset(gateway_id_bak, '\0', sizeof(gateway_id_bak));				
	res = db.SelectData(sql);
	i = db.DBntuples(res);
	t = db.DBnfields(res);

	if(0 == i){
		WARNING(" No Command require write");
		db.DBclear(res);
		db.DisConnectDB();//断开数据库的连接，直接返回不再注册时器了
		return ;	
	}
	for(s=0; s<i;s++){
		memset(gateway_id, '\0', sizeof(gateway_id));
		strcpy(gateway_id, db.DBgetvalue(res,s,1));
		for(k=0;k<50;k++){
			if(gateway_id[k]==0x20)
				gateway_id[k] = '\0';
		}				

		list<class  Gateway* >::iterator theIterator;
		for( theIterator = gateway_object_list.begin(); theIterator != gateway_object_list.end(); (theIterator)++ ){
			if(0 == strcmp(gateway_id, (*theIterator)->gateway_id))break;
		}
		if(theIterator == gateway_object_list.end()){//没有该网关的配置
			UpdateCmdStatus(atoi(db.DBgetvalue(res,s,0)), 1, 3);//如果网关断网，刚更新数据库为超时2013-7-30whl
			WARNING("cant not find the gateway:"<< gateway_id);
			continue;//modified by whl 2013-7-30
		}else{
			DEBUG("find the gateway:"<< gateway_id);
			p_gateway=*theIterator;

		}

		p_gateway->sn = atoi(db.DBgetvalue(res,s,0));

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
			ret = p_gateway->sockfd->write_some(asio::buffer(send_buf,25));

		}
		catch(...){
			FATAL("write_some() error!");
			UpdateCmdStatus( atoi(db.DBgetvalue(res, s, 0)), 1, 2);	
			return;

		}

		/* 更新write_table 表中命令状态为2，表示正在处理*/
		UpdateCmdStatus( atoi(db.DBgetvalue(res, s, 0)), 2, 0);	
	}	

	db.DBclear(res);
	db.DisConnectDB();

	return ;
}
