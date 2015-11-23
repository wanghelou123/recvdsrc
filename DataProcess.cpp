#include "DataProcess.h"
#include "Log.h"
#include <boost/timer.hpp>

extern int record_channel_num_flag;
#ifndef LIGHTSYS
bool handle_msg(tcp_message_ptr& p)
{
	if(p == NULL)return true;
	
	GatewayDB db;
	int channelNum=1;	//ͨ�����
	int dataName;       //ͨ��������
	float dataValue;    //ͨ����ֵ
	int i = 0; int j = 0;
	int record_flag=0;


	if(-1 == db.ConnectDB(dbaddr, dbport, dbname, dbuser, dbpwd, 60)){
		FATAL("connect database failed!");
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

		p->size = p->size - 12;//��ʱ���ֶεĳ���ȥ��
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

	for(i=0,j=9;i<32 && j<p->size;i++,j+=4){//����ÿ·����
		dataName=0; dataValue = 0.0;
		int data2High; int data2Low; int data2Byte; int data4Byte ;
		dataName=(int)p->data[j];
		if(0 == dataName)break;//ͨ�״���Ϊ0��ֱ�Ӹ�����һ·

		if(p->data[j+1] & (1<<6)){//�����ǿ�����
			data2Byte = (p->data[j+2]<<8) + p->data[j+3] ;
			if(data2Byte){
				dataValue=1;
			}else{
				dataValue=0;
			}
		}else{ //��������ֵ	//�ж��Ǽ����ֽ�
				if(p->data[j+1] & (1<<5)){//����4���ֽ�		
					if(p->data[j+1] & (1<<4)){//��2���ֽ�
						data2High = (p->data[j+2]<<8) + p->data[j+3] ;
						data2Low  = (p->data[j+6]<<8) + p->data[j+7] ;
					}else{//��2���ֽ�
						data2Low = (p->data[j+2]<<8) + p->data[j+3] ;
						data2High = (p->data[j+6]<<8) + p->data[j+7] ;
					}
					data4Byte = (data2High<<16)+data2Low ;
					//�ж�����������������
					float a4Byte;
					if(p->data[j+1] & (1<<7)){//�з���������
						int tmpS = (int)data4Byte;
						a4Byte = (float)tmpS;
						}else a4Byte = (float)data4Byte;//�޷�������
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

					}else{//����2���ֽ�		

						data2Byte = (p->data[j+2]<<8) + p->data[j+3] ;
						float a2Byte;
						if(p->data[j+1] & (1<<7)){//�ж����������������� //�з���������
							short tmpS;
							tmpS = (short)data2Byte;
							a2Byte = (float)tmpS;
							}else a2Byte = (float)data2Byte;	//�޷�������		
						int n_2;			
						if(p->data[j+1] & 0x07){//�ж���û��С�� //��С��
							n_2 = p->data[j+1] & 0x07;//С��λ��
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
			if(record_flag == 0){
				snprintf(sql, sizeof(sql) - 1, "delete from  data  where  gateway_logo='%s' and sensor_name=%d ;", p->gateway_id,p->data[6]);
				record_flag=1;
				db.DeleteData(sql);
			}
			if(record_channel_num_flag){//��ͨ������ֶ�
#ifdef ODBC 
				snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_num, channel_name, value, up_state) VALUES ('%s', '%s', %d, %d, %d, %.3f, %d);", strTime, p->gateway_id,  p->data[6], channelNum, dataName, dataValue, 0);				
#else
				snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_num, channel_name, value, up_state) VALUES (to_timestamp('%s', 'YYYY-MM-DD HH24:MI:SS'), '%s', %d, %d, %d, %.3f, %d);", strTime, p->gateway_id,  p->data[6], channelNum, dataName, dataValue, 0);
#endif
			}else{	//û��ͨ������ֶ�
#ifdef ODBC 
				snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_name, value, up_state) VALUES ('%s', '%s', %d, %d, %.3f, %d);", strTime, p->gateway_id,  p->data[6], dataName, dataValue, 0);				
#else
				snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_name, value, up_state) VALUES (to_timestamp('%s', 'YYYY-MM-DD HH24:MI:SS'), '%s', %d, %d, %.3f, %d);", strTime, p->gateway_id,  p->data[6], dataName, dataValue, 0);
#endif
			}
				db.InsertData(sql);

			}catch(...){

			}	

			channelNum++;//ͨ����ŵ���
		}//for() end
		db.DisConnectDB();

		return true;
}
#else
bool handle_msg(GatewayDB &db, tcp_message_ptr& p, bool is_delete)
{
	if(p == NULL)return true;
	
	int dataName;       //ͨ��������
	float dataValue;    //ͨ����ֵ
	int dataTemp;
	int i = 0;
	int record_flag=0;
	int channel_count=0;
	char sql1[2048] = {0};
	char sql2[2048] = {0};
	char sql3[256] = {0};
	char sql4[256] = {0};
	char tmp1[256] = {0};
	char tmp2[256] = {0};

	memcpy(&dataTemp, p->data+9, 4);
	if(p->data[10] == 0xa8){
		channel_count =8;
	}else if(p->data[10] == 0xa4){
		channel_count =4;
	}


	memset(sql1, '\0', sizeof(sql1));
	memset(sql2, '\0', sizeof(sql2));
	memset(sql3, '\0', sizeof(sql3));
	snprintf(sql1, sizeof(sql1) - 1, \
		"INSERT INTO data \
		(data_time, gateway_logo, sensor_name, channel_name, value, up_state) values ");
	snprintf(sql2, sizeof(sql2) - 1, \
		"INSERT INTO iot_history_%d\
		(data_time, gateway_logo, sensor_name, channel_name, value) values ", p->gw_ID);
	snprintf(sql4, sizeof(sql4) - 1, \
		"DELETE FROM  iot_history_%d WHERE id < (SELECT MAX(id) - 300000+1  FROM iot_history_%d);", p->gw_ID, p->gw_ID);
	
	for(i=0; i <channel_count; i++){//����ÿ·����
		dataName = 0xb0 | (i+1);
		dataValue = ((dataTemp&0xff) >> i) & 0x01;
		memset(tmp1, '\0', sizeof(tmp1));
		memset(tmp2, '\0', sizeof(tmp2));

		try{		
			if(record_flag == 0 && is_delete==true){
				snprintf(sql3, sizeof(sql3) - 1, \
				"delete from  data  where  gateway_logo='%s' and sensor_name=%d ;",\
						 p->gateway_id,p->data[6]);

				record_flag=1;
				db.DeleteData(sql3);
			}
			snprintf(tmp1, sizeof(tmp1) - 1, \ 
			"(to_timestamp('%s', 'YYYY-MM-DD HH24:MI:SS'), \
					'%s', %d, %d, %.3f, %d),", \
					p->data+13, p->gateway_id,  p->data[6], dataName, dataValue, 0);

			snprintf(tmp2, sizeof(tmp2) - 1, \ 
			"(to_timestamp('%s', 'YYYY-MM-DD HH24:MI:SS'), \
					'%s', %d, %d, %.3f),", \
					p->data+13, p->gateway_id,  p->data[6], dataName, dataValue);

			strcat(sql1, tmp1);
			strcat(sql2, tmp2);
		}catch(...){

		}	

			
		}//for() end
		
		sql1[strlen(sql1)-1]=';';
		sql2[strlen(sql2)-1]=';';
		//cout << "sql1 length = "<< strlen(sql1) << endl;
		//cout << "sql2 length = "<< strlen(sql2) << endl;
		//cout << sql1<<endl;
		//cout << sql2<<endl;

		db.InsertData(sql2);//������ʷ����
		if(is_delete == true) {
				db.InsertData(sql1);//����ʵʱ����
				//db.DeleteData(sql4);//ɾ���������Ƶ���ʷ����
		}

		return true;
}
#if 0
bool handle_msg(tcp_message_ptr& p, GatewayDB &db)
{
	if(p == NULL)return true;
	
	int dataName;       //ͨ��������
	float dataValue;    //ͨ����ֵ
	int dataTemp;
	int i = 0;
	int record_flag=0;
	int channel_count=0;
	char sql[512] = {0};

	memcpy(&dataTemp, p->data+9, 4);
	if(p->data[10] == 0xa8){
		channel_count =8;
	}else if(p->data[10] == 0xa4){
		channel_count =4;
	}


	
	for(i=0; i <channel_count; i++){//����ÿ·����
		dataName = 0xb0 | (i+1);
		dataValue = ((dataTemp&0xff) >> i) & 0x01;
		memset(sql, '\0', sizeof(sql));

		try{		
#if 0
			if(record_flag == 0){
				snprintf(sql, sizeof(sql) - 1, "delete from  data  where  gateway_logo='%s' and sensor_name=%d ;",\
						 p->gateway_id,p->data[6]);
				record_flag=1;
				db.DeleteData(sql);
			}
#endif
#ifdef ODBC 
			snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_name, value, up_state)\
										   VALUES ('%s', '%s', %d, %d, %.3f, %d);", p->data+13, p->gateway_id,  p->data[6], dataName, dataValue, 0);				
#else
			snprintf(sql, sizeof(sql) - 1, "INSERT INTO data (data_time, gateway_logo, sensor_name, channel_name, value, up_state) \
										   VALUES (to_timestamp('%s', 'YYYY-MM-DD HH24:MI:SS'), '%s', %d, %d, %.3f, %d);", p->data+13, p->gateway_id,  p->data[6], dataName, dataValue, 0);
#endif
					


			//db.InsertData(sql);
		}catch(...){

		}	

			
		}//for() end

		return true;
}
#endif
#endif
