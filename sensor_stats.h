#ifndef  __SENSORS_STATS_H__
#define  __SENSORS_STATS_H__

typedef struct mobile_sensor_stats{
	int pkts_sent;
	int pkts_recv;
	int pkts_err;
} sensor_stats_t;

void update_stats_pkts_recv(unsigned int pkts);
void update_stats_pkts_sent(unsigned int pkts);
int update_stats_pkts_err(unsigned int pkts);
void get_sensor_stats(sensor_stats_t *_stats);

#endif
