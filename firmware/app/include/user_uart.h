#ifndef __USER_UART_H__
#define __USER_UART_H__

#include "user_config.h"



void user_uart_init(void);
void user_uart_send(char * buf,unsigned int len);

#endif

