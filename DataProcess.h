#ifndef	DATAPROCESS_H
#define DATAPROCESS_H
#include "Gateway.h"
#include "pgdb.h"
bool handle_msg(tcp_message_ptr& p, GatewayDB & db);		
#endif
