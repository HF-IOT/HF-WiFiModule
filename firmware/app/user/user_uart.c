/******************************************************************************
 * Copyright 2014-2016 HuaFan IOT team (zhangjinming)
 *
 * FileName: user_uart.c
 *
 * Description: Find your hardware's information while working any mode.
 *
 * Modification history:
 *     2015/11/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "user_json.h"
#include "user_uart.h"
#include "user_tcp_client.h"
#include "user_udp_client.h"
#include "user_esp_platform.h"

LOCAL os_timer_t uart_rx_timer;
extern struct esp_platform_saved_param esp_param;

extern struct espconn tcp_client_conn;
extern struct espconn udp_client_conn;


LOCAL unsigned int m_uart_explain(char * buf,unsigned int len);
unsigned char tmp_buf[4096] = { 0 };
LOCAL void user_uart_recv(void)
{
	unsigned int len = 0;	
	len = rx_buff_deq(tmp_buf,sizeof(tmp_buf) - 1);
	if ((len > 0) && (len < 4096)){
		m_uart_explain(tmp_buf,len);
	}
}

LOCAL unsigned int m_uart_explain(char * buf,unsigned int len)
{
	unsigned int ires = 0;
	//TEST
	if (esp_param.wifiwork_mode == 1){
		user_tcp_sent_data(&tcp_client_conn,buf,len);
	}
	if (esp_param.wifiwork_mode == 2){
		user_udp_sent_data(buf,len);
	}
	return ires;
}

void ICACHE_FLASH_ATTR user_uart_send(char * buf,unsigned int len)
{
	tx_buff_enq(buf,len);
}


void ICACHE_FLASH_ATTR user_uart_init(void)
{
    hw_timer_init(0,1);
    hw_timer_set_func(user_uart_recv);
    hw_timer_arm(100000);     //uart_rx_timer
    //os_timer_disarm(&uart_rx_timer);
	//os_timer_setfn(&uart_rx_timer, (os_timer_func_t *)user_uart_recv,NULL);
	//os_timer_arm(&uart_rx_timer,10,1);
}

