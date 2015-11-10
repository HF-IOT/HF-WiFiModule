#ifndef __USER_TCP_CLIENT_H__
#define __USER_TCP_CLIENT_H__

#include "user_config.h"


void user_tcp_client_init(void);
void user_tcp_sent_data(struct espconn * pespconn,char * buf,unsigned int len);
#endif

