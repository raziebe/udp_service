// libc includes
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

// linux includes
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "spi_config.h"
#include "hslog/hslog.h"


int _read_config(conf_data_t *conf_data)
{
        char buff[1024] = {};
        char *p = NULL;
        char *saveptr = NULL;

        if (read_config1(buff, 1024) != 0)
                return -1;

        p = strtok_r(buff, "<", &saveptr);
        do {       
                switch (*p) {
                        case SSID_TAG_CHAR:
                                p += 2; 
                                conf_data->ssid_len = (uint8_t)strlen(p);
                                memcpy(conf_data->ssid, p, conf_data->ssid_len);
                                break;
                        case WPA2_PSWD_TAG_CHAR:
                                p += 2;
                                conf_data->wpa2_pswd_len = (uint8_t)strlen(p);
                                memcpy(conf_data->wpa2_pswd, p, conf_data->wpa2_pswd_len);
                                break;
                        case BLE_ADDR_TAG_CHAR:
                                p += 2;
                                memcpy(conf_data->ble_addr, p, BLE_ADDR_SIZE);
                                break;
                        case SERVICE_TAG_CHAR:
                                p += 2;
                                conf_data->service_id = (uint8_t)atoi(p);
                                break;
                        case SATID_TAG_CHAR:
                                p += 2;
                                conf_data->beam_len = (uint8_t)strlen(p);
                                memcpy(&conf_data->selected_beam, p, conf_data->beam_len);
                                break;
                        default:
                                break;
                }
        } while ((p = strtok_r(NULL, "<", &saveptr)));

        return 0;
}


int read_config(const char *dev, char *buff, uint32_t buff_size)
{
	int fd;

	memset(buff, 0, buff_size);
	if ((fd = open(dev, O_RDWR)) < 0) {
		hslog_error("%s: open config %s failed. err:%s\n", 
			__func__, dev, strerror(errno));
		return -1;
	}

	if (read(fd, buff, buff_size) < 0) {
		hslog_error("%s: read config failed. err:%s\n", __func__, strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int read_config1(char *buff, uint32_t buff_size) 
{
	return read_config(CONFIG_MTD_DEV_1, buff, buff_size);
}

#ifdef DEBUG
int main(int argc, char *argv[])
{
	conf_data_t conf;

	_read_config(&conf);
	hslog_info("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
		" SERVICE ID=%s\n",
		conf.ble_addr[0],
		conf.ble_addr[1],
		conf.ble_addr[2],
		conf.ble_addr[3],
		conf.ble_addr[4],
		conf.ble_addr[5],
		conf.service_id == SERVICE_ID_BLE ? "BLE" : "UDP" );

}
#endif
