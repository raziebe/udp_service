#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "hs_config_file_reader.h"
#include "hs_udp_service.h"
#include "service_check.h"
#include "spi_config.h"
#include "hslog/hslog.h"


void read_spi_conf(void)
{
	extern int service_disabled;	
	conf_data_t conf;

	_read_config(&conf);
	
	conf.service_id = 0;
	hslog_info("SERVICE ID=%s\n",
		conf.service_id == SERVICE_ID_BLE ? "BLE" : "UDP" );
	
	switch (conf.service_id){

		case SERVICE_ID_BLE:
			service_disabled = 1;
			break;

		case SERVICE_ID_UDP:
			service_disabled = 0;
			break;
		default:
			service_disabled = 1;
	}
}

void parse_options(int argc, char *argv[])
{
        int opt;

        while ((opt = getopt(argc, argv, "i:")) != -1) {
                switch (opt) {
                case 'i':
                        SetConfigFileName(optarg);
			break;
                default:        /* '?' */
                        hslog_error( "Usage: %s -i <config filename>\n",
                                argv[0]);
                        exit(EXIT_FAILURE);
                }
        }
}


int main(int argc, char *argv[])
{
    int res = 0;

    HSlogConfig slgCfg;

    hslog_init( UDP_SERVICE_LOG , "/etc/" SERVICE_LOG_CFG, 0, 1);
 
    hslog_set_module("UDP-service");
    hslog_config_get(&slgCfg);
    slgCfg.nToFile = 0;
    slgCfg.nPretty = 1;
    hslog_config_set(&slgCfg);
    init_network_log("227.0.0.1", NULL, 5555);

    parse_options(argc, argv); 
    read_spi_conf();

    res = start_service_check(SERV_CTRL_SOCKNAME);
    if(res != 0){
        hslog_error("Failed to start UDP service check.\n");
        return -1;
    }

    res =  start_inotify_service("/etc", SERVICE_LOG_CFG);
    if(res != 0){
        hslog_error("Failed to start inotify service.\n");
        return -1;
    }

    res = start_udp_service();
    if(res != 0){
        hslog_error("Failed to start UDP service.\n");
        return -1;
    }

    while(1){
        sleep(6);
    }

    return 0;
}
