#ifndef __USER_DEVICE_H__
#define __USER_DEVICE_H__

/* you can change to other sector if you use other size spi flash. */
//                              00 - FF
//                              7D  7E  7F 存储1 存储2 flag
#define ESP_PARAM_START_SEC		0x7D

#define packet_size   (2 * 1024)

#define token_size 41


//一定要 4字节对其
struct esp_platform_saved_param {
    uint8 devkey[40];
    uint8 token[40];
    uint8 activeflag;
    uint8 pad[3];
	unsigned int wifiwork_mode;
	unsigned int wificonfigflag;
	unsigned char ip[4];
	unsigned int port;
	unsigned char SSID[64];
	unsigned char PWD[64];
};

enum {
    DEVICE_CONNECTING = 40,
    DEVICE_ACTIVE_DONE,
    DEVICE_ACTIVE_FAIL,
    DEVICE_CONNECT_SERVER_FAIL
};

struct dhcp_client_info {
	ip_addr_t ip_addr;
	ip_addr_t netmask;
	ip_addr_t gw;
	uint8 flag;
	uint8 pad[3];
};
#endif
