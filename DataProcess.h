#ifndef	DATAPROCESS_H
#define DATAPROCESS_H
#include "Gateway.h"


bool handle_msg(tcp_message_ptr& p);		
void PacketAnalysis(tcp_message_ptr& p);

#endif