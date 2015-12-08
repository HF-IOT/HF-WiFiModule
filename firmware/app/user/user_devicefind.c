/******************************************************************************
 * Copyright 2014-2016 HuaFan IOT team (zhangjinming)
 *
 * FileName: user_devicefind.c
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
#include "user_iot_version.h"
#include "espconn.h"
#include "user_json.h"
#include "user_devicefind.h"

#define M_MACSTR "%02X%02X%02X%02X%02X%02X"
const char * device_find_request = "HF-IOT-DEVICE";
#if PLUG_DEVICE
const char * device_find_response_ok =  HFWIFIMODE ",%d," IPSTR "," M_MACSTR "\r\n";
#endif

/*---------------------------------------------------------------------------*/
LOCAL struct espconn ptrespconn;

/******************************************************************************
 * FunctionName : user_devicefind_recv
 * Description  : Processing the received data from the host
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR user_devicefind_recv(void *arg, char *pusrdata, unsigned short length)
{
    char DeviceBuffer[40] = {0};
    char Device_mac_buffer[60] = {0};
    char hwaddr[6];
    remot_info *premot = NULL;
    struct ip_info ipconfig;

    if (wifi_get_opmode() != STATION_MODE) {
        wifi_get_ip_info(SOFTAP_IF, &ipconfig);
        wifi_get_macaddr(SOFTAP_IF, hwaddr);

        if (!ip_addr_netcmp((struct ip_addr *)ptrespconn.proto.udp->remote_ip, &ipconfig.ip, &ipconfig.netmask)) {
            wifi_get_ip_info(STATION_IF, &ipconfig);
            wifi_get_macaddr(STATION_IF, hwaddr);
        }
    } else {
        wifi_get_ip_info(STATION_IF, &ipconfig);
        wifi_get_macaddr(STATION_IF, hwaddr);
    }

    if (pusrdata == NULL) {
        return;
    }

    if (length == os_strlen(device_find_request) &&
            os_strncmp(pusrdata, device_find_request, os_strlen(device_find_request)) == 0) {
        os_sprintf(DeviceBuffer, device_find_response_ok,IP2STR(&ipconfig.ip),hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5]);

        os_printf("%s\n", DeviceBuffer);
        length = os_strlen(DeviceBuffer);
        if (espconn_get_connection_info(&ptrespconn, &premot, 0) != ESPCONN_OK)
        	return;
        os_memcpy(ptrespconn.proto.udp->remote_ip, premot->remote_ip, 4);
        ptrespconn.proto.udp->remote_port = premot->remote_port;
        espconn_sent(&ptrespconn, DeviceBuffer, length);
    } else if (length == (os_strlen(device_find_request) + 18)) {
        os_sprintf(Device_mac_buffer, "%s " MACSTR , device_find_request, MAC2STR(hwaddr));
        os_printf("%s", Device_mac_buffer);

        if (os_strncmp(Device_mac_buffer, pusrdata, os_strlen(device_find_request) + 18) == 0) {
            //os_printf("%s\n", Device_mac_buffer);
            length = os_strlen(DeviceBuffer);
            os_sprintf(DeviceBuffer, "%s" MACSTR " " IPSTR, device_find_response_ok,
                       MAC2STR(hwaddr), IP2STR(&ipconfig.ip));

            os_printf("%s\n", DeviceBuffer);
            length = os_strlen(DeviceBuffer);
			if (espconn_get_connection_info(&ptrespconn, &premot, 0) != ESPCONN_OK)
				return;
			os_memcpy(ptrespconn.proto.udp->remote_ip, premot->remote_ip, 4);
			ptrespconn.proto.udp->remote_port = premot->remote_port;
            espconn_sent(&ptrespconn, DeviceBuffer, length);
        } else {
            return;
        }
    }
}

/******************************************************************************
 * FunctionName : user_devicefind_init
 * Description  : the espconn struct parame init
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_devicefind_init(void)
{
    ptrespconn.type = ESPCONN_UDP;
    ptrespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    ptrespconn.proto.udp->local_port = UDP_BOADCASTPORT;
    espconn_regist_recvcb(&ptrespconn, user_devicefind_recv);
    espconn_create(&ptrespconn);
}

