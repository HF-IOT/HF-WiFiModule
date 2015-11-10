#ifndef __USER_GPIO_H__
#define __USER_GPIO_H__

#include "user_config.h"


#define WIFI_LED_PIN 0
#define READY_PIN 5
#define RESTORE_PIN 4






void user_gpio_output(unsigned char gpio_no,BOOL bit_value);
unsigned char user_gpio_input(unsigned char gpio_no);

void user_wifi_led_low_shark(void);
void user_wifi_led_high_shark(void);
void user_wifi_led_stop(void);
void user_wifi_led_start(void);
void user_gpio_init(void);

void user_network_ready(void);
void user_network_unready(void);
unsigned char user_is_restore_set(void);

#endif

