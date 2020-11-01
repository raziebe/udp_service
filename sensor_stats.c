#include "sensor_stats.h"

sensor_stats_t stats = {0};

void update_stats_pkts_recv(unsigned int pkts)
{
	stats.pkts_sent += pkts;
}

void update_stats_pkts_sent(unsigned int pkts)
{
	stats.pkts_recv += pkts;
}


int update_stats_pkts_err(unsigned int pkts)
{
	int last = stats.pkts_err;

	stats.pkts_err += pkts;
	return stats.pkts_err - last;
}

void get_sensor_stats(sensor_stats_t *_stats)
{
	*_stats = stats;
}
