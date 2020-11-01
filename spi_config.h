#ifndef  __SPI_CONFIG_H__
#define  __SPI_CONFIG_H__

#define SSID_MAX_SIZE 32
#define WPA2_PSWD_MAX_SIZE 63

#define KEEP_ALIVE_TIMEOUT_SEC 3
#define SELF_ADVTISE_INTERVAL 3

#define SWU_MULTICAST_GROUP_IP	 "227.0.0.0"
#define SWU_MULTICAST_GROUP_PORT 12225
#define SWU_TX_RX_PORT		 12226

#define OPC_REGISTRATION_ONLY	0
#define OPC_CONNECT_ONLY	1
#define OPC_REG_CON		2

#define MAKE_UINT16_T(high, low) (((((uint16_t)(high)) & 0xff) << 8) + (((uint16_t)(low)) & 0xff))


#define SSID_TAG_CHAR 's'
#define SSID_START_TAG "<s>"
#define SSID_END_TAG "</s>"

#define WPA2_PSWD_TAG_CHAR 'p'
#define WPA2_PSWD_START_TAG "<p>"
#define WPA2_PSWD_END_TAG "</p>"

#define BLE_ADDR_TAG_CHAR 'b'
#define BLE_ADDR_START_TAG "<b>"
#define BLE_ADDR_END_TAG "</b>"

#define SERVICE_TAG_CHAR 'v'
#define SERVICE_START_TAG "<v>"
#define SERVICE_END_TAG "</v>"

#define SATID_TAG_CHAR 'i'
#define SATID_START_TAG "<i>"
#define SATID_END_TAG "</i>"

#define START_TAG_LEN 3
#define END_TAG_LEN 4

#define SSID_BUFF_SIZE (START_TAG_LEN + SSID_MAX_SIZE + END_TAG_LEN)
#define WPA2_PSWD_BUFF_SIZE (START_TAG_LEN + WPA2_PSWD_MAX_SIZE + END_TAG_LEN)
#define BEAM_NAME_MAX_SIZE (START_TAG_LEN + 32 + 4 + END_TAG_LEN)
#define BLE_ADDR_SIZE 6


#define CONFIG_MTD_DEV_0	"/dev/mtd0"
#define CONFIG_MTD_DEV_1	"/dev/mtd1"


#define SERVICE_ID_UDP	0x00
#define SERVICE_ID_BLE	0x01

typedef struct conf_data {
        char ssid[SSID_BUFF_SIZE];
        char wpa2_pswd[WPA2_PSWD_BUFF_SIZE];
        char selected_beam[BEAM_NAME_MAX_SIZE];
        uint8_t ble_addr[BLE_ADDR_SIZE];
        uint8_t wpa2_pswd_len;
        uint8_t ssid_len;
        uint8_t beam_len;
        // uint16_t sat_id;
        uint8_t service_id;
        uint8_t conf_status;
} conf_data_t;


int _read_config(conf_data_t *conf_data);
int read_config1(char *buff, uint32_t buff_size); 

#endif

