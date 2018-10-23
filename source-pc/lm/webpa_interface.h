/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

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

/**
 * @brief To get device CM MAC by querying stack
 * @return deviceMAC
*/
char * getFullDeviceMac();
#ifdef PARODUS_ENABLE 
void initparodusTask();
#endif

#endif /* WEB_INTERFACE_H_ */
