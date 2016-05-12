/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#ifndef NETWORK_DEVICES_STATUS_AVROPACK_H
#define NETWORK_DEVICES_STATUS_AVROPACK_H

#include <sys/time.h>

#if (defined SIMULATION)
#define NETWORK_DEVICE_STATUS_AVRO_FILENAME			"NetworkDevicesStatus.avsc"
#else
#define NETWORK_DEVICE_STATUS_AVRO_FILENAME			"/usr/ccsp/lm/NetworkDevicesStatus.avsc"
#endif
#define CHK_AVRO_ERR (strlen(avro_strerror()) > 0)

struct networkdevicestatusdata
{
struct timeval timestamp;
char* device_mac;
char* interface_name;
BOOL is_active;

struct networkdevicestatusdata *next;
};

/**
 * @brief To send the network devices report to webpa
*/
void network_devices_status_report(struct networkdevicestatusdata *ptr);

#endif /* !NETWORK_DEVICES_STATUS_AVROPACK_H */
