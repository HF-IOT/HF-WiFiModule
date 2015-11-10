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
#include "gpio.h"
#include "espconn.h"
#include "user_json.h"
#include "user_gpio.h"


LOCAL os_timer_t led_timer;
LOCAL uint8 led_level = 0;


void user_wifi_led_timer_init(unsigned int m_time);

void ICACHE_FLASH_ATTR user_gpio_output(unsigned char gpio_no,BOOL bit_value)
{
	GPIO_OUTPUT_SET(gpio_no,bit_value);
}

unsigned char ICACHE_FLASH_ATTR user_gpio_input(unsigned char gpio_no)
{
	unsigned char res = 0;
	res = GPIO_INPUT_GET(gpio_no);
	return res;
}

void ICACHE_FLASH_ATTR user_wifi_led_low_shark(void)
{
	user_wifi_led_timer_init(500);
}

void ICACHE_FLASH_ATTR user_wifi_led_high_shark(void)
{
	user_wifi_led_timer_init(100);
}

LOCAL void ICACHE_FLASH_ATTR user_wifi_led_timer_cb(void)
{
    led_level = (~led_level) & 0x01;
    user_gpio_output(WIFI_LED_PIN,led_level);
}

void ICACHE_FLASH_ATTR user_wifi_led_timer_init(unsigned int m_time)
{
    os_timer_disarm(&led_timer);
    os_timer_setfn(&led_timer, (os_timer_func_t *)user_wifi_led_timer_cb, NULL);
    os_timer_arm(&led_timer, m_time, 1);
}

void ICACHE_FLASH_ATTR user_wifi_led_stop(void)
{
	os_timer_disarm(&led_timer);
	user_gpio_output(WIFI_LED_PIN, 1);
}

void ICACHE_FLASH_ATTR user_wifi_led_start(void)
{
	os_timer_disarm(&led_timer);
	user_gpio_output(WIFI_LED_PIN,0);
}

void ICACHE_FLASH_ATTR user_network_ready(void)
{
	user_gpio_output(READY_PIN,0);
}

void ICACHE_FLASH_ATTR user_network_unready(void)
{
	user_gpio_output(READY_PIN, 1);
}


unsigned char ICACHE_FLASH_ATTR user_is_restore_set(void)
{
	return user_gpio_input(RESTORE_PIN);
}



void ICACHE_FLASH_ATTR user_gpio_init(void)
{
	//GPIO0 GPIO2 GPIO4 GPIO5 GPIO12 GPIO13 GPIO14 GPIO15
	//GPIO0   WiFi LED
	//GPIO4   READY
	//GPIO5   RESTORE
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U,FUNC_GPIO0);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U,FUNC_GPIO5);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U,FUNC_GPIO13);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U,FUNC_GPIO14);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U,FUNC_GPIO15);
	user_gpio_output(0,1);
	user_gpio_output(2,1);
	user_gpio_output(4,1);
	user_gpio_output(5,1);
	user_gpio_output(12,1);
	user_gpio_output(13,1);
	user_gpio_output(14,1);
	user_gpio_output(15,1);
	user_network_unready();
	user_is_restore_set();
}

