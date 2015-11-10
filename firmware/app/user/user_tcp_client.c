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
#include "user_tcp_client.h"
#include "user_uart.h"
#include "user_gpio.h"



#define ESP_DBG os_printf


#define MAX_RECONN_COUNT 100
LOCAL unsigned int tcp_reconn_count;

struct espconn tcp_client_conn;
LOCAL struct _esp_tcp tcp_client_tcp;
LOCAL os_timer_t tcp_reconn_timer;

extern struct esp_platform_saved_param esp_param;
LOCAL void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{   	
    os_printf("tcp recv:\r\n %s \r\n",pusrdata);
	//TEST
	user_uart_send(pusrdata,length);
}

void ICACHE_FLASH_ATTR user_tcp_sent_data(struct espconn * pespconn,char * buf,unsigned int len)
{
	if (pespconn != NULL){
		if((len > 0) && (len < 4096)){
			espconn_send(pespconn,buf,len);
		}
	}
}


LOCAL void ICACHE_FLASH_ATTR user_tcp_client_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;
    ESP_DBG("user_tcp_client_connect_cb\n");
    espconn_regist_recvcb(pespconn, user_tcp_recv_cb);
    espconn_set_opt(pespconn,0xF);
	tcp_reconn_count = 0;
	user_uart_init();
	user_network_ready();
}


void ICACHE_FLASH_ATTR user_tcp_client_reconnect(struct espconn *pespconn)
{
    ESP_DBG("user_tcp_client_reconnect\n");
	tcp_client_conn.proto.tcp = &tcp_client_tcp;
    tcp_client_conn.type = ESPCONN_TCP;
    tcp_client_conn.state = ESPCONN_NONE;
    os_memcpy(tcp_client_conn.proto.tcp->remote_ip,esp_param.ip,4);
    tcp_client_conn.proto.tcp->local_port = espconn_port();
    tcp_client_conn.proto.tcp->remote_port = esp_param.port;
	espconn_connect(pespconn);
    tcp_reconn_count++;
	if (tcp_reconn_count > MAX_RECONN_COUNT){
		system_restart();
	}
	user_network_unready();
}


LOCAL void ICACHE_FLASH_ATTR user_tcp_client_recon_cb(void *arg, sint8 err)
{	
	ESP_DBG("user_tcp_client_recon_cb\n");
    struct espconn *pespconn = (struct espconn *)arg;
	os_timer_disarm(&tcp_reconn_timer);
	os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)user_tcp_client_reconnect,pespconn);
	os_timer_arm(&tcp_reconn_timer,1000, 0);
}

LOCAL ICACHE_FLASH_ATTR void tcp_client_discon(void *arg)
{
	ESP_DBG("tcp_client_discon\n");
    struct espconn *pespconn = (struct espconn *)arg;
	os_timer_disarm(&tcp_reconn_timer);
	os_timer_setfn(&tcp_reconn_timer, (os_timer_func_t *)user_tcp_client_reconnect,pespconn);
	os_timer_arm(&tcp_reconn_timer,1000, 0);
}



void ICACHE_FLASH_ATTR user_tcp_client_init(void)
{
	os_printf("user_tcp_client_init\r\n");
	tcp_client_conn.proto.tcp = &tcp_client_tcp;
    tcp_client_conn.type = ESPCONN_TCP;
    tcp_client_conn.state = ESPCONN_NONE;
    os_memcpy(tcp_client_conn.proto.tcp->remote_ip,esp_param.ip,4);
    tcp_client_conn.proto.tcp->local_port = espconn_port();
    tcp_client_conn.proto.tcp->remote_port = esp_param.port;
    espconn_regist_connectcb(&tcp_client_conn, user_tcp_client_connect_cb);
    espconn_regist_reconcb(&tcp_client_conn, user_tcp_client_recon_cb);
	espconn_regist_disconcb(&tcp_client_conn, tcp_client_discon);
    espconn_connect(&tcp_client_conn);
}


