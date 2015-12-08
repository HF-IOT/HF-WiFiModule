/******************************************************************************
 * Copyright 2014-2016 HuaFan IOT team (zhangjinming)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2015/11/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_devicefind.h"
#include "driver/uart.h"
#include "user_iot_version.h"
#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif
#include "user_gpio.h"

void user_rf_pre_init(void)
{
	
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{	
	user_gpio_init();
	uart_init(115200,115200);
	//system_timer_reinit();
	//wifi_set_sleep_type(MODEM_SLEEP_T);
    os_printf("\n\nHFWiFiMode version:%s\n",HFWIFIMODE);
	wifi_station_set_auto_connect(1);
    user_esp_platform_init();
}

