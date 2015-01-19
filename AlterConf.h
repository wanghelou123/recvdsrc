#ifndef ALTERCONF_H
#define ALTERCONF_H

#include "pgdb.h"
#include "Gateway.h"
using namespace std;
using namespace boost;

class AlterConf
{
public:
	AlterConf(boost::asio::io_service& io);
	~AlterConf();
	int AddGateway(char * gateway_logo, queue_type& m_queue);
	int ModiGateway(char * gateway_log);
	int DelGateway(char * gateway_logo);	

private:
	GatewayDB db;
};


#endif