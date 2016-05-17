/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#ifndef NETWORK_DEVICES_TRAFFIC_AVROPACK_H
#define NETWORK_DEVICES_TRAFFIC_AVROPACK_H

#include <sys/time.h>

#if (defined SIMULATION)
#define NETWORK_DEVICE_TRAFFIC_AVRO_FILENAME			"NetworkDevicesTraffic.avsc"
#else
#define NETWORK_DEVICE_TRAFFIC_AVRO_FILENAME			"/usr/ccsp/lm/NetworkDevicesTraffic.avsc"
#endif
#define CHK_AVRO_ERR (strlen(avro_strerror()) > 0)

struct networkdevicetrafficdata
{
struct timeval timestamp;
char* device_mac;
ULONG external_bytes_up;
ULONG external_bytes_down;

struct networkdevicetrafficdata *next;
};

/**
 * @brief To send the network devices traffic report to webpa
*/
void network_devices_traffic_report(struct networkdevicetrafficdata *ptr);

#endif /* !NETWORK_DEVICES_TRAFFIC_AVROPACK_H */
