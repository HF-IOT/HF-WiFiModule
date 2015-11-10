#ifndef __USER_UDP_CLIENT_H__
#define __USER_UDP_CLIENT_H__

#include "user_config.h"


void user_udp_client_init(void);
void user_udp_sent_data(char * buf,unsigned int len);
	
#endif


