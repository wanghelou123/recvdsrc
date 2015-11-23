#ifndef	DATAPROCESS_H
#define DATAPROCESS_H
#include "Gateway.h"
#include "pgdb.h"
bool handle_msg(GatewayDB &db, tcp_message_ptr& p, bool is_delete);
#endif
