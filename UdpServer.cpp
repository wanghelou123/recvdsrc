#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "UdpServer.h"
#include "pgdb.h"
#include "log.h"
#include "io_service_pool.cpp"
#include "debug.h"
#include <list>

using boost::asio::ip::udp;
using namespace std;

extern list<class Gateway* > gateway_object_list;
extern list<struct gateway_conf> gateway_conf_list;
extern io_service_pool io_service_pool_;

UdpServer::UdpServer(boost::asio::io_service& io_service, short port, queue_type& q)
: io_service_(io_service),m_queue(q),
socket_(io_service, udp::endpoint(udp::v4(), port)),sn(0),p_gateway(NULL)
{
#ifdef DEBUG
cout << __FILE__<< "	" << __FUNCTION__ <<endl;
cout <<"UdpServer start, port is:"<< port<<"."<<endl;
#endif

	memset(data_, '\0', sizeof(data_));
	socket_.async_receive_from(
		boost::asio::buffer(data_, max_length), sender_endpoint_,
		boost::bind(&UdpServer::handle_receive_from, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));

		p_alter_conf = new AlterConf(io_service_pool_.get_io_service());
		p_write_cmd = new WriteCmd( io_service_pool_.get_io_service() );
		async_timer = new asio::deadline_timer( io_service, posix_time::seconds(1) );

}

void UdpServer::handle_receive_from(const boost::system::error_code& error,
									size_t bytes_recvd)
{
	if (!error && bytes_recvd > 0)
	{
		socket_.async_send_to(
			boost::asio::buffer(data_, bytes_recvd), sender_endpoint_,
			boost::bind(&UdpServer::handle_send_to, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}

	socket_.async_receive_from(
		boost::asio::buffer(data_, max_length), sender_endpoint_,
		boost::bind(&UdpServer::handle_receive_from, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));

}

void UdpServer::handle_send_to(const boost::system::error_code& /*error*/,
							   size_t bytes_sent)
{
	GatewayDB db;
	cout << bytes_sent << " bytes received "<< endl;

	if(crc_check(data_, bytes_sent)==0){
		log_output("%d:%s:%s: CRC CHECK ERROR!\n", __LINE__, __FILE__, __FUNCTION__);
		return;
	}

	data_[bytes_sent-2]='\0';
	switch (data_[0]){
	case 0x01://�յ�֪ͨ��write_table����������Ҫ�·�	
		switch(data_[1]){
			case 0x01:
				log_output("write_table has write cmd\n");
				p_write_cmd->GetCmdFromDB();
				log_output("execute complete the write cmd\n");
				break;

			case 0x02:
				cout << "correction time" << endl;
				p_write_cmd->correctTime();
				break;
		}
		break;

	case 0x02:
		switch(data_[1]){
			/*����������*/
			case 0x01:{
				char *p_gateway_logo = ( char *)&data_[2];
				p_alter_conf->AddGateway(p_gateway_logo , m_queue);
				break;
			}
			/*�޸�����*/
			case 0x02:{
				char *p_gateway_logo = ( char *)&data_[2];
				p_alter_conf->ModiGateway(p_gateway_logo);
				break;
			}


			/*ɾ������*/
			case 0x03:{
				char *p_gateway_logo = ( char *)&data_[2];
				p_alter_conf->DelGateway(p_gateway_logo);
				break;
			}
		}
		break;	

	}
	
	memset(data_, '\0', sizeof(data_));
}

#if 0
void UdpServer::ConfNodeToNet(const boost::system::error_code& ec, timer_pt async_timer)
{
	cout << "CONF NODE TO NET TIMER" << endl;
	int pos = 0;
	char gateway_logo[50];
	GatewayDB db;
	DBresult *res = NULL;
	char sql[4096] = {0};
	
	while(1){
	if(sn == 0){
			if(-1 == db.MyconnectDB()){
				cout << "case 0x04 connect db failed!!!"<<endl;
				log_output("DelNodeFromNet():	Connect DB failed \n");
				return;

			}


			snprintf(sql, sizeof(sql) - 1, "SELECT * FROM  new_node where  status=1 limit 1;");
			res = db.SelectData(sql); 

			/**ȡ�ò�ѯ�Ľ���ļ�¼������*/ 
			int row =  db.DBntuples(res);

			//15 01 00 00 00 5b 41   10   00 71 00 2a 54    00 11 30 31 32 33 34 35 36 37 38 39 61 62 63 64 65 66 00 3c 11 11 22 33 44 55 66 77 88 99 11 11 22 33 44 55 66 77 88 99 00 11 22 33 44 55 66 77 88 99 00 11 22 33 44 55 66 77 88 99
			if(row > 0){

				memset (gateway_logo, '\0', sizeof(gateway_logo));
				memcpy(gateway_logo, db.DBgetvalue(res,0,1), 16);

				pos = l.LocateElem(gateway_logo);
				if( 0 == pos || !l.GetNodeData(pos, p_gateway) ){
					snprintf(sql, sizeof(sql) - 1, "update delete_node set status=2 where sn=%d;", atoi(db.DBgetvalue(res, 0, 0))); //�������ݿ��״̬Ϊ����ʧ��
					db.UpdateData(sql);
					db.DisConnectDB();
					cout << "gateway_logo errno!!!"<< endl;
					continue;
				}

				if(!p_gateway->work_status){
					cout << "UdpServer::ConfNodeToNet:  gateway not work!\n";
					break;
				}


				//l.GetNodeData(pos, p_gateway);

				memset(p_gateway->write_buff, '\0', sizeof(p_gateway->write_buff));
				p_gateway->write_buff[0] = 0x15; p_gateway->write_buff[1] = 0x01; p_gateway->write_buff[2] = 0x00;
				p_gateway->write_buff[3] = 0x00; p_gateway->write_buff[4] = 0x00; p_gateway->write_buff[5] = 0x5b;//����
				p_gateway->write_buff[6] = 0x41;//unitid
				p_gateway->write_buff[7] = 0x10;//������
				p_gateway->write_buff[8] = 0x00; p_gateway->write_buff[9] = 0x71;//��ʼ��ַ
				p_gateway->write_buff[10] = 0x00; p_gateway->write_buff[11] = 0x2a;//�Ĵ�������
				p_gateway->write_buff[12] = 0x54;//�ֽڸ���

				int device_id = atoi(db.DBgetvalue(res,0,3));
				int off_line_time_interval = atoi(db.DBgetvalue(res, 0, 4));

				p_gateway->write_buff[13] = 0;
				p_gateway->write_buff[14] = device_id;		 //13,14 λ����豸��
				memcpy(p_gateway->write_buff+15, db.DBgetvalue(res, 0, 2), 16); //15��ʼ��Žڵ��ַ
				p_gateway->write_buff[31] = (off_line_time_interval>>8)&0xff; //	����ʱ���λ
				p_gateway->write_buff[32] = (off_line_time_interval)&0xff; //	����ʱ���λ
				memcpy(p_gateway->write_buff+33, db.DBgetvalue(res, 0, 5), 64);		//	�ڵ㱸ע��Ϣ

				p_gateway->write_flag=2;//�����·���־λ
	
				p_gateway->sn = atoi(db.DBgetvalue(res, 0, 0));//��¼��ǰ�����ڱ�����к�
	
				sn = atoi(db.DBgetvalue(res, 0, 0));//���кű��浽����
	


					try {
						if(p_gateway->sockfd==NULL){
							cout << "sockfd is NULL" << endl;
							db.DBclear(res);
							db.DisConnectDB();//�Ͽ����ݿ�����ӣ�ֱ�ӷ��ز���ע��ʱ����
							return ;					

						}else{
						p_gateway->sockfd->write_some(asio::buffer(p_gateway->write_buff,13+p_gateway->write_buff[12]));
						cout << "try write_some ok"<< endl;
							
						}
					}
					catch(std::exception& e) {
						p_gateway->sockfd->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
						p_gateway->sockfd->close();
						p_gateway->async_timer->cancel();
						p_gateway->sockfd=NULL;
						p_gateway->async_timer=NULL;
					}

				} else { // û����Ҫ���õĽڵ���
					db.DBclear(res);
					db.DisConnectDB();//�Ͽ����ݿ�����ӣ�ֱ�ӷ��ز���ע��ʱ����
					return ;					
				}

				db.DBclear(res);
				db.DisConnectDB();
				break;

		} else if(1 == p_gateway->reval || 2== p_gateway->reval){	//�������ýڵ����������ݿ���е�״̬
				if(-1 == db.MyconnectDB()){
					cout << "case 0x04 connect db failed!!!"<<endl;
				}
				memset(sql, '\0', sizeof(sql));
				if(1 == p_gateway->reval){				
					snprintf(sql, sizeof(sql) - 1, "update new_node set status=2 where sn=%d;", p_gateway->sn); //�������ݿ��״̬Ϊ���óɹ�,
	cout << "UPDATE NEW_NODE STATUS RIGHT" << endl;
				}else if(2 == p_gateway->reval){
					snprintf(sql, sizeof(sql) - 1, "update new_node set status=3 where sn=%d;", p_gateway->sn); //�������ݿ��״̬Ϊ����ʧ��
	cout << "UPDATE NEW_NODE STATUS FALSE" << endl;
				}
				db.UpdateData(sql);
				p_gateway->reval=0;
				p_gateway->write_flag=2;//�����·���־λ
				p_gateway->sn = 0;//��¼��ǰ�����ڱ�����к�	
				sn = 0;

				db.DBclear(res);
				db.DisConnectDB();

				continue;
									
		}		
		
		break;
	}//while

	async_timer->expires_at(async_timer->expires_at()+posix_time::seconds( 2 ));
	async_timer->async_wait(bind(&UdpServer::ConfNodeToNet, this, asio::placeholders::error, async_timer));
	cout << "regist config new node to net async_timer 2 sec"<< endl;

}

void UdpServer::DelNodeFromNet(const boost::system::error_code& ec, timer_pt async_timer)
{

	int pos = 0;
	char gateway_logo[50];
	GatewayDB db;
	DBresult *res = NULL;
	char sql[4096] = {0};

	while(1){
		if(sn == 0){
			if(-1 == db.MyconnectDB()){
				cout << "case 0x04 connect db failed!!!"<<endl;
				log_output("DelNodeFromNet():	Connect DB failed \n");
				return;
			}
			snprintf(sql, sizeof(sql) - 1, "SELECT * FROM  delete_node where  status=0 limit 1;");
			res = db.SelectData(sql); 

			/**ȡ�ò�ѯ�Ľ���ļ�¼������*/ 
			int row =  db.DBntuples(res);

			if(row > 0){
				memset (gateway_logo, '\0', sizeof(gateway_logo));
				memcpy(gateway_logo, db.DBgetvalue(res,0,1), 16);

				pos = l.LocateElem(gateway_logo);
				printf("pos = %d\n", pos);
				if( 0 == pos || !l.GetNodeData(pos, p_gateway) ){
					snprintf(sql, sizeof(sql) - 1, "update delete_node set status=3 where sn=%d;", atoi(db.DBgetvalue(res, 0, 0))); //�������ݿ��״̬Ϊ����ʧ��
					db.UpdateData(sql);
					db.DisConnectDB();
					cout << "gateway_logo errno!!!"<< endl;
					continue;
				}

				if(!p_gateway->work_status){//���ش���ֹͣ����״̬
					cout << " UdpServer::DelNodeFromNet:  gateway not work!\n";
					snprintf(sql, sizeof(sql) - 1, "update delete_node set status=3 where sn=%d;", atoi(db.DBgetvalue(res, 0, 0))); //�������ݿ��״̬Ϊ����ʧ��
					db.UpdateData(sql);
					db.DisConnectDB();
					break;
				}				

				memset(p_gateway->write_buff, '\0', sizeof(p_gateway->write_buff));
				p_gateway->write_buff[0] = 0x15; p_gateway->write_buff[1] = 0x01; p_gateway->write_buff[2] = 0x00;
				p_gateway->write_buff[3] = 0x00; p_gateway->write_buff[4] = 0x00; p_gateway->write_buff[5] = 0x17;//����
				p_gateway->write_buff[6] = 0x41;//unitid
				p_gateway->write_buff[7] = 0x10;//������
				p_gateway->write_buff[8] = 0x00; p_gateway->write_buff[9] = 0x9b;//��ʼ��ַ
				p_gateway->write_buff[10] = 0x00; p_gateway->write_buff[11] = 0x08;//�Ĵ�������
				p_gateway->write_buff[12] = 0x10;//�ֽڸ���

				memcpy(p_gateway->write_buff+13, db.DBgetvalue(res, 0, 2), 16); //13��ʼ��Žڵ��ַ

				p_gateway->write_flag=2;//�����·���־λ
				p_gateway->sn = atoi(db.DBgetvalue(res, 0, 0));//��¼��ǰ�����ڱ�����к�
				sn = atoi(db.DBgetvalue(res, 0, 0));//���кű��浽����
					try {

						if(p_gateway->sockfd==NULL){
							cout << "sockfd is NULL" << endl;
							db.DBclear(res);
							db.DisConnectDB();//�Ͽ����ݿ�����ӣ�ֱ�ӷ��ز���ע��ʱ����
							return ;					

						}else{
							p_gateway->sockfd->write_some(asio::buffer(p_gateway->write_buff,13+p_gateway->write_buff[12]));
							cout << "try write_some ok"<< endl;
							
						}
					}
					catch(std::exception& e) {
						p_gateway->sockfd->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
						p_gateway->sockfd->close();
						p_gateway->async_timer->cancel();
						p_gateway->sockfd=NULL;
						p_gateway->async_timer=NULL;
					}

			}else { // û����Ҫɾ���Ľڵ���
				cout << "has not NODE delte"<< endl;
					db.DBclear(res);
					db.DisConnectDB();//�Ͽ����ݿ�����ӣ�ֱ�ӷ��ز���ע��ʱ����
					return ;					
				}

				db.DBclear(res);
				db.DisConnectDB();
				break;

		}else if(1 == p_gateway->reval || 2== p_gateway->reval){	//����ɾ���ڵ������ݿ���е�״̬
				if(-1 == db.MyconnectDB()){
					cout << "case 0x04 connect db failed!!!"<<endl;
				}
				memset(sql, '\0', sizeof(sql));
				if(1 == p_gateway->reval){				
					snprintf(sql, sizeof(sql) - 1, "update delete_node set status=1 where sn=%d;", p_gateway->sn); //�������ݿ��״̬Ϊ���óɹ�,
	cout << "UPDATE DELETE_NODE STATUS RIGHT" << endl;
				}else if(2 == p_gateway->reval){
					snprintf(sql, sizeof(sql) - 1, "update delete_node set status=2 where sn=%d;", p_gateway->sn); //�������ݿ��״̬Ϊ����ʧ��
	cout << "UPDATE DELETE_NODE STATUS FALSE" << endl;
				}
				db.UpdateData(sql);
				p_gateway->reval=0;
				p_gateway->write_flag=2;//�����·���־λ
				p_gateway->sn = 0;//��¼��ǰ�����ڱ�����к�	
				sn = 0;

				db.DBclear(res);
				db.DisConnectDB();

				continue;
									
		}

		break;
	}//while

	async_timer->expires_at(async_timer->expires_at()+posix_time::seconds( 2 ));
	async_timer->async_wait(bind(&UdpServer::DelNodeFromNet, this, asio::placeholders::error, async_timer));
	cout << "regist DELETE node to net async_timer 2 sec"<< endl;

}
#endif

UdpServer::~UdpServer()
{
	cout << "~UdpServer()" <<endl;
	//this->io_service_.stop();
}


unsigned int UdpServer::crc_get(unsigned char *data_point, unsigned int data_length)
{
	unsigned int crc_register,temp_data,i,j;
	crc_register=0xffff;
	for(i=0;i<data_length;i++){
		crc_register^=*data_point;
		for(j=0;j<8;j++){ temp_data=crc_register&0x0001; crc_register>>=1; if(temp_data) crc_register^=0xa001;}
		data_point++;
	}   
	return(crc_register);
}


unsigned char UdpServer::crc_check(unsigned char *frame_buff,unsigned int recv_num)
{
	unsigned int crc,org;
	org =  *(frame_buff+recv_num-1);
	org <<= 8;
	org +=  *(frame_buff+recv_num-2);
	crc=crc_get(frame_buff,recv_num-2);
	if (org==0x5555 ) return 1;
	if (crc==org) return 1; else return 0;
}