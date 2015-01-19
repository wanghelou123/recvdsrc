#include "DataProcess.h"
#include "log.h"
#include <boost/timer.hpp>

extern int record_channel_num_flag;
bool handle_msg(tcp_message_ptr& p)
{
	if(p == NULL)return true;
	
	GatewayDB db;
	int channelNum=1;	//通道编号
	int dataName;       //通道的名称
	float dataValue;    //通道的值
	int i = 0; int j = 0;
	int record_flag=0;


	if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
		printf("%s connect db fialed\n", __FUNCTION__);
		return true;
	}

	char strTime[32] = {0};
	if(0x03 == p->data[7]){
		time_t timenow = time(NULL);
		strftime(strTime, 32, "%Y-%m-%d %H:%M:%S", localtime(&timenow));
	}
	if(0x60 == p->data[7]){
		if(p->data[p->size - 4]==0xF2){
			unsigned char tmp[32];
			memcpy((void*)tmp, (void*)(p->data+p->size-16), 16);
			memcpy((void*)(p->data+p->size-16), (void*)(tmp+12), 4);
			memcpy((void*)(p->data+p->size-4), (void*)tmp, 12);
		} 

		p->size = p->size - 12;//把时间字段的长度去掉
		int i=0;
		struct tm pst_time;
		for(i=p->size; i<(p->size+12); i+=4){
			switch (p->data[i])
			{
			case 0xF3:
				pst_time.tm_year = (p->data[i+2]>70 ? p->data[i+2]+1900 : p->data[i+2]+2000) -1900;
				pst_time.tm_mon = p->data[i+3] - 1;
				break;

			case 0xF4:
				pst_time.tm_mday = p->data[i+2];
				pst_time.tm_hour = p->data[i+3];
				break;
			case 0xF5:
				pst_time.tm_min = p->data[i+2];
				pst_time.tm_sec = p->data[i+3];
				break;
			}		
		}
		memset(strTime,'\0',sizeof(strTime));
		strftime(strTime, 32, "%Y-%m-%d %H:%M:%S", &pst_time);	

	}

	for(i=0,j=9;i<32 && j<p->size;i++,j+=4){//解析每路数据
		dataName=0; dataValue = 0.0;
		int data2High; int data2Low; int data2Byte; int data4Byte ;
		dataName=(int)p->data[j];
		if(0 == dataName)break;//通首代码为0，直接跟到下一路

		if(p->data[j+1] & (1<<6)){//数据是开关量
			data2Byte = (p->data[j+2]<<8) + p->data[j+3] ;
			if(data2Byte){
				dataValue=1;
			}else{
				dataValue=0;
			}
		}else{ //数据是数值	//判断是几个字节
				if(p->data[j+1] & (1<<5)){//数据4个字节		
					if(p->data[j+1] & (1<<4)){//高2个字节
						data2High = (p->data[j+2]<<8) + p->data[j+3] ;
						data2Low  = (p->data[j+6]<<8) + p->data[j+7] ;
					}else{//低2个字节
						data2Low = (p->data[j+2]<<8) + p->data[j+3] ;
						data2High = (p->data[j+6]<<8) + p->data[j+7] ;
					}
					data4Byte = (data2High<<16)+data2Low ;
					//判断是正数还是正负数
					float a4Byte;
					if(p->data[j+1] & (1<<7)){//有符号正负数
						int tmpS = (int)data4Byte;
						a4Byte = (float)tmpS;
						}else a4Byte = (float)data4Byte;//无符号正数
					int n_4;						
					if(p->data[j+1] & 0x07){
						n_4 = p->data[j+1] & 0x07;
						int div = 1;
						int mm=0;
						for(mm=1;mm<=n_4;mm++) div=div*10;
						a4Byte = a4Byte/div;
						}
					dataValue = a4Byte;
					j+=4;

					}else{//数据2个字节		

						data2Byte = (p->data[j+2]<<8) + p->data[j+3] ;
						float a2Byte;
						if(p->data[j+1] & (1<<7)){//判断是正数还是正负数 //有符号正负数
							short tmpS;
							tmpS = (short)data2Byte;
							a2Byte = (float)tmpS;
							}else a2Byte = (float)data2Byte;	//无符号正数		
						int n_2;			
						if(p->data[j+1] & 0x07){//判断有没有小数 //有小数
							n_2 = p->data[j+1] & 0x07;//小数位数
							int div = 1; int m = 1;
							for(m=1;m<=n_2;m++) div=div*10;
							a2Byte = a2Byte/div;					
							} 
						dataValue = a2Byte;
					}				
			}
			
		char sql[512] = {0};
		memset(sql, '\0', sizeof(sql));

		try{


/*
#ifdef ODBC
			snprintf(sql, sizeof(sql) - 1, "UPDATE data set data_time = '%s', value=%.3f, up_state = %d where  gateway_logo='%s' and sensor_name=%d and channel_name=%d;", strTime, dataValue, 0, p->gateway_id,p->data[6], dataName);
#else
			snprintf(sql, sizeof(sql) - 1, "UPDATE data set data_time = to_timestamp('%s', 'YYYY-MM-DD HH24:MI:SS'), value=%.3f, up_state = %d where  gateway_logo='%s' and sensor_name=%d and channel_name=%d;", strTime, dataValue, 0, p->gateway_id,p->data[6], dataName[j]);
#endif

			if(db.UpdateData(sql)<0){
				memset(sql, '\0', sizeof(sql));

#ifdef DEBUG
				printf("insert %d,%d\n",p->data[6],dataName);
#endif 
		*/
			

			if(record_flag == 0){
				snprintf(sql, sizeof(sql) - 1, "delete from  data  where  gateway_logo='%s' and sensor_name=%d ;", p->gateway_id,p->data[6]);
				record_flag=1;
				db.DeleteData(sql);
			}
			if(record_channel_num_flag){//有通道编号字段
#ifdef ODBC 
				snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_num, channel_name, value, up_state) VALUES ('%s', '%s', %d, %d, %d, %.3f, %d);", strTime, p->gateway_id,  p->data[6], channelNum, dataName, dataValue, 0);				
#else
				snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_num, channel_name, value, up_state) VALUES (to_timestamp('%s', 'YYYY-MM-DD HH24:MI:SS'), '%s', %d, %d, %d, %.3f, %d);", strTime, p->gateway_id,  p->data[6], channelNum, dataName, dataValue, 0);
#endif
			}else{	//没有通道编号字段
#ifdef ODBC 
				snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_name, value, up_state) VALUES ('%s', '%s', %d, %d, %.3f, %d);", strTime, p->gateway_id,  p->data[6], dataName, dataValue, 0);				
#else
				snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_name, value, up_state) VALUES (to_timestamp('%s', 'YYYY-MM-DD HH24:MI:SS'), '%s', %d, %d, %.3f, %d);", strTime, p->gateway_id,  p->data[6], dataName, dataValue, 0);
#endif
			}
				//log_output("%s\n", sql);
				db.InsertData(sql);

			}catch(...){

			}	

			channelNum++;//通道编号递增
		}//for() end
		db.DisConnectDB();

		return true;
}