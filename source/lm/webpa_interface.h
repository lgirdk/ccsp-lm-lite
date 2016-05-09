/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#ifndef WEB_INTERFACE_H_
#define WEB_INTERFACE_H_

/**
 * @brief To send message to webpa and further upstream
 *
 * @param[in] serviceName Name of component/service trying to send message upstream, sending entity
 * @param[in] dest Destination to identify the type of upstream message or receiving entity
 * @param[in] trans_id Transaction UUID unique identifier for the message/transaction
 * @param[in] payload The actual message data
 * @param[in] contentType content type of message "application/json", "avro/binary"
 * @param[in] payload_len length of payload or message length
 */
void sendWebpaMsg(char *serviceName, char *dest, char *trans_id, char *contentType, char *payload, unsigned int payload_len);

/**
 * @brief To get device CM MAC by querying stack
 * @return deviceMAC
*/
char * getDeviceMac();

#endif /* WEB_INTERFACE_H_ */
