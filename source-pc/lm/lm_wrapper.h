/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

#ifndef _LM_WRAPPER_H_
#define _LM_WRAPPER_H_

#include <sys/socket.h>
#include <lm_api.h>
#include "moca_hal.h"
#include "ccsp_custom_logs.h"
/*****************
 *
 */
//#define LM_NETWORK_NAME_SIZE 32
#define LM_MAX_INTERFACE_NUMBER 10

#define DEVICE_WIFI_ACCESS_POINT "Device.WiFi.AccessPoint." 
#define ARP_CACHE_FILE "/var/arp_cache"
#define DNSMASQ_LEASES_FILE "/tmp/dnsmasq.leases"
#define DNSMASQ_RESERVED_FILE "/etc/dhcp_static_hosts"

//#define DEBUG
#ifdef DEBUG
#define PRINTD(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define PRINTD(fmt, args...)
#endif

/*****************
 * Moca header file will include some private path
 * We cannot include these file, so ...
 */
#define kMoca_MaxCpeList 256
#define STATUS int

typedef struct _moca_cpe_list
{
   unsigned char mac_addr[6];
} moca_cpe_list;

/*****************
 *
 */
enum LM_NEIGHBOR_STATE{
	LM_NEIGHBOR_STATE_REACHABLE = 0,
	LM_NEIGHBOR_STATE_STALE,
	LM_NEIGHBOR_STATE_DELAY,
	LM_NEIGHBOR_STATE_PROBE
};

/*****************
 *
 */
struct arp_pkt {
    unsigned char ether_dhost[6];
    unsigned char ether_shost[6];
    unsigned short ether_type;
    unsigned short hw_type;
    unsigned short pro_type;
    unsigned char hw_size;
    unsigned char pro_size;
    unsigned short opcode;
    unsigned char sMac[6];
    unsigned char sIP[4];
    unsigned char tMac[6];
    unsigned char tIP[4];   
};

typedef struct {
    unsigned char phyAddr[18];
    unsigned char ssid[LM_GEN_STR_SIZE];
    unsigned char AssociatedDevice[LM_GEN_STR_SIZE];
    int RSSI;
}LM_wifi_wsta_t;

typedef struct {
    unsigned char phyAddr[18];
    unsigned char ncId[LM_GEN_STR_SIZE];
}LM_moca_cpe_t;


typedef struct _ClientInfo
{

char* MAC_Address;
char* SSID_Type;
char* Device_Name;
char* SSID_Name;
char* RSSI;
char* RxRate;
char* TxRate;

struct _ClientInfo* next;

} ClientInfo;

typedef struct _ClientInfoLists
{
int numClient;
ClientInfo* connectedDeviceList;

} ClientInfoLists;

//RDKB-7592
typedef struct _Ssid
{
int     index;
char*   name;
char*   bssid;
char*   band;

/* for future use
int                             channel;
char*                   mode;
security_mode*  security_mode;
encrption_type* encryption;
*/
struct _Ssid* next;
} Ssid;

typedef struct _ExtenderInfo
{

char* extender_ip;
char* client_info_result;
ClientInfoLists* list;
Ssid*   ssid_list;
int     ssid_count;
} ExtenderInfo;

typedef struct _ExtenderList
{
ExtenderInfo* info;
struct _ExtenderList* next;
} ExtenderList;


typedef struct{
	unsigned char phyAddr[18];
	unsigned char ipAddr[64];
	enum LM_NEIGHBOR_STATE status;
    unsigned char hostName[LM_GEN_STR_SIZE];
    unsigned char ifName[LM_GEN_STR_SIZE];
    int LeaseTime;
}LM_host_entry_t;



extern ANSC_HANDLE bus_handle;

#define  CcspTraceBaseStr(arg ...)                                                             \
            do {                                                                            \
                snprintf(pTempChar1, 4095, arg);                                         	\
            } while (FALSE)
			
#define  CcspWifiTrace(msg)                         \
{\
	                char   pTempChar1[4096];                                     \
			char	sfn[32];														 \
                    	CcspTraceBaseStr msg;                                                       \
			WriteLog(pTempChar1,bus_handle,"eRT.","Device.LogAgent.WifiLogMsg"); \
}

/******************
 *
 */
int lm_wrapper_init();
int lm_arping_v4_send(char netName[64], char strMac[17], unsigned char ip[4]);
int lm_wrapper_get_moca_cpe_list(char netName[LM_NETWORK_NAME_SIZE], int *pCount, LM_moca_cpe_t **ppArray);
int lm_wrapper_get_wifi_wsta_list(char netName[LM_NETWORK_NAME_SIZE], int *pCount, LM_wifi_wsta_t **ppWstaArray);
int lm_wrapper_get_arp_entries (char netName[LM_NETWORK_NAME_SIZE], int *pCount, LM_host_entry_t **ppArray);
void lm_wrapper_get_dhcpv4_client();
void lm_wrapper_get_dhcpv4_reserved();

int mac_string_to_array(char *pStr, unsigned char array[6]);
int ip_string_to_arrary(char* pStr, unsigned char array[4]);
void getAddressSource(char *physAddress, char *pAddressSource);
int getIPAddress(char *physAddress,char *IPAddress);

#endif
