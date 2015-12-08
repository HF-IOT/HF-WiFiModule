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
#include "user_esp_platform.h"
#include "espconn.h"
#include "user_json.h"
#include "user_udp_client.h"
#include "user_uart.h"
#include "user_gpio.h"

struct espconn udp_client_conn;

extern struct esp_platform_saved_param esp_param;
#define ESP_DBG //os_printf

LOCAL void udp_client_recv_callback(void *arg, char *pdata, unsigned short len)
{
	struct espconn * pespconn = arg;
	uart0_tx_buffer(pdata,len);
}


void ICACHE_FLASH_ATTR user_udp_sent_data(char * buf,unsigned int len)
{
	remot_info * premot = NULL;
	if((len > 0) && (len < 4096)){
		os_memcpy(udp_client_conn.proto.udp->remote_ip,esp_param.ip,4);
		udp_client_conn.proto.udp->remote_port = esp_param.port;
        espconn_sent(&udp_client_conn,buf,len);
	}
}




void ICACHE_FLASH_ATTR user_udp_client_init(void)
{
	//os_printf("user_tcp_client_init\r\n");
    udp_client_conn.type = ESPCONN_UDP;
	udp_client_conn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    udp_client_conn.state = ESPCONN_NONE;
    os_memcpy(udp_client_conn.proto.udp->remote_ip,esp_param.ip,4);
    udp_client_conn.proto.udp->local_port = espconn_port();
    udp_client_conn.proto.udp->remote_port = esp_param.port;
    espconn_regist_recvcb(&udp_client_conn,udp_client_recv_callback);
    espconn_create(&udp_client_conn);
	user_uart_init();
	user_network_ready();
}


