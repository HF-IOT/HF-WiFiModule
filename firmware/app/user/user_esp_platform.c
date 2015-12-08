/******************************************************************************
 * Copyright 2014-2016 HuaFan IOT team (zhangjinming)
 *
 * FileName: user_esp_platform.c
 *
 * Description: The client mode configration.
 *              Check your hardware connection with the host while use this mode.
 *
 * Modification history:
 *     2015/11/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_esp_platform.h"

#include "espconn.h"
#include "user_iot_version.h"
#include "upgrade.h"
#include "smartconfig.h"
#include "user_webserver.h"
#include "user_gpio.h"
#include "user_tcp_client.h"
#include "user_udp_client.h"
#include "driver/uart.h"

#if ESP_PLATFORM

#define ESP_DEBUG

#ifdef ESP_DEBUG
#define ESP_DBG //os_printf
#else
#define ESP_DBG
#endif

#define ACTIVE_FRAME    "{\"nonce\": %d,\"path\": \"/v1/device/activate/\", \"method\": \"POST\", \"body\": {\"encrypt_method\": \"PLAIN\", \"token\": \"%s\", \"bssid\": \""MACSTR"\",\"rom_version\":\"%s\"}, \"meta\": {\"Authorization\": \"token %s\"}}\n"

#if PLUG_DEVICE
#define RESPONSE_FRAME  "{\"status\": 200, \"datapoint\": {\"x\": %d}, \"nonce\": %d, \"deliver_to_device\": true}\n"
#define FIRST_FRAME     "{\"nonce\": %d, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"Authorization\": \"token %s\"}}\n"

#elif LIGHT_DEVICE
#include "user_light.h"

#define RESPONSE_FRAME  "{\"status\": 200,\"nonce\": %d, \"datapoint\": {\"x\": %d,\"y\": %d,\"z\": %d,\"k\": %d,\"l\": %d},\"deliver_to_device\":true}\n"
#define FIRST_FRAME     "{\"nonce\": %d, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"Authorization\": \"token %s\"}}\n"

#elif SENSOR_DEVICE
#include "user_sensor.h"

#if HUMITURE_SUB_DEVICE
#define UPLOAD_FRAME  "{\"nonce\": %d, \"path\": \"/v1/datastreams/tem_hum/datapoint/\", \"method\": \"POST\", \
\"body\": {\"datapoint\": {\"x\": %s%d.%02d,\"y\": %d.%02d}}, \"meta\": {\"Authorization\": \"token %s\"}}\n"
#elif FLAMMABLE_GAS_SUB_DEVICE
#define UPLOAD_FRAME  "{\"nonce\": %d, \"path\": \"/v1/datastreams/flammable_gas/datapoint/\", \"method\": \"POST\", \
\"body\": {\"datapoint\": {\"x\": %d.%03d}}, \"meta\": {\"Authorization\": \"token %s\"}}\n"
#endif

LOCAL uint32 count = 0;
#endif

#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"

#if PLUG_DEVICE || LIGHT_DEVICE
#define BEACON_FRAME    "{\"path\": \"/v1/ping/\", \"method\": \"POST\",\"meta\": {\"Authorization\": \"token %s\"}}\n"
#define RPC_RESPONSE_FRAME  "{\"status\": 200, \"nonce\": %d, \"deliver_to_device\": true}\n"
#define TIMER_FRAME     "{\"body\": {}, \"get\":{\"is_humanize_format_simple\":\"true\"},\"meta\": {\"Authorization\": \"Token %s\"},\"path\": \"/v1/device/timers/\",\"post\":{},\"method\": \"GET\"}\n"
#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Authorization: token %s\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"

LOCAL uint8 ping_status;
LOCAL os_timer_t beacon_timer;
#endif

#ifdef USE_DNS
ip_addr_t esp_server_ip;
#endif

LOCAL struct espconn user_conn;
LOCAL struct _esp_tcp user_tcp;
LOCAL os_timer_t client_timer;
LOCAL os_timer_t sleep_timer;
struct esp_platform_saved_param esp_param;
LOCAL uint8 device_status;
LOCAL uint8 device_recon_count = 0;
LOCAL uint32 active_nonce = 0;
LOCAL uint8 iot_version[20] = {0};
struct rst_info rtc_info;


#define M_MAX_RECONN_COUNT 5*60
void user_esp_platform_check_ip(uint8 reset_flag);

/******************************************************************************
 * FunctionName : user_esp_platform_get_token
 * Description  : get the espressif's device token
 * Parameters   : token -- the parame point which write the flash
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_get_token(uint8_t *token)
{
    if (token == NULL) {
        return;
    }

    os_memcpy(token, esp_param.token, sizeof(esp_param.token));
}

/******************************************************************************
 * FunctionName : user_esp_platform_set_token
 * Description  : save the token for the espressif's device
 * Parameters   : token -- the parame point which write the flash
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_set_token(uint8_t *token)
{
    if (token == NULL) {
        return;
    }

    esp_param.activeflag = 0;
    os_memcpy(esp_param.token, token, os_strlen(token));

    system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
}

/******************************************************************************
 * FunctionName : user_esp_platform_set_active
 * Description  : set active flag
 * Parameters   : activeflag -- 0 or 1
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_set_active(uint8 activeflag)
{
    esp_param.activeflag = activeflag;

    system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
}

void ICACHE_FLASH_ATTR
user_esp_platform_set_connect_status(uint8 status)
{
    device_status = status;
}

/******************************************************************************
 * FunctionName : user_esp_platform_get_connect_status
 * Description  : get each connection step's status
 * Parameters   : none
 * Returns      : status
*******************************************************************************/
uint8 ICACHE_FLASH_ATTR
user_esp_platform_get_connect_status(void)
{
    uint8 status = wifi_station_get_connect_status();

    if (status == STATION_GOT_IP) {
        status = (device_status == 0) ? DEVICE_CONNECTING : device_status;
    }

    ESP_DBG("status %d\n", status);
    return status;
}

/******************************************************************************
 * FunctionName : user_esp_platform_parse_nonce
 * Description  : parse the device nonce
 * Parameters   : pbuffer -- the recivce data point
 * Returns      : the nonce
*******************************************************************************/
int ICACHE_FLASH_ATTR
user_esp_platform_parse_nonce(char *pbuffer)
{
    char *pstr = NULL;
    char *pparse = NULL;
    char noncestr[11] = {0};
    int nonce = 0;
    pstr = (char *)os_strstr(pbuffer, "\"nonce\": ");

    if (pstr != NULL) {
        pstr += 9;
        pparse = (char *)os_strstr(pstr, ",");

        if (pparse != NULL) {
            os_memcpy(noncestr, pstr, pparse - pstr);
        } else {
            pparse = (char *)os_strstr(pstr, "}");

            if (pparse != NULL) {
                os_memcpy(noncestr, pstr, pparse - pstr);
            } else {
                pparse = (char *)os_strstr(pstr, "]");

                if (pparse != NULL) {
                    os_memcpy(noncestr, pstr, pparse - pstr);
                } else {
                    return 0;
                }
            }
        }

        nonce = atoi(noncestr);
    }

    return nonce;
}

/******************************************************************************
 * FunctionName : user_esp_platform_get_info
 * Description  : get and update the espressif's device status
 * Parameters   : pespconn -- the espconn used to connect with host
 *                pbuffer -- prossing the data point
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_get_info(struct espconn *pconn, uint8 *pbuffer)
{
    char *pbuf = NULL;
    int nonce = 0;

    pbuf = (char *)os_zalloc(packet_size);

    nonce = user_esp_platform_parse_nonce(pbuffer);

    if (pbuf != NULL) {
#if PLUG_DEVICE
        //os_sprintf(pbuf, RESPONSE_FRAME, user_plug_get_status(), nonce);
#elif LIGHT_DEVICE
        uint32 white_val;
        //white_val = (PWM_CHANNEL>LIGHT_COLD_WHITE?user_light_get_duty(LIGHT_COLD_WHITE):0);
        //os_sprintf(pbuf, RESPONSE_FRAME, nonce, user_light_get_period(),
        //           user_light_get_duty(LIGHT_RED), user_light_get_duty(LIGHT_GREEN),
        //           user_light_get_duty(LIGHT_BLUE),white_val );//50);
#endif

        ESP_DBG("%s\n", pbuf);
#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pconn, pbuf, os_strlen(pbuf));
#endif
        os_free(pbuf);
        pbuf = NULL;
    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_set_info
 * Description  : prossing the data and controling the espressif's device
 * Parameters   : pespconn -- the espconn used to connect with host
 *                pbuffer -- prossing the data point
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_set_info(struct espconn *pconn, uint8 *pbuffer)
{
#if PLUG_DEVICE
    char *pstr = NULL;
    pstr = (char *)os_strstr(pbuffer, "plug-status");

    if (pstr != NULL) {
        pstr = (char *)os_strstr(pbuffer, "body");

        if (pstr != NULL) {

            if (os_strncmp(pstr + 27, "1", 1) == 0) {
                //user_plug_set_status(0x01);
            } else if (os_strncmp(pstr + 27, "0", 1) == 0) {
                //user_plug_set_status(0x00);
            }
        }
    }

#elif LIGHT_DEVICE
    char *pstr = NULL;
    char *pdata = NULL;
    char *pbuf = NULL;
    char recvbuf[10];
    uint16 length = 0;
    uint32 data = 0;
    static uint32 rr,gg,bb,cw,ww,period;
    ww=0;
    cw=0;
    extern uint8 light_sleep_flg;
    pstr = (char *)os_strstr(pbuffer, "\"path\": \"/v1/datastreams/light/datapoint/\"");

    if (pstr != NULL) {
        pstr = (char *)os_strstr(pbuffer, "{\"datapoint\": ");

        if (pstr != NULL) {
            pbuf = (char *)os_strstr(pbuffer, "}}");
            length = pbuf - pstr;
            length += 2;
            pdata = (char *)os_zalloc(length + 1);
            os_memcpy(pdata, pstr, length);

            pstr = (char *)os_strchr(pdata, 'x');

            if (pstr != NULL) {
                pstr += 4;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    period = data;
                    //user_light_set_period(data);
                }
            }

            pstr = (char *)os_strchr(pdata, 'y');

            if (pstr != NULL) {
                pstr += 4;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    rr=data;
                    //ESP_DBG("r: %d\r\n",rr);
                    //user_light_set_duty(data, 0);
                }
            }

            pstr = (char *)os_strchr(pdata, 'z');

            if (pstr != NULL) {
                pstr += 4;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    gg=data;
                    //ESP_DBG("g: %d\r\n",gg);
                    //user_light_set_duty(data, 1);
                }
            }

            pstr = (char *)os_strchr(pdata, 'k');

            if (pstr != NULL) {
                pstr += 4;;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    bb=data;
                    //ESP_DBG("b: %d\r\n",bb);
                    //user_light_set_duty(data, 2);
                }
            }

            pstr = (char *)os_strchr(pdata, 'l');

            if (pstr != NULL) {
                pstr += 4;;
                pbuf = (char *)os_strchr(pstr, ',');

                if (pbuf != NULL) {
                    length = pbuf - pstr;
                    os_memset(recvbuf, 0, 10);
                    os_memcpy(recvbuf, pstr, length);
                    data = atoi(recvbuf);
                    cw=data;
		      ww=data;
                    //ESP_DBG("cw: %d\r\n",cw);
		      //ESP_DBG("ww:%d\r\n",ww);   //chg
                    //user_light_set_duty(data, 2);
                }
            }

            os_free(pdata);
        }
    }
    
    if((rr|gg|bb|cw|ww) == 0){
        if(light_sleep_flg==0){

        }
        
    }else{
        if(light_sleep_flg==1){
            //ESP_DBG("modem sleep en\r\n");
            wifi_set_sleep_type(MODEM_SLEEP_T);
            light_sleep_flg =0;
        }
    }
#endif

    user_esp_platform_get_info(pconn, pbuffer);
}

/******************************************************************************
 * FunctionName : user_esp_platform_reconnect
 * Description  : reconnect with host after get ip
 * Parameters   : pespconn -- the espconn used to reconnect with host
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_esp_platform_reconnect(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_reconnect\n");
    user_esp_platform_check_ip(0);
}

/******************************************************************************
 * FunctionName : user_esp_platform_discon_cb
 * Description  : disconnect successfully with the host
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_discon_cb(void *arg)
{
    struct espconn *pespconn = arg;
    struct ip_info ipconfig;
	struct dhcp_client_info dhcp_info;
    ESP_DBG("user_esp_platform_discon_cb\n");

#if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_disarm(&beacon_timer);
#endif

    if (pespconn == NULL) {
        return;
    }

    pespconn->proto.tcp->local_port = espconn_port();
    user_esp_platform_reconnect(pespconn);
}

/******************************************************************************
 * FunctionName : user_esp_platform_discon
 * Description  : A new incoming connection has been disconnected.
 * Parameters   : espconn -- the espconn used to disconnect with host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_discon(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_discon\n");
#ifdef CLIENT_SSL_ENABLE
    espconn_secure_disconnect(pespconn);
#else
    espconn_disconnect(pespconn);
#endif
}

/******************************************************************************
 * FunctionName : user_esp_platform_sent_cb
 * Description  : Data has been sent successfully and acknowledged by the remote host.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_sent_cb(void *arg)
{
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_sent_cb\n");
}

/******************************************************************************
 * FunctionName : user_esp_platform_sent
 * Description  : Processing the application data and sending it to the host
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_sent(struct espconn *pespconn)
{
    uint8 devkey[token_size] = {0};
    uint32 nonce;
    char *pbuf = (char *)os_zalloc(packet_size);

    os_memcpy(devkey, esp_param.devkey, 40);

    if (esp_param.activeflag == 0xFF) {
        esp_param.activeflag = 0;
    }

    if (pbuf != NULL) {
        if (esp_param.activeflag == 0) {
            uint8 token[token_size] = { 0 };
            uint8 bssid[6];
            active_nonce = os_random() & 0x7FFFFFFF;

            os_memcpy(token, esp_param.devkey, 40);

            wifi_get_macaddr(STATION_IF, bssid);

            os_sprintf(pbuf, ACTIVE_FRAME, active_nonce, token, MAC2STR(bssid),HFWIFIMODE, devkey);
        }

#if SENSOR_DEVICE
#if HUMITURE_SUB_DEVICE
        else {
#if 0
#else
            uint16 tp, rh;
            uint8 *data;
            uint32 tp_t, rh_t;
            rh = data[0] << 8 | data[1];
            tp = data[2] << 8 | data[3];
#endif
            tp_t = (tp >> 2) * 165 * 100 / (16384 - 1);
            rh_t = (rh & 0x3fff) * 100 * 100 / (16384 - 1);

            if (tp_t >= 4000) {
                os_sprintf(pbuf, UPLOAD_FRAME, count, "", tp_t / 100 - 40, tp_t % 100, rh_t / 100, rh_t % 100, devkey);
            } else {
                tp_t = 4000 - tp_t;
                os_sprintf(pbuf, UPLOAD_FRAME, count, "-", tp_t / 100, tp_t % 100, rh_t / 100, rh_t % 100, devkey);
            }
        }

#elif FLAMMABLE_GAS_SUB_DEVICE
        else {
            uint32 adc_value = system_adc_read();

            os_sprintf(pbuf, UPLOAD_FRAME, count, adc_value / 1024, adc_value * 1000 / 1024, devkey);
        }

#endif
#else
        else {
            nonce = os_random() & 0x7FFFFFFF;
            os_sprintf(pbuf, FIRST_FRAME, nonce , devkey);
        }

#endif
        ESP_DBG("%s\n", pbuf);

#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif

        os_free(pbuf);
    }
}

#if PLUG_DEVICE || LIGHT_DEVICE
/******************************************************************************
 * FunctionName : user_esp_platform_sent_beacon
 * Description  : sent beacon frame for connection with the host is activate
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR user_esp_platform_sent_beacon(struct espconn *pespconn)
{
    if (pespconn == NULL) {
        return;
    }

    if (pespconn->state == ESPCONN_CONNECT) {
        if (esp_param.activeflag == 0) {
            ESP_DBG("please check device is activated.\n");
            user_esp_platform_sent(pespconn);
        } else {
            uint8 devkey[token_size] = {0};
            os_memcpy(devkey, esp_param.devkey, 40);

            ESP_DBG("user_esp_platform_sent_beacon %u\n", system_get_time());

            if (ping_status == 0) {
                ESP_DBG("user_esp_platform_sent_beacon sent fail!\n");
                user_esp_platform_discon(pespconn);
            } else {
                char *pbuf = (char *)os_zalloc(packet_size);

                if (pbuf != NULL) {
                    os_sprintf(pbuf, BEACON_FRAME, devkey);

#ifdef CLIENT_SSL_ENABLE
                    espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
                    espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif

                    ping_status = 0;
                    os_timer_arm(&beacon_timer, BEACON_TIME, 0);
                    os_free(pbuf);
                }
            }
        }
    } else {
        ESP_DBG("user_esp_platform_sent_beacon sent fail!\n");
        user_esp_platform_discon(pespconn);
    }
}

/******************************************************************************
 * FunctionName : user_platform_rpc_set_rsp
 * Description  : response the message to server to show setting info is received
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                nonce -- mark the message received from server
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_platform_rpc_set_rsp(struct espconn *pespconn, int nonce)
{
    char *pbuf = (char *)os_zalloc(packet_size);

    if (pespconn == NULL) {
        return;
    }

    os_sprintf(pbuf, RPC_RESPONSE_FRAME, nonce);
    ESP_DBG("%s\n", pbuf);
#ifdef CLIENT_SSL_ENABLE
    espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
    espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif
    os_free(pbuf);
}

/******************************************************************************
 * FunctionName : user_platform_timer_get
 * Description  : get the timers from server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_platform_timer_get(struct espconn *pespconn)
{
    uint8 devkey[token_size] = {0};
    char *pbuf = (char *)os_zalloc(packet_size);
    os_memcpy(devkey, esp_param.devkey, 40);

    if (pespconn == NULL) {
        return;
    }

    os_sprintf(pbuf, TIMER_FRAME, devkey);
    ESP_DBG("%s\n", pbuf);
#ifdef CLIENT_SSL_ENABLE
    espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
    espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif
    os_free(pbuf);
}

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : Processing the downloaded data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_rsp(void *arg)
{
    struct upgrade_server_info *server = arg;
    struct espconn *pespconn = server->pespconn;
    uint8 devkey[41] = {0};
    uint8 *pbuf = NULL;
    char *action = NULL;

    os_memcpy(devkey, esp_param.devkey, 40);
    pbuf = (char *)os_zalloc(packet_size);

    if (server->upgrade_flag == true) {
        ESP_DBG("user_esp_platform_upgarde_successfully\n");
        action = "device_upgrade_success";
        os_sprintf(pbuf, UPGRADE_FRAME, devkey, action, server->pre_version, server->upgrade_version);
        ESP_DBG("%s\n",pbuf);

#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif

        if (pbuf != NULL) {
            os_free(pbuf);
            pbuf = NULL;
        }
		/* update success reset */
		os_timer_disarm(&client_timer);
        os_timer_setfn(&client_timer, (os_timer_func_t *)system_upgrade_reboot, NULL);
        os_timer_arm(&client_timer, 1000, 0);
    } else {
        ESP_DBG("user_esp_platform_upgrade_failed\n");
        action = "device_upgrade_failed";
        os_sprintf(pbuf, UPGRADE_FRAME, devkey, action,server->pre_version, server->upgrade_version);
        ESP_DBG("%s\n",pbuf);

#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif

        if (pbuf != NULL) {
            os_free(pbuf);
            pbuf = NULL;
        }
    }

    os_free(server->url);
    server->url = NULL;
    os_free(server);
    server = NULL;
}

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_begin
 * Description  : Processing the received data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                server -- upgrade param
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_begin(struct espconn *pespconn, struct upgrade_server_info *server)
{
    uint8 user_bin[9] = {0};
    uint8 devkey[41] = {0};

    server->pespconn = pespconn;

    os_memcpy(devkey, esp_param.devkey, 40);
    os_memcpy(server->ip, pespconn->proto.tcp->remote_ip, 4);

#ifdef UPGRADE_SSL_ENABLE
    server->port = 443;
#else
    server->port = 80;
#endif

    server->check_cb = user_esp_platform_upgrade_rsp;
    server->check_times = 120000;

    if (server->url == NULL) {
        server->url = (uint8 *)os_zalloc(512);
    }

    if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1) {
        os_memcpy(user_bin, "user2.bin", 10);
    } else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2) {
        os_memcpy(user_bin, "user1.bin", 10);
    }

    os_sprintf(server->url, "GET /v1/device/rom/?action=download_rom&version=%s&filename=%s HTTP/1.0\r\nHost: "IPSTR":%d\r\n"pheadbuffer"",
               server->upgrade_version, user_bin, IP2STR(server->ip),
               server->port, devkey);
    ESP_DBG("%s\n",server->url);

#ifdef UPGRADE_SSL_ENABLE

    if (system_upgrade_start_ssl(server) == false) {
#else

    if (system_upgrade_start(server) == false) {
#endif
        ESP_DBG("upgrade is already started\n");
    }
}
#endif

/******************************************************************************
 * FunctionName : user_esp_platform_recv_cb
 * Description  : Processing the received data from the server
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    char *pstr = NULL;
    LOCAL char pbuffer[1024 * 2] = {0};
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_recv_cb %s\n", pusrdata);

#if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_disarm(&beacon_timer);
#endif

    if (length == 1460) {
        os_memcpy(pbuffer, pusrdata, length);
    } else {
        struct espconn *pespconn = (struct espconn *)arg;

        os_memcpy(pbuffer + os_strlen(pbuffer), pusrdata, length);

        if ((pstr = (char *)os_strstr(pbuffer, "\"activate_status\": ")) != NULL &&
                user_esp_platform_parse_nonce(pbuffer) == active_nonce) {
            if (os_strncmp(pstr + 19, "1", 1) == 0) {
                ESP_DBG("device activates successful.\n");

                device_status = DEVICE_ACTIVE_DONE;
                esp_param.activeflag = 1;
                system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
                user_esp_platform_sent(pespconn);
		  if(LIGHT_DEVICE){
                    system_restart();
                }
            } else {
                ESP_DBG("device activates failed.\n");
                device_status = DEVICE_ACTIVE_FAIL;
            }
        }

#if (PLUG_DEVICE || LIGHT_DEVICE)
        else if ((pstr = (char *)os_strstr(pbuffer, "\"action\": \"sys_upgrade\"")) != NULL) {
            if ((pstr = (char *)os_strstr(pbuffer, "\"version\":")) != NULL) {
                struct upgrade_server_info *server = NULL;
                int nonce = user_esp_platform_parse_nonce(pbuffer);
                user_platform_rpc_set_rsp(pespconn, nonce);

                server = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));
                os_memcpy(server->upgrade_version, pstr + 12, 16);
                server->upgrade_version[15] = '\0';
                os_sprintf(server->pre_version,"%s%d.%d.%dt%d(%s)",VERSION_TYPE,IOT_VERSION_MAJOR,\
                    	IOT_VERSION_MINOR,IOT_VERSION_REVISION,device_type,UPGRADE_FALG);
                user_esp_platform_upgrade_begin(pespconn, server);
            }
        } else if ((pstr = (char *)os_strstr(pbuffer, "\"action\": \"sys_reboot\"")) != NULL) {
            os_timer_disarm(&client_timer);
            os_timer_setfn(&client_timer, (os_timer_func_t *)system_upgrade_reboot, NULL);
            os_timer_arm(&client_timer, 1000, 0);
        } else if ((pstr = (char *)os_strstr(pbuffer, "/v1/device/timers/")) != NULL) {
            int nonce = user_esp_platform_parse_nonce(pbuffer);
            user_platform_rpc_set_rsp(pespconn, nonce);
            os_timer_disarm(&client_timer);
            os_timer_setfn(&client_timer, (os_timer_func_t *)user_platform_timer_get, pespconn);
            os_timer_arm(&client_timer, 2000, 0);
        } else if ((pstr = (char *)os_strstr(pbuffer, "\"method\": ")) != NULL) {
            if (os_strncmp(pstr + 11, "GET", 3) == 0) {
                user_esp_platform_get_info(pespconn, pbuffer);
            } else if (os_strncmp(pstr + 11, "POST", 4) == 0) {
                user_esp_platform_set_info(pespconn, pbuffer);
            }
        } else if ((pstr = (char *)os_strstr(pbuffer, "ping success")) != NULL) {
            ESP_DBG("ping success\n");
            ping_status = 1;
        } else if ((pstr = (char *)os_strstr(pbuffer, "send message success")) != NULL) {
        } else if ((pstr = (char *)os_strstr(pbuffer, "timers")) != NULL) {
            user_platform_timer_start(pusrdata , pespconn);
        }

#elif SENSOR_DEVICE
        else if ((pstr = (char *)os_strstr(pbuffer, "\"status\":")) != NULL) {
            if (os_strncmp(pstr + 10, "200", 3) != 0) {
                ESP_DBG("message upload failed.\n");
            } else {
                count++;
                ESP_DBG("message upload sucessful.\n");
            }

            os_timer_disarm(&client_timer);
            os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_discon, pespconn);
            os_timer_arm(&client_timer, 10, 0);
        }

#endif
        else if ((pstr = (char *)os_strstr(pbuffer, "device")) != NULL) {
#if PLUG_DEVICE || LIGHT_DEVICE
            user_platform_timer_get(pespconn);
#elif SENSOR_DEVICE

#endif
        }

        os_memset(pbuffer, 0, sizeof(pbuffer));
    }

#if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_arm(&beacon_timer, BEACON_TIME, 0);
#endif
}

#if AP_CACHE
/******************************************************************************
 * FunctionName : user_esp_platform_ap_change
 * Description  : add the user interface for changing to next ap ID.
 * Parameters   :
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_ap_change(void)
{
    uint8 current_id;
    uint8 i = 0;
    ESP_DBG("user_esp_platform_ap_is_changing\n");

    current_id = wifi_station_get_current_ap_id();
    ESP_DBG("current ap id =%d\n", current_id);

    if (current_id == AP_CACHE_NUMBER - 1) {
       i = 0;
    } else {
       i = current_id + 1;
    }
    while (wifi_station_ap_change(i) != true) {
       i++;
       if (i == AP_CACHE_NUMBER - 1) {
    	   i = 0;
       }
    }
    /* just need to re-check ip while change AP */
    device_recon_count = 0;
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
    os_timer_arm(&client_timer, 100, 0);
	/******/
	static unsigned int m_reconnect_count = 0;
	m_reconnect_count++;
	if (m_reconnect_count > M_MAX_RECONN_COUNT){
		system_restart();
	}
	/******/
}
#endif

LOCAL bool ICACHE_FLASH_ATTR
user_esp_platform_reset_mode(void)
{
    if (wifi_get_opmode() == STATION_MODE) {
        wifi_set_opmode(STATIONAP_MODE);
    }

#if AP_CACHE
    /* delay 5s to change AP */
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_ap_change, NULL);
    os_timer_arm(&client_timer, 5000, 0);

    return true;
#endif
    return false;
}

/******************************************************************************
 * FunctionName : user_esp_platform_recon_cb
 * Description  : The connection had an error and is already deallocated.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_recon_cb(void *arg, sint8 err)
{
    struct espconn *pespconn = (struct espconn *)arg;

    ESP_DBG("user_esp_platform_recon_cb\n");

#if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_disarm(&beacon_timer);
#endif
    if (++device_recon_count == 5) {
        device_status = DEVICE_CONNECT_SERVER_FAIL;

        if (user_esp_platform_reset_mode()) {
            return;
        }
    }
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
    os_timer_arm(&client_timer, 1000, 0);
}

/******************************************************************************
 * FunctionName : user_esp_platform_connect_cb
 * Description  : A new incoming connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_connect_cb\n");
    if (wifi_get_opmode() ==  STATIONAP_MODE ) {
        wifi_set_opmode(STATION_MODE);
    }
    device_recon_count = 0;
    espconn_regist_recvcb(pespconn, user_esp_platform_recv_cb);
    espconn_regist_sentcb(pespconn, user_esp_platform_sent_cb);
    user_esp_platform_sent(pespconn);
}

/******************************************************************************
 * FunctionName : user_esp_platform_connect
 * Description  : The function given as the connect with the host
 * Parameters   : espconn -- the espconn used to connect the connection
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_connect(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_connect\n");

#ifdef CLIENT_SSL_ENABLE
    espconn_secure_connect(pespconn);
#else
    espconn_connect(pespconn);
#endif
}

#ifdef USE_DNS
/******************************************************************************
 * FunctionName : user_esp_platform_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;

    if (ipaddr == NULL) {
        ESP_DBG("user_esp_platform_dns_found NULL\n");

        if (++device_recon_count == 5) {
            device_status = DEVICE_CONNECT_SERVER_FAIL;

            user_esp_platform_reset_mode();
        }

        return;
    }

    ESP_DBG("user_esp_platform_dns_found %d.%d.%d.%d\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
            *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));

    if (esp_server_ip.addr == 0 && ipaddr->addr != 0) {
        os_timer_disarm(&client_timer);
        esp_server_ip.addr = ipaddr->addr;
        os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);

        pespconn->proto.tcp->local_port = espconn_port();

#ifdef CLIENT_SSL_ENABLE
        pespconn->proto.tcp->remote_port = 8443;
#else
        pespconn->proto.tcp->remote_port = 8000;
#endif

#if (PLUG_DEVICE || LIGHT_DEVICE)
        ping_status = 1;
#endif

        espconn_regist_connectcb(pespconn, user_esp_platform_connect_cb);
        espconn_regist_disconcb(pespconn, user_esp_platform_discon_cb);
        espconn_regist_reconcb(pespconn, user_esp_platform_recon_cb);
        user_esp_platform_connect(pespconn);
    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_dns_check_cb
 * Description  : 1s time callback to check dns found
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_dns_check_cb(void *arg)
{
    struct espconn *pespconn = arg;

    ESP_DBG("user_esp_platform_dns_check_cb\n");

    espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);

    os_timer_arm(&client_timer, 1000, 0);
}

LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_start_dns(struct espconn *pespconn)
{
    esp_server_ip.addr = 0;
    espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);

    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_dns_check_cb, pespconn);
    os_timer_arm(&client_timer, 1000, 0);
}
#endif


/******************************************************************************
 * FunctionName : user_esp_platform_check_ip
 * Description  : espconn struct parame init when get ip addr
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_esp_platform_check_ip(uint8 reset_flag)
{
    struct ip_info ipconfig;

    os_timer_disarm(&client_timer);
#if 1
    wifi_get_ip_info(STATION_IF, &ipconfig);

    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
        user_conn.proto.tcp = &user_tcp;
        user_conn.type = ESPCONN_TCP;
        user_conn.state = ESPCONN_NONE;

        device_status = DEVICE_CONNECTING;

        if (reset_flag) {
            device_recon_count = 0;
        }

#if (PLUG_DEVICE || LIGHT_DEVICE)
        os_timer_disarm(&beacon_timer);
        os_timer_setfn(&beacon_timer, (os_timer_func_t *)user_esp_platform_sent_beacon, &user_conn);
#endif

#ifdef USE_DNS
        user_esp_platform_start_dns(&user_conn);
#else
        const char esp_server_ip[4] = {115, 29, 202, 58};
        os_memcpy(user_conn.proto.tcp->remote_ip, esp_server_ip, 4);
        user_conn.proto.tcp->local_port = espconn_port();
#ifdef CLIENT_SSL_ENABLE
        user_conn.proto.tcp->remote_port = 8443;
#else
        user_conn.proto.tcp->remote_port = 8000;
#endif
        espconn_regist_connectcb(&user_conn, user_esp_platform_connect_cb);
        espconn_regist_reconcb(&user_conn, user_esp_platform_recon_cb);
        user_esp_platform_connect(&user_conn);
#endif
		// get ip init function		
		if (esp_param.wifiwork_mode == 3){
			user_devicefind_init();
			user_webserver_init(SERVER_PORT);
		}
		if (esp_param.wifiwork_mode == 1){
			user_tcp_client_init();
		}
		if (esp_param.wifiwork_mode == 2){
			user_udp_client_init();
		}
    } else {
        /* if there are wrong while connecting to some AP, then reset mode */
        if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
                wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
                wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) {
            user_esp_platform_reset_mode();
        } else {
            os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
            os_timer_arm(&client_timer, 100, 0);
        }
    }
#endif
}




unsigned char m_str_to_ch(char ch)
{
	unsigned char res = 0;
	if ((ch >= '0') && (ch <= '9')){
		res = ch - '0';
	}
	if ((ch >= 'a') && (ch <= 'f')){
		res = ch - 'a' + 10;
	}
	if ((ch >= 'A') && (ch <= 'F')){
		res = ch - 'A' + 10;
	}
	res = res & 0xF;
	return res;
}


unsigned char m_str_to_hex(char * buf)
{
	unsigned char lt = 0;
	unsigned char ht;
	unsigned char res = 0;
	ht = m_str_to_ch(buf[0]);
	lt = m_str_to_ch(buf[1]);
	res = lt + (ht<<4);
	return res;
}


void ICACHE_FLASH_ATTR sleep_handle(void *timer_arg)
{
	static unsigned int pre_status = 1;
	unsigned char status = 0;
	status = user_is_restore_set();
	if ((status == 0) && (pre_status == 0)){
		user_network_unready();
		//deep sleep
		pre_status = 1;
		//system_deep_sleep(0);
	}
	pre_status = status;
}



void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata)
{	
	unsigned int type = 0;
	char tmp_ssid[64] = { 0 };
	char tmp_pwd[64] = { 0 };
	char m_protol_buf[128] = { 0 };
	struct station_config * sta_conf = pdata;
    switch(status) {
        case SC_STATUS_WAIT:            
            break;
        case SC_STATUS_FIND_CHANNEL:            
            break;
        case SC_STATUS_GETTING_SSID_PSWD:			
			type = *(unsigned int *)pdata;
            if (type == SC_TYPE_ESPTOUCH) {
                //ESP_DBG("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                //ESP_DBG("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            ESP_DBG("SC_STATUS_LINK\n");
			//解析SSID 和 PWD
			os_memcpy(m_protol_buf,sta_conf->password,os_strlen(sta_conf->password));
			if (os_strncmp(m_protol_buf,HFCONFIGFLAG,os_strlen(HFCONFIGFLAG)) == 0){
				//解析
				char * p_start = NULL;
				char * p_end = NULL;
				unsigned char m_ip[4] = { 0 };
				unsigned int m_port = 0;
				unsigned char wifimode_workmdoe = 0;
				p_start = m_protol_buf + 6;
				wifimode_workmdoe = m_str_to_ch(p_start[0]);
				p_start += 2;
				//分析工作模式
				switch (wifimode_workmdoe){
					case 0:						
					case 1:
					case 2:
						//IP地址和端口
						m_ip[0] = m_str_to_hex(p_start);
						p_start += 2;
						m_ip[1] = m_str_to_hex(p_start);
						p_start += 2;
						m_ip[2] = m_str_to_hex(p_start);
						p_start += 2;
						m_ip[3] = m_str_to_hex(p_start);
						p_start += 2;
						p_start += 1;
						unsigned int port_l = 0;
						unsigned int port_h = 0;
						port_h = m_str_to_hex(p_start);
						p_start += 2;
						port_l = m_str_to_hex(p_start);
						m_port = (port_h<<8) + port_l;
						esp_param.wifiwork_mode = wifimode_workmdoe;
						os_memcpy(esp_param.ip,m_ip,4);
						esp_param.port = m_port;
						p_start = (char *)os_strstr(p_start,"-");
						if (p_start != NULL){
							p_start += 1;
							os_strncpy(tmp_pwd,p_start,os_strlen(p_start));
						}
						ESP_DBG("workmdoe:%d IP:%d.%d.%d.%d port:%d \n",wifimode_workmdoe,esp_param.ip[0],esp_param.ip[1],esp_param.ip[2],esp_param.ip[3],m_port);
						break;
					case 3:
						//<@#&>-3-C0A80172-1ED7-yanfa222
						p_start = (char *)os_strstr(p_start,"-");
						if (p_start != NULL){
							p_start += 1;
							p_start = (char *)os_strstr(p_start,"-");
							if (p_start != NULL){
								p_start += 1;
								os_strncpy(tmp_pwd,p_start,os_strlen(p_start));
							}
						}
						break;
					case 8:						
						break;
					case 9:						
						break;
					default:
						break;
				}
				struct station_config m_conf;
				os_memset(&m_conf,0,sizeof(m_conf));
				os_strncpy(tmp_ssid,sta_conf->ssid,os_strlen(sta_conf->ssid));
				if(os_strlen(tmp_ssid) > 2){
					//LINK
					m_conf.bssid_set = 0;
					os_strncpy(m_conf.ssid,tmp_ssid,os_strlen(tmp_ssid));
					os_strncpy(m_conf.password,tmp_pwd,os_strlen(tmp_pwd));
					ESP_DBG("SSID:%s PWD:%s \r\n",m_conf.ssid,m_conf.password);
					wifi_station_set_config(&m_conf);
					wifi_station_disconnect();
					wifi_station_connect();
					system_param_save_with_protect(ESP_PARAM_START_SEC,&esp_param, sizeof(esp_param));
					return ;
				}
				system_param_save_with_protect(ESP_PARAM_START_SEC,&esp_param, sizeof(esp_param));
				user_wifi_led_stop();
			}
			else{
				// 联网
				//参数配置完成
				if ((esp_param.wifiwork_mode > 0) && (esp_param.wifiwork_mode < 16) ){
					wifi_station_set_config(sta_conf);
					wifi_station_disconnect();
					wifi_station_connect();
					os_memcpy(esp_param.SSID,sta_conf->bssid,os_strlen(sta_conf->bssid));
					os_memcpy(esp_param.PWD,sta_conf->password,os_strlen(sta_conf->password));
					system_param_save_with_protect(ESP_PARAM_START_SEC,&esp_param, sizeof(esp_param));
				}
				else{
					os_timer_disarm(&client_timer);
        			os_timer_setfn(&client_timer, (os_timer_func_t *)system_restart, 0);
        			os_timer_arm(&client_timer, 1000, 0);
				}
			}
            break;
        case SC_STATUS_LINK_OVER:
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};
                os_memcpy(phone_ip, (uint8*)pdata, 4);
                ESP_DBG("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            }
            smartconfig_stop();
			user_wifi_led_stop();
			esp_param.wificonfigflag = 1;
			system_param_save_with_protect(ESP_PARAM_START_SEC,&esp_param, sizeof(esp_param));
			os_timer_disarm(&client_timer);
        	os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, 1);
        	os_timer_arm(&client_timer, 100, 0);
            break;
    }
	
}

/******************************************************************************
 * FunctionName : user_esp_platform_init
 * Description  : device parame init based on espressif platform
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_esp_platform_init(void)
{
	system_param_load(ESP_PARAM_START_SEC, 0, &esp_param, sizeof(esp_param));
	struct rst_info *rtc_info = system_get_rst_info();
	if (rtc_info->reason == REASON_WDT_RST ||
		rtc_info->reason == REASON_EXCEPTION_RST ||
		rtc_info->reason == REASON_SOFT_WDT_RST) {
		if (rtc_info->reason == REASON_EXCEPTION_RST) {
			ESP_DBG("Fatal exception (%d):\n", rtc_info->exccause);
		}
		ESP_DBG("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
				rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
	}
	/***add by tzx for saving ip_info to avoid dhcp_client start****/
    struct dhcp_client_info dhcp_info;
    struct ip_info sta_info;
    system_rtc_mem_read(64,&dhcp_info,sizeof(struct dhcp_client_info));
	if(dhcp_info.flag == 0x01 ) {
		if (true == wifi_station_dhcpc_status()){
			wifi_station_dhcpc_stop();
		}
		sta_info.ip = dhcp_info.ip_addr;
		sta_info.gw = dhcp_info.gw;
		sta_info.netmask = dhcp_info.netmask;
		if ( true != wifi_set_ip_info(STATION_IF,&sta_info)) {
			ESP_DBG("set default ip wrong\n");
		}
	}
    os_memset(&dhcp_info,0,sizeof(struct dhcp_client_info));
    system_rtc_mem_write(64,&dhcp_info,sizeof(struct rst_info));
	//UART0 PRINT MAC
	char hwaddr[6] = { 0 };
	char M_MAC[32] = { 0 };
	wifi_get_macaddr(STATION_IF,hwaddr);
	os_sprintf(M_MAC,"\r\n\r\nMAC:%02X%02X%02X%02X%02X%02X\r\n",hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5]);
	uart0_tx_buffer(M_MAC,os_strlen(M_MAC));
#if AP_CACHE
    wifi_station_ap_number_set(AP_CACHE_NUMBER);
#endif
	//恢复出厂设置
	if (user_is_restore_set() == 0){
		ESP_DBG("user_is_restore_set\r\n");
		esp_param.wificonfigflag = -1;
		system_param_save_with_protect(ESP_PARAM_START_SEC,&esp_param, sizeof(esp_param));
	}
	ESP_DBG("wificonfigflag:%d wifiwork_mode:%d\r\n",esp_param.wificonfigflag,esp_param.wifiwork_mode);
	if (esp_param.wificonfigflag != 1){
		// init
		wifi_set_opmode(STATION_MODE);
		smartconfig_start(smartconfig_done,0);
		if ((esp_param.wifiwork_mode == 1) || (esp_param.wifiwork_mode == 2)){
			user_wifi_led_high_shark();
		}
		else{
			user_wifi_led_high_shark();
		}
	}
	else{		
		if (wifi_get_opmode() != SOFTAP_MODE) {
       		os_timer_disarm(&client_timer);
        	os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, 1);
        	os_timer_arm(&client_timer, 100, 0);
    	}
	}
	// SLEEP  
	//os_timer_disarm(&sleep_timer);
    //os_timer_setfn(&sleep_timer, (os_timer_func_t *)sleep_handle, 1);
    //os_timer_arm(&sleep_timer,1000,1);
}

#endif
