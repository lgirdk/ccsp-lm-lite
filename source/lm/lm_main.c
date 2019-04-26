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


/**************************************************************************

    module: cosa_apis_hosts.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        09/16/2011    initial revision.

**************************************************************************/

#include <time.h>
#include <sys/sysinfo.h>


#include "ansc_platform.h"
#include "ccsp_base_api.h"
#include "lm_main.h"
#include "lm_util.h"
#include "webpa_interface.h"
#include "lm_wrapper.h"
#include "lm_api.h"
#include "lm_wrapper_priv.h"
#include "ccsp_lmliteLog_wrapper.h"

#define LM_IPC_SUPPORT
#include "ccsp_dm_api.h"

#define NAME_DM_LEN  257

#define STRNCPY_NULL_CHK1(dest, src) { if((dest) != NULL ) AnscFreeMemory((dest));\
                                           (dest) = _CloneString((src));}


#define DNS_LEASE "/nvram/dnsmasq.leases"
#define DEBUG_INI_NAME  "/etc/debug.ini"
#define HOST_ENTRY_LIMIT 250
#define ARP_IPv6 0
#define DIBBLER_IPv6 1

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <mqueue.h>

#define EVENT_QUEUE_NAME  "/Event_queue"

#define MAX_SIZE    2048
#define MAX_SIZE_EVT    1024

#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            return; \
        } \
    } while (0) \

#define MSG_TYPE_EMPTY  0
#define MSG_TYPE_ETH    1
#define MSG_TYPE_WIFI   2
#define MSG_TYPE_MOCA   3

#define VALIDATE_QUEUE_NAME             "/Validate_host_queue"
#define MAX_SIZE_VALIDATE_QUEUE         sizeof(ValidateHostQData)
#define MAX_COUNT_VALIDATE_RETRY        (3)
#define MAX_WAIT_VALIDATE_RETRY         (15)
#define ARP_CACHE                       "/tmp/arp.txt"
#define DNSMASQ_CACHE                   "/tmp/dns.txt"
#define DNSMASQ_FILE                    "/nvram/dnsmasq.leases"
#define ACTION_FLAG_ADD                 (1)
#define ACTION_FLAG_DEL                 (2)

typedef enum {
    CLIENT_STATE_OFFLINE,
    CLIENT_STATE_DISCONNECT,
    CLIENT_STATE_ONLINE,
    CLIENT_STATE_CONNECT
} ClientConnectState;

typedef struct _EventQData 
{
    char Msg[MAX_SIZE_EVT];
    int MsgType; // Ethernet = 1, WiFi = 2, MoCA = 3
}EventQData;

typedef struct _Eth_data
{
    char MacAddr[18];
    int Active; // Online = 1, offline = 0
}Eth_data;

typedef struct _Name_DM 
{
    char name[NAME_DM_LEN];
    char dm[NAME_DM_LEN];
}Name_DM_t;

typedef struct _ValidateHostQData
{
    char phyAddr[18];
    char AssociatedDevice[LM_GEN_STR_SIZE];
    char ssid[LM_GEN_STR_SIZE];
    int RSSI;
    int Status;
} ValidateHostQData;

typedef struct _RetryHostList
{
    ValidateHostQData host;
    int retryCount;
    struct _RetryHostList *next;
} RetryHostList;

RetryHostList *pListHead = NULL;

int g_IPIfNameDMListNum = 0;
Name_DM_t *g_pIPIfNameDMList = NULL;

int g_MoCAADListNum = 0;
Name_DM_t *g_pMoCAADList = NULL;

int g_DHCPv4ListNum = 0;
Name_DM_t *g_pDHCPv4List = NULL;

static int firstFlg = 0;
static int xfirstFlg = 0;


extern int bWifiHost;

extern char*                                pComponentName;

int g_Client_Poll_interval;

/***********************************************************************
 IMPORTANT NOTE:

 According to TR69 spec:
 On successful receipt of a SetParameterValues RPC, the CPE MUST apply
 the changes to all of the specified Parameters atomically. That is, either
 all of the value changes are applied together, or none of the changes are
 applied at all. In the latter case, the CPE MUST return a fault response
 indicating the reason for the failure to apply the changes.

 The CPE MUST NOT apply any of the specified changes without applying all
 of them.

 In order to set parameter values correctly, the back-end is required to
 hold the updated values until "Validate" and "Commit" are called. Only after
 all the "Validate" passed in different objects, the "Commit" will be called.
 Otherwise, "Rollback" will be called instead.

 The sequence in COSA Data Model will be:

 SetParamBoolValue/SetParamIntValue/SetParamUlongValue/SetParamStringValue
 -- Backup the updated values;

 if( Validate_XXX())
 {
     Commit_XXX();    -- Commit the update all together in the same object
 }
 else
 {
     Rollback_XXX();  -- Remove the update at backup;
 }

***********************************************************************/
#define LM_HOST_OBJECT_NAME_HEADER  "Device.Hosts.Host."
#define LM_HOST_RETRY_LIMIT         30

//#define TIME_NO_NEGATIVE(x) ((long)(x) < 0 ? 0 : (x))

#define STRNCPY_NULL_CHK(x, y, z) if((y) != NULL) strncpy((x),(y),(z)); else  *(unsigned char*)(x) = 0;

LmObjectHosts lmHosts = {
    .pHostBoolParaName = {"Active"},
    .pHostIntParaName = {"X_CISCO_COM_ActiveTime", "X_CISCO_COM_InactiveTime", "X_CISCO_COM_RSSI"},
    .pHostUlongParaName = {"X_CISCO_COM_DeviceType", "X_CISCO_COM_NetworkInterface", "X_CISCO_COM_ConnectionStatus", "X_CISCO_COM_OSType"},
    .pHostStringParaName = {"Alias", "PhysAddress", "IPAddress", "DHCPClient", "AssociatedDevice", "Layer1Interface", "Layer3Interface", "HostName",
                                        "X_CISCO_COM_UPnPDevice", "X_CISCO_COM_HNAPDevice", "X_CISCO_COM_DNSRecords", "X_CISCO_COM_HardwareVendor",
                                        "X_CISCO_COM_SoftwareVendor", "X_CISCO_COM_SerialNumbre", "X_CISCO_COM_DefinedDeviceType",
                                        "X_CISCO_COM_DefinedHWVendor", "X_CISCO_COM_DefinedSWVendor", "AddressSource", "Comments",
                                    	"X_RDKCENTRAL-COM_Parent", "X_RDKCENTRAL-COM_DeviceType", "X_RDKCENTRAL-COM_Layer1Interface" },
    .pIPv4AddressStringParaName = {"IPAddress"},
    .pIPv6AddressStringParaName = {"IPAddress"}
};

LmObjectHosts XlmHosts = {
    .pHostBoolParaName = {"Active"},
    .pHostIntParaName = {"X_CISCO_COM_ActiveTime", "X_CISCO_COM_InactiveTime", "X_CISCO_COM_RSSI"},
    .pHostUlongParaName = {"X_CISCO_COM_DeviceType", "X_CISCO_COM_NetworkInterface", "X_CISCO_COM_ConnectionStatus", "X_CISCO_COM_OSType"},
    .pHostStringParaName = {"Alias", "PhysAddress", "IPAddress", "DHCPClient", "AssociatedDevice", "Layer1Interface", "Layer3Interface", "HostName",
                                        "X_CISCO_COM_UPnPDevice", "X_CISCO_COM_HNAPDevice", "X_CISCO_COM_DNSRecords", "X_CISCO_COM_HardwareVendor",
                                        "X_CISCO_COM_SoftwareVendor", "X_CISCO_COM_SerialNumbre", "X_CISCO_COM_DefinedDeviceType",
                                        "X_CISCO_COM_DefinedHWVendor", "X_CISCO_COM_DefinedSWVendor", "AddressSource", "Comments",
                                    	"X_RDKCENTRAL-COM_Parent", "X_RDKCENTRAL-COM_DeviceType", "X_RDKCENTRAL-COM_Layer1Interface" },
    .pIPv4AddressStringParaName = {"IPAddress"},
    .pIPv6AddressStringParaName = {"IPAddress"}
};


/* It may be updated by different threads at the same time? */
ULONG HostsUpdateTime = 0;
ULONG XHostsUpdateTime = 0;

pthread_mutex_t HostNameMutex;
pthread_mutex_t PollHostMutex;
pthread_mutex_t LmHostObjectMutex;
pthread_mutex_t XLmHostObjectMutex;
pthread_mutex_t LmRetryHostListMutex;

static void Wifi_ServerSyncHost(char *phyAddr, char *AssociatedDevice, char *ssid, int RSSI, int Status);


#ifdef USE_NOTIFY_COMPONENT

extern ANSC_HANDLE bus_handle;
void DelAndShuffleAssoDevIndx(PLmObjectHost pHost);
int extract(char* line, char* mac, char * ip);
void Add_IPv6_from_Dibbler();

void Send_Notification(char* interface, char*mac , ClientConnectState status, char *hostname)
{

	char  str[500] = {0};
	parameterValStruct_t notif_val[1];
	char param_name[256] = "Device.NotifyComponent.SetNotifi_ParamName";
	char compo[256] = "eRT.com.cisco.spvtg.ccsp.notifycomponent";
	char bus[256] = "/com/cisco/spvtg/ccsp/notifycomponent";
	char* faultParam = NULL;
	int ret = 0;
	char status_str[16]={0};
	

	CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
	switch (status) {
	case CLIENT_STATE_OFFLINE:
	    strcpy(status_str,"Offline");
	    break;
    case CLIENT_STATE_DISCONNECT:
        strcpy(status_str,"Disconnected");
        break;
    case CLIENT_STATE_ONLINE:
        strcpy(status_str,"Online");
        break;
    case CLIENT_STATE_CONNECT:
        strcpy(status_str,"Connected");
        break;
    default:
        break;
	}
	
	if(mac && strlen(mac))
	{
		
	snprintf(str,sizeof(str)/sizeof(str[0]),"Connected-Client,%s,%s,%s,%s",
										interface!=NULL ? (strlen(interface)>0 ? interface:"NULL" ): "NULL",
										mac!=NULL ? (strlen(mac)>0 ? mac:"NULL") : "NULL",
										status_str!=NULL ? (strlen(status_str)>0 ? status_str:"NULL") : "NULL",
										hostname!=NULL ? (strlen(hostname)>0 ?hostname:"NULL"):"NULL");

	notif_val[0].parameterName =  param_name ;
	notif_val[0].parameterValue = str;
	notif_val[0].type = ccsp_string;

	ret = CcspBaseIf_setParameterValues(
		  bus_handle,
		  compo,
		  bus,
		  0,
		  0,
		  notif_val,
		  1,
		  TRUE,
		  &faultParam
		  );

	if(ret != CCSP_SUCCESS)
        {
		CcspTraceWarning(("\n LMLite <%s> <%d >  Notification Failure %d \n",__FUNCTION__,__LINE__, ret));
                if(faultParam)
                {
                        bus_info->freefunc(faultParam);
                }
        } 	
        }
	else
	{
		CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: MacAddress is NULL, hence Connected-Client notifications are not sent\n"));
		//printf("RDKB_CONNECTED_CLIENTS: MacAddress is NULL, hence Connected-Client notifications are not sent\n");
	}

}
int FindHostInLeases(char *Temp, char *FileName)
{
	FILE *fp = NULL;
	char buf[200] = {0};
	int ret = 0;
	
	if ( (fp=fopen(FileName, "r")) == NULL )
	{
		return 1;
	}

	while ( fgets(buf, sizeof(buf), fp)!= NULL )
	{
	
		if(strstr(buf,Temp))
		{
			
			ret = 0;
			
			break;
		}
		else
		{
			ret = 1;
		}


	}
	fclose(fp);
	return ret; 
}
#endif
int logOnlineDevicesCount()
{
	PLmObjectHost   pHost      = NULL;
	int NumOfOnlineDevices = 0;
	int i;
	for ( i = 0; i < lmHosts.numHost; i++ )
	{               
		pHost = lmHosts.hostArray[i];

		if(pHost->bBoolParaValue[LM_HOST_ActiveId])
		{
			NumOfOnlineDevices ++;
		}
	}
	CcspTraceWarning(("CONNECTED_CLIENTS_COUNT : %d \n",NumOfOnlineDevices));
	return NumOfOnlineDevices;
}

#define LM_SET_ACTIVE_STATE_TIME(x, y) LM_SET_ACTIVE_STATE_TIME_(__LINE__, x, y)
static void LM_SET_ACTIVE_STATE_TIME_(int line, LmObjectHost *pHost,BOOL state){
	char interface[32] = {0};
	int uptime = 0;
    if(pHost->bBoolParaValue[LM_HOST_ActiveId] != state){

        char addressSource[20] = {0};
	char IPAddress[50] = {0};
	memset(addressSource,0,sizeof(addressSource));
	memset(IPAddress,0,sizeof(IPAddress));
	memset(interface,0,sizeof(interface));
		if ( ! pHost->pStringParaValue[LM_HOST_IPAddressId] )	
		{
			 getIPAddress(pHost->pStringParaValue[LM_HOST_PhysAddressId], IPAddress);
      			  LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_IPAddressId]) , IPAddress);
		}
/*
		getAddressSource(pHost->pStringParaValue[LM_HOST_PhysAddressId], addressSource);
		if ( (pHost->pStringParaValue[LM_HOST_AddressSource]) && (strlen(addressSource)))	
		{
   		       LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AddressSource]) , addressSource);
		}
*/
	if(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] != NULL)
	{
		if((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi"))) 
		{
			if(state) 
			{
				if(pHost->ipv4Active == TRUE)
				{
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is WiFi, MacAddress is %s and HostName is %s appeared online\n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));

				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: IP Address is  %s , address source is %s and HostName is %s \n",pHost->pStringParaValue[LM_HOST_IPAddressId],pHost->pStringParaValue[LM_HOST_AddressSource],pHost->pStringParaValue[LM_HOST_HostNameId]));
				}
			}  
			else 
			{
				if(pHost->ipv4Active == TRUE)
				{
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Wifi client with %s MacAddress and %s HostName gone offline\n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
				CcspWifiTrace(("RDK_LOG_WARN: Wifi client with %s MacAddress and %s HostName gone offline \n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
				}
#ifndef USE_NOTIFY_COMPONENT
				remove_Mac_to_band_mapping(pHost->pStringParaValue[LM_HOST_PhysAddressId]);
#endif
			}
			strcpy(interface,"WiFi");
		}
#ifndef _CBR_PRODUCT_REQ_ 
		else if ((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"MoCA")))
		{
			if(pHost->ipv4Active == TRUE)
			{
				  if(state) {
					CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is MoCA, MacAddress is %s and HostName is %s appeared online \n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
					CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: IP Address is  %s , address source is %s and HostName is %s \n",pHost->pStringParaValue[LM_HOST_IPAddressId],pHost->pStringParaValue[LM_HOST_AddressSource],pHost->pStringParaValue[LM_HOST_HostNameId]));
				}  else {
					CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: MoCA client with %s MacAddress and HostName is %s gone offline \n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));

				}
			}
			strcpy(interface,"MoCA");
		}
#endif
		else if ((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"Ethernet")))
		{
			if(pHost->ipv4Active == TRUE)
			{
				  if(state) {
					CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is Ethernet, MacAddress is %s and HostName is %s appeared online \n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
					CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: IP Address is %s , address source is %s and HostName is %s \n",pHost->pStringParaValue[LM_HOST_IPAddressId],pHost->pStringParaValue[LM_HOST_AddressSource],pHost->pStringParaValue[LM_HOST_HostNameId]));
				}  else {
					CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Ethernet client with %s MacAddress and %s HostName gone offline \n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));

				}
			}
			strcpy(interface,"Ethernet");
		}

		else 
		{
		      if(state) {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is %s , MacAddress is %s and HostName is %s appeared online \n",pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
				CcspTraceWarning(("RDK_LOG_WARN,RDKB_CONNECTED_CLIENTS: IP Address is  %s , address source is %s and HostName is %s\n",pHost->pStringParaValue[LM_HOST_IPAddressId],pHost->pStringParaValue[LM_HOST_AddressSource],pHost->pStringParaValue[LM_HOST_HostNameId]));
			}  else {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:  client with %s MacAddress and %s HostName gone offline \n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));

			}
			strcpy(interface,"Other");
		}
        pHost->bBoolParaValue[LM_HOST_ActiveId] = state;
        pHost->activityChangeTime = time((time_t*)NULL);
		logOnlineDevicesCount();

	}
	PRINTD("%d: mac %s, state %d time %d\n",line ,pHost->pStringParaValue[LM_HOST_PhysAddressId], state, pHost->activityChangeTime);
    }
	#ifdef USE_NOTIFY_COMPONENT
	if(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] != NULL)
	{
		if((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi"))) {
			strcpy(interface,"WiFi");
		}
		else if ((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"MoCA")))
		{
			strcpy(interface,"MoCA");
		}
		else if ((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"Ethernet")))
		{
			strcpy(interface,"Ethernet");
		}
		else
		{
			strcpy(interface,"Other");
		}
	
		if(state == FALSE)
		{
			
            #if 0
            if(FindHostInLeases(pHost->pStringParaValue[LM_HOST_PhysAddressId], DNS_LEASE))
            {

                if(pHost->ipv4Active == TRUE)
                {
                    if(pHost->bNotify == TRUE)
                    {
                        CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is %s, MacAddress is %s Disconnected \n",interface,pHost->pStringParaValue[LM_HOST_PhysAddressId]));
                        lmHosts.lastActivity++;
                        Send_Notification(interface, pHost->pStringParaValue[LM_HOST_PhysAddressId], CLINET_STATE_DISCONNECT, pHost->pStringParaValue[LM_HOST_HostNameId]);
                        char buf[12] = {0};
                        snprintf(buf,sizeof(buf)-1,"%lu",lmHosts.lastActivity);
                        pHost->ipv4Active = FALSE;
                        if (syscfg_set(NULL, "X_RDKCENTRAL-COM_HostVersionId", buf) != 0)
                        {
                            AnscTraceWarning(("syscfg_set failed\n"));
                        }
                        else
                        {
                            if (syscfg_commit() != 0)
                            {
                                AnscTraceWarning(("syscfg_commit failed\n"));
                            }

                        }
                        pHost->bNotify = FALSE;

                    }
                }

            }
            #endif

			#if defined(FEATURE_SUPPORT_MESH)
            // We are going to send offline notifications to mesh when clients go offline.
            if(pHost->bNotify == TRUE)
            {
                //CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is %s, MacAddress is %s Offline \n",interface,pHost->pStringParaValue[LM_HOST_PhysAddressId]));
                Send_Notification(interface, pHost->pStringParaValue[LM_HOST_PhysAddressId], CLIENT_STATE_OFFLINE, pHost->pStringParaValue[LM_HOST_HostNameId]);
            }
			#endif
		}
		else
		{
			
			{
				if(pHost->bNotify == FALSE)
				{
					get_uptime(&uptime);
                  	CcspTraceWarning(("Client_Connect_complete:%d\n",uptime));	
					CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is %s, MacAddress is %s and HostName is %s Connected  \n",interface,pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
					lmHosts.lastActivity++;
					pHost->bClientReady = TRUE;
                    if(pHost->pStringParaValue[LM_HOST_HostNameId])
					{

						if(0 == strcmp(pHost->pStringParaValue[LM_HOST_HostNameId],pHost->pStringParaValue[LM_HOST_PhysAddressId]))
						{
						char HostName[50];
						if(get_HostName(pHost->pStringParaValue[LM_HOST_PhysAddressId],HostName))
						LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_HostNameId]), HostName);

						CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is %s, MacAddress is %s and HostName is %s Connected \n",interface,pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
						}
					}
					//CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:  %s pHost->bClientReady = %d \n",interface,pHost->bClientReady));
					Send_Notification(interface, pHost->pStringParaValue[LM_HOST_PhysAddressId], CLIENT_STATE_CONNECT, pHost->pStringParaValue[LM_HOST_HostNameId]);
					char buf[12] = {0};
					snprintf(buf,sizeof(buf)-1,"%lu",lmHosts.lastActivity);
					if (syscfg_set(NULL, "X_RDKCENTRAL-COM_HostVersionId", buf) != 0) 
					{
						AnscTraceWarning(("syscfg_set failed\n"));
					}
					else 
					{
						if (syscfg_commit() != 0) 
						{
							AnscTraceWarning(("syscfg_commit failed\n"));
						}
		
					}
					pHost->bNotify = TRUE;
				}
				else
				{
				    // This case is for "Online" events after we have send a connection message. WebPA apparently only wants a
				    // single connect request and no online/offline events.
                    //CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is %s, MacAddress is %s and HostName is %s Online  \n",interface,pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
                    Send_Notification(interface, pHost->pStringParaValue[LM_HOST_PhysAddressId], CLIENT_STATE_ONLINE, pHost->pStringParaValue[LM_HOST_HostNameId]);

				}
			}
			
		}
	}
#endif
} 

#define LM_SET_PSTRINGPARAVALUE(var, val) if((var)) LanManager_Free(var);var = LanManager_CloneString(val);

/***********************************************************************

 APIs for Object:

    Hosts.

    *  Hosts_Init
    *  Hosts_SavePsmValueRecord

***********************************************************************/
static void _getLanHostComments(char *physAddress, char *pComments)
{
    lm_wrapper_priv_getLanHostComments(physAddress, pComments);
    return;
}


static inline BOOL _isIPv6Addr(const char* ipAddr)
{
    if(strchr(ipAddr, ':') != NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
#if 0
void
Hosts_FindHostByIPv4Address
(
    const char *ipv4Addr,
    char hostList[],
    int *hostListSize,
    void * userData,
    enum DeviceType userDataType
)
{
    if(!ipv4Addr) return;
    int i, j, firstOne = 1;
    for(i=0; i<lmHosts.numHost; i++)
    {
        for(j=0; j<lmHosts.hostArray[i]->numIPv4Addr; j++)
        {
            if(AnscEqualString(ipv4Addr, lmHosts.hostArray[i]->ipv4AddrArray[j]->pStringParaValue[LM_HOST_IPAddress_IPAddressId], FALSE))
            {
                if(!firstOne){
                    strcat(hostList, ",");
                    *hostListSize--;
                }
                else firstOne = 0;
                size_t len = strlen(lmHosts.hostArray[i]->objectName);
                if(*hostListSize < len) return;
                strcat(hostList, lmHosts.hostArray[i]->objectName);
                *hostListSize -= len;
                Host_SetExtensionParameters(lmHosts.hostArray[i], userData, userDataType);
                break;
            }
        }
    }
}
#endif
void Hosts_FreeHost(PLmObjectHost pHost){
    int i;
    if(pHost == NULL)
        return;
    for(i=0; i<LM_HOST_NumStringPara; i++)
    {
        if(NULL != pHost->pStringParaValue[i])
            LanManager_Free(pHost->pStringParaValue[i]);
        pHost->pStringParaValue[i] = NULL;
    }
    if(pHost->objectName != NULL)
        LanManager_Free(pHost->objectName);
	if(pHost->Layer3Interface != NULL)
        LanManager_Free(pHost->Layer3Interface);

	pHost->objectName = NULL;
    pHost->Layer3Interface = NULL;
    Host_FreeIPAddress(pHost, 4);
    Host_FreeIPAddress(pHost, 6);

	LanManager_Free(pHost);
	pHost = NULL;

	lmHosts.numHost--;
}

void Hosts_RmHosts(){
    int i;

    if(lmHosts.numHost == 0)
        return;

    for(i = 0; i < lmHosts.numHost; i++){
        Hosts_FreeHost(lmHosts.hostArray[i]);
        lmHosts.hostArray[i] = NULL;
    }
    LanManager_Free(lmHosts.hostArray);
    lmHosts.availableInstanceNum = 1;
    lmHosts.hostArray = NULL;
    lmHosts.numHost = 0;
    lmHosts.sizeHost = 0;
    lmHosts.lastActivity++;
	char buf[12] = {0};
	snprintf(buf,sizeof(buf)-1,"%lu",lmHosts.lastActivity);
	if (syscfg_set(NULL, "X_RDKCENTRAL-COM_HostVersionId", buf) != 0) 
		{
			AnscTraceWarning(("syscfg_set failed\n"));
		}
	else 
		{
			if (syscfg_commit() != 0) 
				{
					AnscTraceWarning(("syscfg_commit failed\n"));
				}
		}

    return;
}

PLmObjectHost XHosts_AddHost(int instanceNum)
{
    //printf("in Hosts_AddHost %d \n", instanceNum);
    PLmObjectHost pHost = LanManager_Allocate(sizeof(LmObjectHost));
    if(pHost == NULL)
    {
        return NULL;
    }
    pHost->instanceNum = instanceNum;
    /* Compose Host object name. */
    char objectName[100] = LM_HOST_OBJECT_NAME_HEADER;
    char instanceNumStr[50] = {0};
    _ansc_itoa(pHost->instanceNum, instanceNumStr, 10);
    strcat(instanceNumStr, ".");
    strcat(objectName, instanceNumStr);
    pHost->objectName = LanManager_CloneString(objectName);

    pHost->ipv4AddrArray = NULL;
    pHost->numIPv4Addr = 0;
    pHost->ipv6AddrArray = NULL;
    pHost->numIPv6Addr = 0;
	pHost->pStringParaValue[LM_HOST_IPAddressId] = NULL;
    /* Default it is inactive. */
    pHost->bBoolParaValue[LM_HOST_ActiveId] = FALSE;
    pHost->ipv4Active = FALSE;
    pHost->ipv6Active = FALSE;
    pHost->activityChangeTime  = time(NULL);

    pHost->iIntParaValue[LM_HOST_X_CISCO_COM_ActiveTimeId] = -1;
    pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = -200;

	pHost->Layer3Interface = NULL;

	memset(pHost->backupHostname,0,64);
    int i;
    for(i=0; i<LM_HOST_NumStringPara; i++) pHost->pStringParaValue[i] = NULL;

    if(XlmHosts.numHost >= XlmHosts.sizeHost){
        XlmHosts.sizeHost += LM_HOST_ARRAY_STEP;
        PLmObjectHost *newArray = LanManager_Allocate(XlmHosts.sizeHost * sizeof(PLmObjectHost));
        for(i=0; i<XlmHosts.numHost; i++){
            newArray[i] = XlmHosts.hostArray[i];
        }
        PLmObjectHost *backupArray = XlmHosts.hostArray;
        XlmHosts.hostArray = newArray;
        if(backupArray) LanManager_Free(backupArray);
    }
    pHost->id = XlmHosts.numHost;
    XlmHosts.hostArray[pHost->id] = pHost;
    XlmHosts.numHost++;
    return pHost;
}

Clean_Host_Table()
{

	if(lmHosts.numHost < HOST_ENTRY_LIMIT)
		return;

	time_t currentTime = time(NULL);
	int count,count1,total_count = lmHosts.numHost;
	for(count=0 ; count < total_count; count++)
	{
		PLmObjectHost pHost = lmHosts.hostArray[count];

		if((pHost->bBoolParaValue[LM_HOST_ActiveId] == FALSE) &&
			(AnscEqualString(pHost->pStringParaValue[LM_HOST_AddressSource], "DHCP", TRUE)) &&
			((pHost->LeaseTime == 0xFFFFFFFF) || (currentTime >= pHost->LeaseTime)))
		{
			CcspTraceWarning((" Freeing Host %s \n",pHost->pStringParaValue[LM_HOST_PhysAddressId]));
			Hosts_FreeHost(pHost);
			lmHosts.hostArray[count] = NULL;
		}
	}

	for(count=0 ; count < total_count; count++)
	{
		if(lmHosts.hostArray[count]) continue;
		for(count1=count+1; count1 < total_count; count1++)
		{
			if(lmHosts.hostArray[count1]) break;
		}
		if(count1 >= total_count) break;
		lmHosts.hostArray[count] = lmHosts.hostArray[count1];
		lmHosts.hostArray[count1] = NULL;
	}

}

PLmObjectHost Hosts_AddHost(int instanceNum)
{
    //printf("in Hosts_AddHost %d \n", instanceNum);
    PLmObjectHost pHost = LanManager_Allocate(sizeof(LmObjectHost));
    if(pHost == NULL)
    {
        return NULL;
    }
    pHost->instanceNum = instanceNum;
    /* Compose Host object name. */
    char objectName[100] = LM_HOST_OBJECT_NAME_HEADER;
    char instanceNumStr[50] = {0};
    _ansc_itoa(pHost->instanceNum, instanceNumStr, 10);
    strcat(instanceNumStr, ".");
    strcat(objectName, instanceNumStr);
    pHost->objectName = LanManager_CloneString(objectName);

    pHost->l3unReachableCnt = 0;
    pHost->l1unReachableCnt = 0;
    pHost->ipv4AddrArray = NULL;
    pHost->numIPv4Addr = 0;
    pHost->ipv6AddrArray = NULL;
    pHost->numIPv6Addr = 0;
	pHost->pStringParaValue[LM_HOST_IPAddressId] = NULL;
    /* Default it is inactive. */
    pHost->bBoolParaValue[LM_HOST_ActiveId] = FALSE;
    pHost->ipv4Active = FALSE;
    pHost->ipv6Active = FALSE;
    pHost->activityChangeTime  = time(NULL);

    pHost->iIntParaValue[LM_HOST_X_CISCO_COM_ActiveTimeId] = -1;
    pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = -200;

	pHost->Layer3Interface = NULL;

	memset(pHost->backupHostname,0,64);
    int i;
    for(i=0; i<LM_HOST_NumStringPara; i++) pHost->pStringParaValue[i] = NULL;
	Clean_Host_Table();
    if(lmHosts.numHost >= lmHosts.sizeHost){
        lmHosts.sizeHost += LM_HOST_ARRAY_STEP;
        PLmObjectHost *newArray = LanManager_Allocate(lmHosts.sizeHost * sizeof(PLmObjectHost));
        for(i=0; i<lmHosts.numHost; i++){
            newArray[i] = lmHosts.hostArray[i];
        }
        PLmObjectHost *backupArray = lmHosts.hostArray;
        lmHosts.hostArray = newArray;
        if(backupArray) LanManager_Free(backupArray);
    }
    pHost->id = lmHosts.numHost;
    lmHosts.hostArray[pHost->id] = pHost;
    lmHosts.numHost++;
    return pHost;
}

void Host_SetIPAddress(PLmObjectHostIPAddress pIP, int l3unReachableCnt, char *source)
{
    pIP->l3unReachableCnt = l3unReachableCnt;
    LM_SET_PSTRINGPARAVALUE(pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressSourceId], source); 
}

PLmObjectHost Hosts_FindHostByPhysAddress(char * physAddress)
{
    int i = 0;
    for(; i<lmHosts.numHost; i++){
        if(AnscEqualString(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId], physAddress, FALSE)){
            return lmHosts.hostArray[i];
        }
    }
    return NULL;
}
PLmObjectHost XHosts_FindHostByPhysAddress(char * physAddress)
{
    int i = 0;
    for(; i<XlmHosts.numHost; i++){
        if(AnscEqualString(XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId], physAddress, FALSE)){
            return XlmHosts.hostArray[i];
        }
    }
    return NULL;
}

#define MACADDR_SZ          18
#define ATOM_MAC "00:00:ca:01:02:03"
#define ATOM_MAC_CSC "00:05:04:03:02:01"
BOOL validate_mac(char * physAddress)
{
	if (physAddress && physAddress[0]) {
	    if(physAddress[2] == ':')
		if(physAddress[5] == ':')
			if(physAddress[8] == ':')
				if(physAddress[11] == ':')
					if(physAddress[14] == ':')
					  return TRUE;
	}

	return FALSE;
}

PLmObjectHost XHosts_AddHostByPhysAddress(char * physAddress)
{
    char comments[256] = {0};
    char ssid[LM_GEN_STR_SIZE]={0};
	if(!physAddress || !validate_mac(physAddress))
	{
		CcspTraceWarning(("RDKB_CONNECTED_CLIENT: Invalid MacAddress ignored\n"));
		return NULL;
	}
		
    if(!physAddress || \
       0 == strcmp(physAddress, "00:00:00:00:00:00")) return NULL;

    if(strlen(physAddress) != MACADDR_SZ-1) return NULL;
    PLmObjectHost pHost = XHosts_FindHostByPhysAddress(physAddress);
    if(pHost) return pHost;
	
    pHost = XHosts_AddHost(XlmHosts.availableInstanceNum);
    if(pHost){
        pHost->pStringParaValue[LM_HOST_PhysAddressId] = LanManager_CloneString(physAddress);
        pHost->pStringParaValue[LM_HOST_HostNameId] = LanManager_CloneString(physAddress);
        _getLanHostComments(physAddress, comments);
        if ( comments[0] != 0 )
        {
            pHost->pStringParaValue[LM_HOST_Comments] = LanManager_CloneString(comments);
        }
		
		pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString("Device.WiFi.SSID.3");
   		pHost->pStringParaValue[LM_HOST_AddressSource] = LanManager_CloneString("DHCP");
		pHost->bClientReady = FALSE;
		//CcspTraceWarning(("RDKB_CONNECTED_CLIENT: pHost->bClientReady = %d \n",pHost->bClientReady));
		XlmHosts.availableInstanceNum++;
    }
    CcspTraceWarning(("New XHS host added sucessfully\n"));
    return pHost;
}


PLmObjectHost Hosts_AddHostByPhysAddress(char * physAddress)
{
    char comments[256] = {0};
    char ssid[LM_GEN_STR_SIZE]={0};

    if(!physAddress || \
       0 == strcmp(physAddress, "00:00:00:00:00:00")) return NULL;

    if(strlen(physAddress) != MACADDR_SZ-1) return NULL;

	if(!validate_mac(physAddress))
	{
		CcspTraceWarning(("RDKB_CONNECTED_CLIENT: Invalid MacAddress ignored\n"));
		return NULL;
	}
		
    PLmObjectHost pHost = Hosts_FindHostByPhysAddress(physAddress);
    if(pHost) return pHost;
	
	if((!strcmp(ATOM_MAC,physAddress))||(!strcmp(ATOM_MAC_CSC,physAddress)))
	{
		//CcspTraceWarning(("RDKB_CONNECTED_CLIENT: ATOM_MAC = %s ignored\n",physAddress));
		return NULL;
	}
    pHost = Hosts_AddHost(lmHosts.availableInstanceNum);
    if(pHost){
        pHost->pStringParaValue[LM_HOST_PhysAddressId] = LanManager_CloneString(physAddress);
        pHost->pStringParaValue[LM_HOST_HostNameId] = LanManager_CloneString(physAddress);
        _getLanHostComments(physAddress, comments);
        if ( comments[0] != 0 )
        {
            pHost->pStringParaValue[LM_HOST_Comments] = LanManager_CloneString(comments);
        }
/* #ifdef USE_NOTIFY_COMPONENT
        if(bWifiHost)
        {
			if(SearchWiFiClients(physAddress,ssid))
			{
				pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString(ssid);
				bWifiHost = FALSE;
			}
			else
			{
				pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString("Ethernet");
			}

        }
        else
#endif
*/
		if( physAddress &&
		        (strncasecmp(physAddress,"60:b4:f7:", 9)==0 ||
                 strncasecmp(physAddress,"58:90:43:", 9)==0 ||
	             strncasecmp(physAddress,"b8:ee:0e:", 9)==0 ||
	             strncasecmp(physAddress,"b8:d9:4d:", 9)==0))
			pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString("Mesh");
		else
			pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString("Ethernet");
        
		pHost->pStringParaValue[LM_HOST_AddressSource] = LanManager_CloneString("DHCP");
		pHost->bClientReady = FALSE;
		//CcspTraceWarning(("RDKB_CONNECTED_CLIENT: pHost->bClientReady = %d \n",pHost->bClientReady));
		lmHosts.availableInstanceNum++;
    }
#ifdef USE_NOTIFY_COMPONENT
	
	CcspTraceWarning(("LMlite-CLIENT <%s> <%d> : Connected Mac = %s \n",__FUNCTION__,__LINE__ ,pHost->pStringParaValue[LM_HOST_PhysAddressId]));
	pHost->bNotify = FALSE;
#endif
    return pHost;
}
void Host_FreeIPAddress(PLmObjectHost pHost, int version)
{
    int *num;
    PLmObjectHostIPAddress pIpAddrList, pCur, *ppHeader;

    if(version == 4){
        num = &(pHost->numIPv4Addr);
        pIpAddrList = pHost->ipv4AddrArray;
        ppHeader = &(pHost->ipv4AddrArray);
    }else{
        num = &(pHost->numIPv6Addr);
        pIpAddrList = pHost->ipv6AddrArray;
        ppHeader = &(pHost->ipv6AddrArray);
    }

    *num = 0;
    while(pIpAddrList != NULL)
    {
        LanManager_Free(pIpAddrList->pStringParaValue[LM_HOST_IPAddress_IPAddressId]);
        pCur = pIpAddrList;
        pIpAddrList = pIpAddrList->pNext;
        LanManager_Free(pCur); /*RDKB-7348, CID-33198, free current list*/
        pCur = NULL;
        *ppHeader = NULL;
    }
}

PLmObjectHostIPAddress
Add_Update_IPv4Address
    (
        PLmObjectHost pHost,
        char * ipAddress
    )
{
	int *num;
	PLmObjectHostIPAddress pIpAddrList, pCur, pPre, *ppHeader;

	num = &(pHost->numIPv4Addr);
	pIpAddrList = pHost->ipv4AddrArray;
	ppHeader = &(pHost->ipv4AddrArray);
	pHost->ipv4Active = TRUE;

   for(pCur = pIpAddrList; pCur != NULL; pPre = pCur, pCur = pCur->pNext){
        if(AnscEqualString(pCur->pStringParaValue[LM_HOST_IPAddress_IPAddressId], ipAddress, FALSE)){
			break;
        }
    }
	if (pCur == NULL){
		pCur = LanManager_Allocate(sizeof(LmObjectHostIPAddress));
		if(pCur == NULL){
			return NULL;
	}
	pCur->pStringParaValue[LM_HOST_IPAddress_IPAddressId] = LanManager_CloneString(ipAddress);
        pCur->pNext = *ppHeader;
        *ppHeader = pCur;
        (*num)++;
	pCur->instanceNum = *num;
   }
   else{
     	if(pCur != pIpAddrList)
	{
          pPre->pNext=pCur->pNext;
          pCur->pNext = pIpAddrList;
          *ppHeader = pCur;
        }
    }
    return pCur;
}

PLmObjectHostIPAddress
Add_Update_IPv6Address
    (
        PLmObjectHost pHost,
        char * ipAddress,
	int dibbler_flag
    )
{
	int i, *num;
	PLmObjectHostIPAddress pIpAddrList, pCur, *ppHeader, prev, temp;
	num = &(pHost->numIPv6Addr);
	pIpAddrList = pHost->ipv6AddrArray;
	ppHeader = &(pHost->ipv6AddrArray);
	pHost->ipv6Active = TRUE;

	if(*ppHeader==NULL)
	{
		prev=NULL;
		/*List is Empty, Allocate Memory*/

		for(i=0;i<3;i++)
		{
			temp=LanManager_Allocate(sizeof(LmObjectHostIPAddress));
			if(temp == NULL)
			{
				return NULL;
			}
			else
			{
				//temp->pStringParaValue[LM_HOST_IPAddress_IPAddressId] = LanManager_CloneString("EMPTY");
				temp->pStringParaValue[LM_HOST_IPAddress_IPAddressId] = LanManager_CloneString(" "); // fix for RDKB-19836
				(*num)++;
				temp->instanceNum = *num;
				temp->pNext=prev;
				pIpAddrList=temp;
				*ppHeader=temp;
				prev=temp;
			}
		}
	}
	if(dibbler_flag==0)
	{
		if(strncmp(ipAddress,"fe80:",5)==0)
		{
			pCur=pIpAddrList->pNext;
		}
		else
		{
			pCur=pIpAddrList->pNext->pNext;
		}
	}
	else
	{
		pCur=pIpAddrList;
	}
	LanManager_CheckCloneCopy(&(pCur->pStringParaValue[LM_HOST_IPAddress_IPAddressId]), ipAddress);
	return pCur;
}

int extract(char* line, char* mac, char * ip)
{
	PLmObjectHost pHost;
	int i,pivot=0,mac_start=0,flag=-1;
	mac[0] = 0;
	if((strstr(line,"<entry") == NULL) || (strstr(line,"</entry>") == NULL))
	{
		//CcspTraceWarning(("Invalid dibbler entry : %s\n",line));
		return 1;
	}

	for (i=0;i<(strlen(line));i++)
	{
		if(line[i]=='>')
		{
			pivot=i+1;
			mac_start=pivot-19;
			if (0 <= mac_start)
			{
				flag = 0;
				break;
			}
			return 1;
		}
	}

	if (-1 == flag) {
		return 1;
	}

	for(i=0;((flag==0)||(i<=17));i++)
	{
		if((line[pivot+i]!='<')&&(flag==0))
		{
			ip[i]=line[pivot+i];
		}
		if(line[pivot+i]=='<')
		{
			ip[i]='\0';
			flag=1;
		}
		if(i<17)
		{
			mac[i]=line[mac_start+i];
		}
		if(i==17)
		{
			mac[i]='\0';
		}
	}
	return 0;
}

void Add_IPv6_from_Dibbler()
{
	FILE *fptr = NULL;
	char line[256]={0},ip[64]={0},mac[18]={0};
	PLmObjectHost	pHost	= NULL;

	if ((fptr=fopen("/etc/dibbler/server-cache.xml","r")) != NULL )
	{
		while ( fgets(line, sizeof(line), fptr) != NULL )
		{
			if(strstr(line,"addr") != NULL)
			{
				if(1 == extract(line,mac,ip))
					continue;
		        pthread_mutex_lock(&LmHostObjectMutex);
				pHost = Hosts_AddHostByPhysAddress(mac);
				if(pHost)
				{
					Add_Update_IPv6Address(pHost,ip,DIBBLER_IPv6);
				}
                pthread_mutex_unlock(&LmHostObjectMutex);
			}
		}
		fclose(fptr);
	}
}

PLmObjectHostIPAddress
Host_AddIPAddress
    (
        PLmObjectHost pHost,
        char * ipAddress,
        int version
    )
{
    PLmObjectHostIPAddress pCur;

	if(!ipAddress)
		return NULL;

    if(version == 4)
	{
		pCur = Add_Update_IPv4Address(pHost,ipAddress);
		LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_IPAddressId]) , ipAddress);
    }
	else
	{
		pCur = Add_Update_IPv6Address(pHost,ipAddress,ARP_IPv6);
    }
	return pCur;
}

void _set_comment_(LM_cmd_comment_t *cmd)
{
    PLmObjectHost pHost;
    char mac[18];
	
    snprintf(mac,sizeof(mac)/sizeof(mac[0]), "%02x:%02x:%02x:%02x:%02x:%02x", cmd->mac[0], cmd->mac[1], cmd->mac[2], cmd->mac[3], cmd->mac[4], cmd->mac[5]);

    /* set comment value into syscfg */
    /* we don't check whether this device is in our LmObject list */

    if (lm_wrapper_priv_set_lan_host_comments(cmd))
		return;

    /* But if this device is in LmObject list, update the comments value */
    
    pHost = Hosts_FindHostByPhysAddress(mac);
    if(pHost){
		pthread_mutex_lock(&LmHostObjectMutex);
        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Comments]), cmd->comment);
		pthread_mutex_unlock(&LmHostObjectMutex);
    }
    

}

char* FindMACByIPAddress(char * ip_address)
{
	if(ip_address)
	{
	    int i = 0;
	    for(; i<lmHosts.numHost; i++){
	        if(AnscEqualString(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_IPAddressId], ip_address, TRUE)){
	            return lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId];
	        }
	    }		
	}

    return NULL;
}


static inline int _mac_string_to_array(char *pStr, unsigned char array[6])
{
    int tmp[6],n,i;
	if(pStr == NULL)
		return -1;
		
    memset(array,0,6);
    n = sscanf(pStr,"%02x:%02x:%02x:%02x:%02x:%02x",&tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
    if(n==6){
        for(i=0;i<n;i++)
            array[i] = (unsigned char)tmp[i];
        return 0;
    }

    return -1;
}

PLmObjectHostIPAddress LM_GetIPArr_FromIndex(PLmObjectHost pHost, ULONG nIndex, int version)
{

	PLmObjectHostIPAddress pIpAddrList, pCur = NULL;
	UINT i;

	if(version == IP_V4){
		pIpAddrList = pHost->ipv4AddrArray;
	}else{
		pIpAddrList = pHost->ipv6AddrArray;
	}

	for(pCur = pIpAddrList, i=0; (pCur != NULL) && (i < nIndex); pCur =	pCur->pNext,i++);

	return pCur;
}

int LM_get_online_device()
{
    int i;
    int num = 0;
    PLmObjectHostIPAddress pIP4;

	if(0 != Hosts_stop_scan()){
        PRINTD("bridge mode\n");
		return num;
    }
	
   // pthread_mutex_lock(&LmHostObjectMutex);
    for(i = 0; i < lmHosts.numHost; i++){
        if(TRUE == lmHosts.hostArray[i]->bBoolParaValue[LM_HOST_ActiveId]){
            /* Do NOT count TrueStaticIP client */
            for(pIP4 = lmHosts.hostArray[i]->ipv4AddrArray; pIP4 != NULL; pIP4 = pIP4->pNext){
                if ( 0 == strncmp(pIP4->pStringParaValue[LM_HOST_IPAddress_IPAddressId], "192.168", 7) ||
                     0 == strncmp(pIP4->pStringParaValue[LM_HOST_IPAddress_IPAddressId], "10.", 3) ||
                     (	(0 == strncmp(pIP4->pStringParaValue[LM_HOST_IPAddress_IPAddressId], "172.", 4)) && \
                     	(0 != strncmp(pIP4->pStringParaValue[LM_HOST_IPAddress_IPAddressId], "172.16.12", 9) ) ) 
                   )
                num++;
                break;
            }
        }
    }
    //pthread_mutex_unlock(&LmHostObjectMutex);
	return num;
}
int XLM_get_online_device()
{
	int i;
    int num = 0;
    PLmObjectHostIPAddress pIP4;

	CcspTraceWarning(("Inside %s XlmHosts.numHost = %d\n",__FUNCTION__,XlmHosts.numHost));


	for(i = 0; i < XlmHosts.numHost; i++){
        if(TRUE == XlmHosts.hostArray[i]->bBoolParaValue[LM_HOST_ActiveId]){
            /* Do NOT count TrueStaticIP client */
            for(pIP4 = XlmHosts.hostArray[i]->ipv4AddrArray; pIP4 != NULL; pIP4 = pIP4->pNext){
                if (0 == strncmp(pIP4->pStringParaValue[LM_HOST_IPAddress_IPAddressId], "172.", 4))
                	num++;
                break;
            }
        }
    }
	return num;
}

int LMDmlHostsSetHostComment
    (
        char*                       pMac,
        char*                       pComment
    )
{
    int ret;
    unsigned char mac[6];
	LM_cmd_comment_t cmd;
    
    ret = _mac_string_to_array(pMac, mac);
	
    if(ret == 0){
		cmd.cmd = LM_API_CMD_SET_COMMENT;
	    memcpy(cmd.mac, mac, 6);
	    if(pComment == NULL){
	        cmd.comment[0] = '\0';
	    }else{
	        strncpy(cmd.comment, pComment, LM_COMMENTS_LEN -1);
	        cmd.comment[LM_COMMENTS_LEN -1] = '\0';
	    }
        _set_comment_(&cmd);   
    }
    
    return 0;
}

#if 0
PLmObjectHostIPv6Address
Host_AddIPv6Address
    (
        PLmObjectHost pHost,
        int instanceNum,
        char * ipv6Address
    )
{
    /* check if the address has already exist. */
    int i = 0;
    for(i=0; i<pHost->numIPv6Addr; i++){
        /* If IP address already exists, return. */
        if(AnscEqualString(pHost->ipv6AddrArray[i]->pStringParaValue[LM_HOST_IPv6Address_IPAddressId],ipv6Address, FALSE))
            return pHost->ipv6AddrArray[i];
    }

    for(i=0; i<pHost->numIPv6Addr; i++){
        /* If instance number is occuppied, assign a new instance number. It may not happen in DHCP mode. */
        if(pHost->ipv6AddrArray[i]->instanceNum == instanceNum){
            instanceNum = pHost->availableInstanceNumIPv6Address;
            pHost->availableInstanceNumIPv6Address++;
        }
    }

    PLmObjectHostIPv6Address pIPv6Address = LanManager_Allocate(sizeof(LmObjectHostIPv6Address));
    pIPv6Address->instanceNum = instanceNum;
    pIPv6Address->pStringParaValue[LM_HOST_IPv6Address_IPAddressId] = LanManager_CloneString(ipv6Address);
    if(pHost->availableInstanceNumIPv6Address <= pIPv6Address->instanceNum)
        pHost->availableInstanceNumIPv6Address = pIPv6Address->instanceNum + 1;

    if(pHost->numIPv6Addr >= pHost->sizeIPv6Addr){
        pHost->sizeIPv6Addr += LM_HOST_ARRAY_STEP;
        PLmObjectHostIPv6Address *newArray = LanManager_Allocate(pHost->sizeIPv6Addr * sizeof(PLmObjectHostIPv6Address));
        for(i=0; i<pHost->numIPv6Addr; i++){
            newArray[i] = pHost->ipv6AddrArray[i];
        }
        PLmObjectHostIPv6Address *backupArray = pHost->ipv6AddrArray;
        pHost->ipv6AddrArray = newArray;
        if(backupArray) LanManager_Free(backupArray);
    }
    pIPv6Address->id = pHost->numIPv6Addr;
    pHost->ipv6AddrArray[pIPv6Address->id] = pIPv6Address;
    pHost->numIPv6Addr++;
    return pIPv6Address;
}
#endif

#ifdef LM_IPC_SUPPORT

static inline void _get_host_mediaType(enum LM_MEDIA_TYPE * m_type, char * l1Interfce)
{
    if(l1Interfce == NULL){
        *m_type = LM_MEDIA_TYPE_UNKNOWN;
    }else if(strstr(l1Interfce, "MoCA")){
        *m_type = LM_MEDIA_TYPE_MOCA;
    }else if(strstr(l1Interfce, "WiFi")){
        *m_type = LM_MEDIA_TYPE_WIFI;
    }else
        *m_type = LM_MEDIA_TYPE_ETHERNET;
}

static inline enum LM_ADDR_SOURCE _get_addr_source(char *source)
{
    if(source == NULL)
        return LM_ADDRESS_SOURCE_NONE;

    if(strstr(source,LM_ADDRESS_SOURCE_DHCP_STR)){
        return LM_ADDRESS_SOURCE_DHCP;
    }else if(strstr(source, LM_ADDRESS_SOURCE_STATIC_STR)){
        return LM_ADDRESS_SOURCE_STATIC;
    }else if(strstr(source, LM_ADDRESS_SOURCE_RESERVED_STR)){
        return LM_ADDRESS_SOURCE_RESERVED;
    }else
        return LM_ADDRESS_SOURCE_NONE;
}

static inline void _get_host_ipaddress(LM_host_t *pDestHost, PLmObjectHost pHost)
{
    int i;   
    PLmObjectHostIPAddress pIpSrc; 
    pDestHost->ipv4AddrAmount = pHost->numIPv4Addr;
    pDestHost->ipv6AddrAmount = pHost->numIPv6Addr;
    LM_ip_addr_t *pIp;
    for(i=0, pIpSrc = pHost->ipv4AddrArray; pIpSrc != NULL && i < LM_MAX_IP_AMOUNT;i++, pIpSrc = pIpSrc->pNext){
        pIp = &(pDestHost->ipv4AddrList[i]);
        inet_pton(AF_INET, pIpSrc->pStringParaValue[LM_HOST_IPAddress_IPAddressId],pIp->addr);
        pIp->addrSource = _get_addr_source(pIpSrc->pStringParaValue[LM_HOST_IPAddress_IPAddressSourceId]);
        pIp->priFlg = pIpSrc->l3unReachableCnt;
        if(pIp->addrSource == LM_ADDRESS_SOURCE_DHCP)
            pIp->LeaseTime = pIpSrc->LeaseTime;
        else
            pIp->LeaseTime = 0;
   }
    
    
    for(i = 0, pIpSrc = pHost->ipv6AddrArray;pIpSrc != NULL && i < LM_MAX_IP_AMOUNT;i++, pIpSrc = pIpSrc->pNext){
        pIp = &(pDestHost->ipv6AddrList[i]);
        inet_pton(AF_INET6, pIpSrc->pStringParaValue[LM_HOST_IPAddress_IPAddressId],pIp->addr);
        pIp->addrSource = _get_addr_source(pIpSrc->pStringParaValue[LM_HOST_IPAddress_IPAddressSourceId]); 
        //Not support yet
        pIp->LeaseTime = 0;
    }
}

static inline void _get_host_info(LM_host_t *pDestHost, PLmObjectHost pHost)
{
        mac_string_to_array(pHost->pStringParaValue[LM_HOST_PhysAddressId], pDestHost->phyAddr);
        pDestHost->online = (unsigned char)pHost->bBoolParaValue[LM_HOST_ActiveId];
        pDestHost->activityChangeTime = pHost->activityChangeTime;
        _get_host_mediaType(&(pDestHost->mediaType), pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]); 
        STRNCPY_NULL_CHK(pDestHost->hostName, pHost->pStringParaValue[LM_HOST_HostNameId], sizeof(pDestHost->hostName)-1);
        STRNCPY_NULL_CHK(pDestHost->l3IfName, pHost->pStringParaValue[LM_HOST_Layer3InterfaceId], sizeof(pDestHost->l3IfName)-1);
        STRNCPY_NULL_CHK(pDestHost->l1IfName, pHost->pStringParaValue[LM_HOST_Layer1InterfaceId], sizeof(pDestHost->l1IfName)-1);
        STRNCPY_NULL_CHK(pDestHost->comments, pHost->pStringParaValue[LM_HOST_Comments], sizeof(pDestHost->comments)-1);
        STRNCPY_NULL_CHK(pDestHost->AssociatedDevice, pHost->pStringParaValue[LM_HOST_AssociatedDeviceId], sizeof(pDestHost->AssociatedDevice)-1);
        pDestHost->RSSI = pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId];
        _get_host_ipaddress(pDestHost, pHost); 
}

static inline void _get_hosts_info_cfunc(int fd, void* recv_buf, int buf_size)
{
    int i, len = 0;
    PLmObjectHost pHost = NULL;
    LM_host_t *pDestHost = NULL;
    LM_hosts_t hosts;

    memset(&hosts, 0, sizeof(hosts));
/*
    if(0 == Hosts_stop_scan()){
        PRINTD("bridge mode return 0\n");
        Hosts_PollHost();
    }
*/

    pthread_mutex_lock(&LmHostObjectMutex);
    hosts.count = lmHosts.numHost;

    for(i = 0; i < hosts.count; i++){
        pHost = lmHosts.hostArray[i];
        pDestHost = &(hosts.hosts[i]);
        _get_host_info(pDestHost, pHost);
    }
    pthread_mutex_unlock(&LmHostObjectMutex);

    len = (hosts.count)*sizeof(LM_host_t) + sizeof(int);
    write(fd, &hosts, len);
}

static inline void _get_host_by_mac_cfunc(int fd, void* recv_buf, int buf_size)
{
    LM_cmd_get_host_by_mac_t *cmd = recv_buf;
    LM_cmd_common_result_t result;
    PLmObjectHost pHost;
    char mac[18];

    if(buf_size < sizeof(LM_cmd_get_host_by_mac_t))
        return;
    memset(&result, 0, sizeof(result));
    snprintf(mac,sizeof(mac)/sizeof(mac[0]), "%02x:%02x:%02x:%02x:%02x:%02x", cmd->mac[0], cmd->mac[1], cmd->mac[2], cmd->mac[3], cmd->mac[4], cmd->mac[5]);
    pthread_mutex_lock(&LmHostObjectMutex);
    pHost = Hosts_FindHostByPhysAddress(mac);
    if(pHost){
        result.result = LM_CMD_RESULT_OK;
        _get_host_info(&(result.data.host), pHost);
    }else{
        result.result = LM_CMD_RESULT_NOT_FOUND;
    }
    pthread_mutex_unlock(&LmHostObjectMutex);
    write(fd, &result, sizeof(result));
}

static inline void _set_comment_cfunc(int fd, void* recv_buf, int buf_size)
{

    LM_cmd_comment_t *cmd = recv_buf;
    LM_cmd_common_result_t result;
    PLmObjectHost pHost;
    char mac[18];

    if(buf_size < sizeof(LM_cmd_comment_t))
        return;

    memset(&result, 0, sizeof(result));
    snprintf(mac,sizeof(mac)/sizeof(mac[0]), "%02x:%02x:%02x:%02x:%02x:%02x", cmd->mac[0], cmd->mac[1], cmd->mac[2], cmd->mac[3], cmd->mac[4], cmd->mac[5]);

    /* set comment value into syscfg */
    /* we don't check whether this device is in our LmObject list */
    result.result = LM_CMD_RESULT_INTERNAL_ERR;

    if (lm_wrapper_priv_set_lan_host_comments(cmd))
	goto END;

    /* But if this device is in LmObject list, update the comments value */
    pthread_mutex_lock(&LmHostObjectMutex);
    pHost = Hosts_FindHostByPhysAddress(mac);
    if(pHost){
        LanManager_CheckCloneCopy( &(pHost->pStringParaValue[LM_HOST_Comments]) , cmd->comment);
    }
    pthread_mutex_unlock(&LmHostObjectMutex);
    result.result = LM_CMD_RESULT_OK;

END:
    write(fd, &result, sizeof(result));
}
static inline void _get_online_device_cfunc(int fd, void* recv_buf, int buf_size){
    int i;
    int num = 0;
    LM_cmd_common_result_t result;
    PLmObjectHostIPAddress pIP4;
    memset(&result, 0, sizeof(result));
    pthread_mutex_lock(&LmHostObjectMutex);
    for(i = 0; i < lmHosts.numHost; i++){
        if(TRUE == lmHosts.hostArray[i]->bBoolParaValue[LM_HOST_ActiveId]){
            /* Do NOT count TrueStaticIP client */
            for(pIP4 = lmHosts.hostArray[i]->ipv4AddrArray; pIP4 != NULL; pIP4 = pIP4->pNext){
                if ( 0 == strncmp(pIP4->pStringParaValue[LM_HOST_IPAddress_IPAddressId], "192.168", 7) ||
                     0 == strncmp(pIP4->pStringParaValue[LM_HOST_IPAddress_IPAddressId], "10.", 3) ||
                     0 == strncmp(pIP4->pStringParaValue[LM_HOST_IPAddress_IPAddressId], "172.", 4)
                   )
                num++;
                break;
            }
        }
    }
    pthread_mutex_unlock(&LmHostObjectMutex);
    result.result = LM_CMD_RESULT_OK;
    result.data.online_num = num;
    write(fd, &result, sizeof(result));
}

static inline void _not_support_cfunc(int fd, void* recv_buf, int buf_size){

}

typedef void (*LM_cfunc_t)(int, void*, int);

LM_cfunc_t cfunc[LM_API_CMD_MAX] = {
    _get_hosts_info_cfunc,              // LM_API_CMD_GET_HOSTS = 0,
    _get_host_by_mac_cfunc,             //LM_API_CMD_GET_HOST_BY_MAC,
    _set_comment_cfunc,                 //LM_API_CMD_SET_COMMENT,
    _get_online_device_cfunc,           //LM_API_CMD_GET_ONLINE_DEVICE,
    _not_support_cfunc,                 //LM_API_CMD_ADD_NETWORK,
    _not_support_cfunc,                 //LM_API_CMD_DELETE_NETWORK,
    _not_support_cfunc,                 //LM_API_CMD_GET_NETWORK,
};

void lm_cmd_thread_func()
{
    socklen_t clt_addr_len;
    int listen_fd;
    int cmd_fd;
    int ret;
    int i;
    static char recv_buf[1024];
    int len;
    struct sockaddr_un clt_addr;
    struct sockaddr_un srv_addr;

    listen_fd=socket(PF_UNIX,SOCK_STREAM,0);
    if(listen_fd<0)
        return;

    srv_addr.sun_family=AF_UNIX;
    unlink(LM_SERVER_FILE_NAME);
    strcpy(srv_addr.sun_path,LM_SERVER_FILE_NAME);
    bind(listen_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));

    listen(listen_fd, 10);

    PRINTD("start listen\n");
    while(1){
	len = sizeof(clt_addr);
        cmd_fd = accept(listen_fd,(struct sockaddr *)&clt_addr,&len);
        if(cmd_fd < 0 )
           continue;
        PRINTD("accept \n");
        ret = read(cmd_fd, recv_buf, sizeof(recv_buf));
        if(ret > 0){
            PRINTD("get command %d \n", LM_API_GET_CMD(recv_buf));
            if((unsigned int)LM_API_CMD_MAX > LM_API_GET_CMD(recv_buf)){
                cfunc[LM_API_GET_CMD(recv_buf)](cmd_fd, recv_buf, ret);
            }
        }
        close(cmd_fd);
    }
}
#endif

int Hosts_stop_scan()
{
    return lm_wrapper_priv_stop_scan();
}
void XHosts_SyncWifi()
{
	int count = 0;
    int i;
	CcspTraceWarning(("Inside %s \n",__FUNCTION__));
    PLmObjectHost pHost;
    LM_wifi_wsta_t *hosts = NULL;

	Xlm_wrapper_get_wifi_wsta_list(&count, &hosts);
	
	if (count > 0)
    {
        for (i = 0; i < count; i++)
        {
            PRINTD("%s: Process No.%d mac %s\n", __FUNCTION__, i+1, hosts[i].phyAddr);

            pHost = XHosts_FindHostByPhysAddress(hosts[i].phyAddr);

            if ( !pHost )
            {

				pHost = XHosts_AddHostByPhysAddress(hosts[i].phyAddr);
				if ( pHost )
                {      
                	CcspTraceWarning(("%s, %d New XHS host added sucessfully\n",__FUNCTION__, __LINE__));
			    }
			}
			Xlm_wrapper_get_info(pHost);
			Host_AddIPv4Address ( pHost, pHost->pStringParaValue[LM_HOST_IPAddressId]);
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), hosts[i].ssid);
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), hosts[i].AssociatedDevice);
			pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = hosts[i].RSSI;
			pHost->l1unReachableCnt = 1;
			pHost->bBoolParaValue[LM_HOST_ActiveId] = hosts[i].Status;
			pHost->activityChangeTime = time((time_t*)NULL);
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");
			
        }
 	}

    if ( hosts )
    {
        free(hosts);
    }
	//Get the lease time as well as update host name.
	
}

void Hosts_SyncWifi()
{
    int count = 0;
    int i;

    PLmObjectHost pHost;
    LM_wifi_wsta_t *hosts = NULL;

    lm_wrapper_get_wifi_wsta_list("brlan0", &count, &hosts);

    if (count > 0)
    {
        //pthread_mutex_lock(&LmHostObjectMutex);
        for (i = 0; i < count; i++)
        {
            PRINTD("%s: Process No.%d mac %s\n", __FUNCTION__, i+1, hosts[i].phyAddr);

            pHost = Hosts_FindHostByPhysAddress(hosts[i].phyAddr);

            if ( pHost )
            {
			pthread_mutex_lock(&LmHostObjectMutex);           	
#ifdef USE_NOTIFY_COMPONENT
			if(hosts[i].Status)
			{
#endif
				LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), hosts[i].ssid);
				LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), hosts[i].AssociatedDevice);
				pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = hosts[i].RSSI;
				pHost->l1unReachableCnt = 1;

				if(pHost->bBoolParaValue[LM_HOST_ActiveId] != hosts[i].Status)
					LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
#ifdef USE_NOTIFY_COMPONENT
			}
			else
			{
				if(pHost->bBoolParaValue[LM_HOST_ActiveId] != hosts[i].Status)
					LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);
			}
#endif
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");
			pthread_mutex_unlock(&LmHostObjectMutex);
            }
        }
        //pthread_mutex_unlock(&LmHostObjectMutex);
    }

    if ( hosts )
    {
        free(hosts);
        hosts=NULL;
    }

    return;
}

void *Event_HandlerThread(void *threadid)
{
    long tid;
    tid = (long)threadid;
    LM_wifi_wsta_t hosts;
    LM_moca_cpe_t mhosts;
    PLmObjectHost pHost;
    //printf("Hello World! It's me, thread #%ld!\n", tid);
    mqd_t mq;
    struct mq_attr attr;
    char buffer[MAX_SIZE + 1];
	char radio[32];
    int must_stop = 0;

    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 100;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    /* create the message queue */
    mq = mq_open(EVENT_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);

    CHECK((mqd_t)-1 != mq);
    do
    {
        ssize_t bytes_read;
        EventQData EventMsg;
        Eth_data EthHost;

        /* receive the message */
        bytes_read = mq_receive(mq, buffer, MAX_SIZE, NULL);

        CHECK(bytes_read >= 0);

        buffer[bytes_read] = '\0';

        memcpy(&EventMsg,buffer,sizeof(EventMsg));

        if(EventMsg.MsgType == MSG_TYPE_ETH)
        {
            char Mac_Id[18];
            memcpy(&EthHost,EventMsg.Msg,sizeof(EthHost));

            pHost = Hosts_FindHostByPhysAddress(EthHost.MacAddr);

            if ( !pHost )
            {
                pthread_mutex_lock(&LmHostObjectMutex);
                pHost = Hosts_AddHostByPhysAddress(EthHost.MacAddr);

                if ( pHost )
                {
                    if ( pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] )
                    {
                         LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]);
                         pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = NULL;
                    }
                }
                else
                {
                    pthread_mutex_unlock(&LmHostObjectMutex);
                    continue;
                }      
                pthread_mutex_unlock(&LmHostObjectMutex);
            }

            if(EthHost.Active)
            {
                pthread_mutex_lock(&LmHostObjectMutex);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), "Ethernet");
                pthread_mutex_unlock(&LmHostObjectMutex);
                if ( ! pHost->pStringParaValue[LM_HOST_IPAddressId] )
                {
                    CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is Ethernet, MacAddress is %s IPAddr is not updated in ARP\n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
                    Hosts_SyncDHCP();
                    CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is Ethernet, MacAddress is %s IP from DNSMASQ is %s \n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_IPAddressId]));
                }
                LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
            }
            else
            {
                LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);
            }
            
            LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
            LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");
        }
        else if(EventMsg.MsgType == MSG_TYPE_WIFI)
        {
            memcpy(&hosts,EventMsg.Msg,sizeof(hosts));
            pHost = Hosts_FindHostByPhysAddress(hosts.phyAddr);
            if ( !pHost )
            {
                pthread_mutex_lock(&LmHostObjectMutex);
                pHost = Hosts_AddHostByPhysAddress(hosts.phyAddr);

                if ( pHost )
                {
                    if ( pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] )
                    {
                        LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]);
                        pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = NULL;
                    }
                }
                else
                {
                    pthread_mutex_unlock(&LmHostObjectMutex);
                    continue;
                }   
                
                pthread_mutex_unlock(&LmHostObjectMutex);
            }

            if(hosts.Status)
            {
                pthread_mutex_lock(&LmHostObjectMutex);
				memset(radio,0,sizeof(radio));	
                convert_ssid_to_radio(hosts.ssid, radio);				
				LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Layer1Interface]), radio);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), hosts.ssid);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), hosts.AssociatedDevice);
                pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = hosts.RSSI;
                pHost->l1unReachableCnt = 1;
                pthread_mutex_unlock(&LmHostObjectMutex);
                if ( ! pHost->pStringParaValue[LM_HOST_IPAddressId] )
                {
                    CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is WiFi, MacAddress is %s IPAddr is not updated in ARP\n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
                    Hosts_SyncDHCP();
                    CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is WiFi, MacAddress is %s IP from DNSMASQ is %s \n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_IPAddressId]));
                }

                LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
            }
            else
            {

	if( (hosts.ssid != NULL) && (pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] != NULL) )
	{
		 if(!strcmp(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],hosts.ssid))
		 {
                	pthread_mutex_lock(&LmHostObjectMutex);
				memset(radio,0,sizeof(radio));
				convert_ssid_to_radio(hosts.ssid, radio);
				DelAndShuffleAssoDevIndx(pHost);
				LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Layer1Interface]), radio);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), hosts.ssid);
                //LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), hosts.AssociatedDevice);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), " "); // fix for RDKB-19836
                pthread_mutex_unlock(&LmHostObjectMutex);
                LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);
		}
	}
            }
            
            LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
            LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");
        }
        else if(EventMsg.MsgType == MSG_TYPE_MOCA)
        {
            memcpy(&mhosts,EventMsg.Msg,sizeof(mhosts));
            pHost = Hosts_FindHostByPhysAddress(mhosts.phyAddr);
            if ( !pHost )
            {
                pthread_mutex_lock(&LmHostObjectMutex);
                pHost = Hosts_AddHostByPhysAddress(mhosts.phyAddr);

                if ( pHost )
                {
                    if ( pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] )
                    {
                        LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]);
                        pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = NULL;
                    }
                }
                else
                {
                    pthread_mutex_unlock(&LmHostObjectMutex);
                    continue;
                }   
                pthread_mutex_unlock(&LmHostObjectMutex);
            }

            if(mhosts.Status)
            {
                pthread_mutex_lock(&LmHostObjectMutex);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), mhosts.ssid);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), mhosts.AssociatedDevice);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), mhosts.parentMac);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), mhosts.deviceType);
                pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = mhosts.RSSI;
                pHost->l1unReachableCnt = 1;
                pthread_mutex_unlock(&LmHostObjectMutex);

                if ( ! pHost->pStringParaValue[LM_HOST_IPAddressId] )
                {
                    CcspTraceWarning(("<<< %s client type is MoCA, IPAddr is not updated in ARP %d >>\n>",__FUNCTION__,__LINE__));
                    CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is MoCA, MacAddress is %s IPAddr is not updated in ARP\n",pHost->pStringParaValue[LM_HOST_PhysAddressId]));
                    Hosts_SyncDHCP();
                    CcspTraceWarning(("<<< %s client type is MoCA, IPAddr is not updated in ARP %d %s >>\n>",__FUNCTION__,__LINE__,pHost->pStringParaValue[LM_HOST_IPAddressId]));
                    CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is MoCA, MacAddress is %s IP from DNSMASQ is %s \n",pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_IPAddressId]));
                }
                
                LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
            }
            else
            {
                pthread_mutex_lock(&LmHostObjectMutex);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), mhosts.ssid);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), mhosts.AssociatedDevice);
                pthread_mutex_unlock(&LmHostObjectMutex);

                LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);
            }       
        }           

    } while(1);
   pthread_exit(NULL);
}

void Hosts_SyncArp()
{
    char comments[256] = {0};
    int count = 0;
    int i;

    PLmObjectHost pHost = NULL;
    LM_host_entry_t *hosts = NULL;
    PLmObjectHostIPAddress pIP;

    lm_wrapper_get_arp_entries("brlan0", &count, &hosts);
    if (count > 0)
    {
        pthread_mutex_lock(&LmHostObjectMutex);

        for (i = 0; i < count; i++)
        {
            PRINTD("%s: Process No.%d mac %s\n", __FUNCTION__, i+1, hosts[i].phyAddr);

            pHost = Hosts_FindHostByPhysAddress(hosts[i].phyAddr);

            if ( pHost )
            {
                if ( _isIPv6Addr((char *)hosts[i].ipAddr) )
                {
                    pIP = Host_AddIPv6Address(pHost, hosts[i].ipAddr);
                    if ( hosts[i].status == LM_NEIGHBOR_STATE_REACHABLE)
                    {
                        Host_SetIPAddress(pIP, 0, "NONE"); 
                    }
                    else
                    {
                        Host_SetIPAddress(pIP, LM_HOST_RETRY_LIMIT, "NONE"); 
                    }
                }
                else
                {
                    if ( hosts[i].status == LM_NEIGHBOR_STATE_REACHABLE)
                    {
						/*
						  * We need to maintain recent reachable IP in "Device.Hosts.Host.1.IPAddress" host. so 
						  * that we are doing swap here.
						  */
						pIP = Host_AddIPv4Address(pHost, hosts[i].ipAddr);

                        Host_SetIPAddress(pIP, 0, "NONE");

                        _getLanHostComments(hosts[i].phyAddr, comments);
                        if ( comments[0] != 0 )
                        {
                            LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Comments]), comments);
                        }
                    }
                    else
                    {
						/*
						  * We need to update non-reachable IP in host details. No need to swap.
						  */
						pIP = LM_FindIPv4BaseFromLink( pHost, hosts[i].ipAddr );

						if( NULL != pIP )
						{
							Host_SetIPAddress(pIP, LM_HOST_RETRY_LIMIT, "NONE"); 
						}
                    }
                }

                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer3InterfaceId]), hosts[i].ifName );
            }
        }

        pthread_mutex_unlock(&LmHostObjectMutex);
    }

    if ( hosts )
    {
        free(hosts);
        hosts=NULL;
    }

    return;
}

void Hosts_SyncDHCP()
{
    lm_wrapper_get_dhcpv4_client();
    lm_wrapper_get_dhcpv4_reserved();
}

void Hosts_LoggingThread()
{
    int i;
    PLmObjectHost pHost;
	int TotalDevCount = 0;
	int TotalOnlineDev = 0;
	int TotalOffLineDev = 0;
	int TotalWiFiDev = 0;
	int Radio_2_Dev = 0;
	int Radio_5_Dev = 0;
	int TotalEthDev = 0;
	int TotalMoCADev = 0;
	sleep(30);
	while(1)
	{
		pthread_mutex_lock(&LmHostObjectMutex);
		TotalDevCount = lmHosts.numHost;

		for ( i = 0; i < lmHosts.numHost; i++ )
		{

			pHost = lmHosts.hostArray[i];
			if(pHost)
			{
				if(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId])
				{
					if(pHost->bBoolParaValue[LM_HOST_ActiveId])
					{
						TotalOnlineDev ++;


						if((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi")))
						{
							if((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi.SSID.1")))
							{
								Radio_2_Dev++;
							}
							else if((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi.SSID.2")))
							{
								Radio_5_Dev++;
							}
							TotalWiFiDev++;
						}
						else if ((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"MoCA")))
						{
							
							TotalMoCADev++;
						}
						else if ((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"Ethernet")))
						{
							
							TotalEthDev++;
						}
					}
				
				}
			}
		}
		pthread_mutex_unlock(&LmHostObjectMutex);
		TotalOffLineDev = TotalDevCount - TotalOnlineDev;
		
		CcspTraceWarning(("------------------------AssociatedClientsInfo-----------------------\n"));
		CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:Total_Clients_Connected=%d\n",TotalDevCount));
		CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:Total_Online_Clients=%d\n",TotalOnlineDev));
		CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:Total_Offline_Clients=%d\n",TotalOffLineDev));
		CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:Total_WiFi_Clients=%d\n",TotalWiFiDev));
		CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:Total_WiFi-2.4G_Clients=%d\n",Radio_2_Dev));
		CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:Total_WiFi-5.0G_Clients=%d\n",Radio_5_Dev));
		CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:Total_Ethernet_Clients=%d\n",TotalEthDev));
		CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:Total_MoCA_Clients=%d\n",TotalMoCADev));
		CcspTraceWarning(("-------------------------------------------------------------------\n"));
		
		TotalDevCount = 0;
		TotalOnlineDev = 0;
		TotalOffLineDev = 0;
		TotalWiFiDev = 0;
		Radio_2_Dev = 0;
		Radio_5_Dev = 0;
		TotalEthDev = 0;
		TotalMoCADev = 0;

		sleep(g_Client_Poll_interval*60); 
	}
}



void Hosts_StatSyncThreadFunc()
{
    int i,count;
    char cmd[64] = {0};
    int ret;
    static BOOL bridgemode = FALSE;
    PLmObjectHost   pHost      = NULL;
    LM_host_entry_t *hosts     = NULL;
    LM_wifi_wsta_t  *wifiHosts = NULL;
    LM_moca_cpe_t   *mocaHosts = NULL;
    PLmObjectHostIPAddress pIP;
    
    while (1)
    {
        if(Hosts_stop_scan() )
        {
            PRINTD("\n%s bridge mode, remove all host information\n", __FUNCTION__);
            bridgemode = TRUE;
            pthread_mutex_lock(&LmHostObjectMutex);
            Hosts_RmHosts();
            pthread_mutex_unlock(&LmHostObjectMutex);
            sleep(30);
        }
        else
        {
            if(bridgemode)
            {
                Send_Eth_Host_Sync_Req(); 
#ifndef _CBR_PRODUCT_REQ_ 
                Send_MoCA_Host_Sync_Req(); 
#endif
                SyncWiFi();
                bridgemode = FALSE;
            }

            sleep(30);
            Hosts_SyncDHCP();
            Hosts_SyncArp();
            Add_IPv6_from_Dibbler();
        }
    }
}

void
Hosts_PollHost()
{
    pthread_mutex_lock(&PollHostMutex);
    Hosts_SyncArp();
    Hosts_SyncDHCP();
    pthread_mutex_unlock(&PollHostMutex);
}

static BOOL ValidateHost(char *mac)
{
    char buf[200] = {0};
    FILE *fp = NULL;

    snprintf(buf, sizeof(buf), "ip nei show | grep -i %s > %s", mac, ARP_CACHE);
    system(buf);
    if (NULL == (fp = fopen(ARP_CACHE, "r")))
    {
        return FALSE;
    }
    memset(buf, 0, sizeof(buf));
    if(fgets(buf, sizeof(buf), fp)!= NULL)
    {
        fclose(fp);
        unlink(ARP_CACHE);
        return TRUE;
    }
    else
    {
        fclose(fp);
        fp = NULL;
        unlink(ARP_CACHE);

        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "cat %s | grep -i %s > %s", DNSMASQ_FILE, mac, DNSMASQ_CACHE);
        system(buf);
        if (NULL == (fp = fopen(DNSMASQ_CACHE, "r")))
        {
            CcspTraceWarning(("%s not able to open dnsmasq cache file\n", __FUNCTION__));
            return FALSE;
        }
        memset(buf, 0, sizeof(buf));
        if(NULL != fgets(buf, sizeof(buf), fp))
        {
            fclose(fp);
            unlink(DNSMASQ_CACHE);
            return TRUE;
        }
        fclose(fp);
        unlink(DNSMASQ_CACHE);
    }

    return FALSE;
}

static RetryHostList *CreateValidateHostEntry(ValidateHostQData *pValidateHost)
{
    RetryHostList *pHost;

    pHost = (RetryHostList *)malloc(sizeof(RetryHostList));
    if (pHost)
    {
        memset(pHost, 0, sizeof(RetryHostList));
        strcpy(pHost->host.phyAddr, pValidateHost->phyAddr);
        strcpy(pHost->host.AssociatedDevice, pValidateHost->AssociatedDevice);
        strcpy(pHost->host.ssid, pValidateHost->ssid);
        pHost->host.RSSI = pValidateHost->RSSI;
        pHost->host.Status = pValidateHost->Status;
        pHost->retryCount = 0;
        pHost->next = NULL;
    }

    return pHost;
}

static void UpdateHostRetryValidateList(ValidateHostQData *pValidateHostMsg, int actionFlag)
{
    RetryHostList *pHostNode = NULL;
    RetryHostList *retryList = NULL;
    RetryHostList *prevNode = NULL;

    if (!pValidateHostMsg)
    {
        CcspTraceWarning(("%s Null Param\n",__FUNCTION__));
        return;
    }

    pthread_mutex_lock(&LmRetryHostListMutex);
    retryList = pListHead;
    prevNode = NULL;
    while(retryList)
    {
        if (!strcmp(retryList->host.phyAddr, pValidateHostMsg->phyAddr))
        {
            /* found the mac */
            if (actionFlag == ACTION_FLAG_DEL)
            {
                if (NULL == prevNode)
                {
                    /* First Node */
                    pListHead = retryList->next;
                }
                else
                {
                    prevNode->next = retryList->next;
                }
                free(retryList);
            }
            else if (ACTION_FLAG_ADD == actionFlag)
            {
                /* 
                 * Alreday present in list, if it was off before and now it's on,
                 * update info, and reset the retry count 
                 */
                if (!retryList->host.Status && pValidateHostMsg->Status) {
                    strcpy(retryList->host.AssociatedDevice, pValidateHostMsg->AssociatedDevice);
                    strcpy(retryList->host.ssid, pValidateHostMsg->ssid);
                    retryList->host.RSSI = pValidateHostMsg->RSSI;
                    retryList->host.Status = pValidateHostMsg->Status;
                }
                retryList->retryCount = 0;
            }
            pthread_mutex_unlock(&LmRetryHostListMutex);
            return;
        }
        prevNode = retryList;
        retryList = retryList->next;
    }

    if (ACTION_FLAG_ADD == actionFlag)
    {
        /* Not found in list. Add it. */
        pHostNode = CreateValidateHostEntry(pValidateHostMsg);
        if (!pHostNode)
        {
            CcspTraceWarning(("%s Malloc failed....\n",__FUNCTION__));
            pthread_mutex_unlock(&LmRetryHostListMutex);
            return;
        }
        if (NULL == prevNode)
        {
            /* empty list */
            pListHead = pHostNode;
        }
        else
        {
            /* add at last */
            prevNode->next = pHostNode;
        }
    }

    pthread_mutex_unlock(&LmRetryHostListMutex);
    return;
}

static void RemoveHostRetryValidateList(RetryHostList *pPrevNode, RetryHostList *pHost)
{
    if (NULL == pPrevNode)
    {
        //First Node
        pListHead = pHost->next;
    }
    else
    {
        pPrevNode->next = pHost->next;
    }
    free(pHost);
    return;
}

void *ValidateHostRetry_Thread(void *arg)
{
    RetryHostList *retryList;
    RetryHostList *prevNode = NULL;
    CcspTraceWarning(("%s started\n", __FUNCTION__));
    do
    {
        sleep(MAX_WAIT_VALIDATE_RETRY);
        pthread_mutex_lock(&LmRetryHostListMutex);
        if (pListHead)
        {
            retryList = pListHead;
            prevNode = NULL;
            while(retryList)
            {
                retryList->retryCount++;
                if (TRUE == ValidateHost(retryList->host.phyAddr))
                {
                    Wifi_ServerSyncHost(retryList->host.phyAddr,
                                        retryList->host.AssociatedDevice,
                                        retryList->host.ssid,
                                        retryList->host.RSSI,
                                        retryList->host.Status);
                    /* Valide Host. Remove from Retry Validate list */
                    RemoveHostRetryValidateList(prevNode, retryList);
                    retryList = (NULL == prevNode) ? pListHead : prevNode->next;
                    continue;
                }
                else if (retryList->retryCount >= MAX_COUNT_VALIDATE_RETRY)
                {
                    /* Reached maximum retry. Remove from the Retry Validate list */
                    RemoveHostRetryValidateList(prevNode, retryList);
                    retryList = (NULL == prevNode) ? pListHead : prevNode->next;
                    continue;
                }
                prevNode = retryList;
                retryList = retryList->next;
            }
        }
        pthread_mutex_unlock(&LmRetryHostListMutex);
    } while (1);
    pthread_exit(NULL);
}

void *ValidateHost_Thread(void *arg)
{
    mqd_t mq;
    struct mq_attr attr;

    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 100;
    attr.mq_msgsize = MAX_SIZE_VALIDATE_QUEUE;
    attr.mq_curmsgs = 0;


    /* create the message queue */
    mq = mq_open(VALIDATE_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    CHECK((mqd_t)-1 != mq);

    do
    {
        ssize_t bytes_read;
        ValidateHostQData ValidateHostMsg = {0};

        /* receive the message */
        bytes_read = mq_receive(mq, (char *)&ValidateHostMsg, MAX_SIZE_VALIDATE_QUEUE, NULL);
        CHECK(bytes_read >= 0);

        if (TRUE == ValidateHost(ValidateHostMsg.phyAddr))
        {
            Wifi_ServerSyncHost(ValidateHostMsg.phyAddr,
                                ValidateHostMsg.AssociatedDevice,
                                ValidateHostMsg.ssid,
                                ValidateHostMsg.RSSI,
                                ValidateHostMsg.Status);
            /* Valid Host. Remove from retry list if present */
            UpdateHostRetryValidateList(&ValidateHostMsg, ACTION_FLAG_DEL);
        }
        else
        {
            /* Host is not valide. Add the host details in retry list */
            UpdateHostRetryValidateList(&ValidateHostMsg, ACTION_FLAG_ADD);
        }
    } while(1);
    pthread_exit(NULL);
}

const char compName[25]="LOG.RDK.LM";
void LM_main()
{
    int res;
    void *status;
    char buf[12]; // this value is reading a ULONG
	char buf1[8]; // this is reading an int
    pthread_mutex_init(&PollHostMutex, 0);
    pthread_mutex_init(&LmHostObjectMutex,0);
	pthread_mutex_init(&XLmHostObjectMutex,0);
    pthread_mutex_init(&HostNameMutex,0);
    pthread_mutex_init(&LmRetryHostListMutex, 0);
    lm_wrapper_init();
    lmHosts.hostArray = LanManager_Allocate(LM_HOST_ARRAY_STEP * sizeof(PLmObjectHost));
    lmHosts.sizeHost = LM_HOST_ARRAY_STEP;
    lmHosts.numHost = 0;
    lmHosts.lastActivity = 0;
    lmHosts.availableInstanceNum = 1;

	XlmHosts.hostArray = LanManager_Allocate(LM_HOST_ARRAY_STEP * sizeof(PLmObjectHost));
	XlmHosts.sizeHost = LM_HOST_ARRAY_STEP;
	XlmHosts.numHost = 0;
	XlmHosts.lastActivity = 0;
    XlmHosts.availableInstanceNum = 1;
	
    pComponentName = compName;
	syscfg_init();
	syscfg_get( NULL, "X_RDKCENTRAL-COM_HostVersionId", buf, sizeof(buf));
    if( buf != NULL )
    	{
   		    lmHosts.lastActivity = atol(buf);
			
   		}

	memset(buf1, 0, sizeof(buf1));
	if(syscfg_get( NULL, "X_RDKCENTRAL-COM_HostCountPeriod", buf1, sizeof(buf1)) == 0)
	{
    if( buf1 != NULL )
    	{
   		    g_Client_Poll_interval =  atoi(buf1);
			
   		}
		
	}
	else
		{
			g_Client_Poll_interval = 60;
			strcpy(buf1,"60");
			if (syscfg_set(NULL, "X_RDKCENTRAL-COM_HostCountPeriod" , buf1) != 0) {
                     		     return ANSC_STATUS_FAILURE;
             } else {

                    if (syscfg_commit() != 0)
						{
                            CcspTraceWarning(("X_RDKCENTRAL-COM_HostCountPeriod syscfg_commit failed\n"));
							return ANSC_STATUS_FAILURE;
						}
			 }
		}

	#ifdef FEATURE_SUPPORT_RDKLOG
		rdk_logger_init(DEBUG_INI_NAME);
	#endif
    CcspTraceWarning(("LMLite:rdk initialzed!\n"));
    initparodusTask();

    pthread_t ValidateHost_ThreadID;
    res = pthread_create(&ValidateHost_ThreadID, NULL, ValidateHost_Thread, "ValidateHost_Thread");
    if(res != 0) {
        CcspTraceError(("Create Event_HandlerThread error %d\n", res));
    }

    pthread_t ValidateHostRetry_ThreadID;
    pthread_create(&ValidateHostRetry_ThreadID, NULL, ValidateHostRetry_Thread, "ValidateHostRetry_Thread");
    if(res != 0) {
        CcspTraceError(("Create ValidateHostRetry_Thread error %d\n", res));
    }

	pthread_t Event_HandlerThreadID;
    res = pthread_create(&Event_HandlerThreadID, NULL, Event_HandlerThread, "Event_HandlerThread");
    if(res != 0) {
        CcspTraceError(("Create Event_HandlerThread error %d\n", res));
    }

    Hosts_PollHost();

    sleep(5);
    
    pthread_t Hosts_StatSyncThread;
    res = pthread_create(&Hosts_StatSyncThread, NULL, Hosts_StatSyncThreadFunc, "Hosts_StatSyncThreadFunc");
    if(res != 0) {
        CcspTraceError(("Create Hosts_StatSyncThread error %d\n", res));
    }
	pthread_t Hosts_LogThread;
	res = pthread_create(&Hosts_LogThread, NULL, Hosts_LoggingThread, "Hosts_LoggingThread");
    if(res != 0) {
        CcspTraceError(("Create Hosts_LogThread error %d\n", res));
    }
#ifdef LM_IPC_SUPPORT
    pthread_t Hosts_CmdThread;
    res = pthread_create(&Hosts_CmdThread, NULL, lm_cmd_thread_func, "lm_cmd_thread_func");
    if(res != 0){
        CcspTraceError(("Create lm_cmd_thread_func error %d\n", res));
    }
#endif
#ifdef USE_NOTIFY_COMPONENT
/* Use DBUS instead of socket */
#if 0
	printf("\n WIFI-CLIENT : Creating Wifi_Server_Thread \n");

	pthread_t Wifi_Server_Thread;
	res = pthread_create(&Wifi_Server_Thread, NULL, Wifi_Server_Thread_func, "Wifi_Server_Thread_func");
	if(res != 0){
		CcspTraceWarning(("\n WIFI-CLIENT : Create Wifi_Server_Thread error %d \n",res));
	}
	else
	{
		CcspTraceWarning(("\n WIFI-CLIENT : Create Wifi_Server_Thread success %d \n",res));
	}
	///pthread_join(Wifi_Server_Thread, &status);
#else

#endif /* 0 */
#endif
	if(!Hosts_stop_scan()) {
		 Send_Eth_Host_Sync_Req();
 	#ifndef _CBR_PRODUCT_REQ_ 
		 Send_MoCA_Host_Sync_Req();
 	#endif
		 SyncWiFi( );
	 }
    //pthread_join(Hosts_StatSyncThread, &status);
    //pthread_join(Hosts_CmdThread, &status);
    return 0;

}


char * _CloneString
    (
    const char * src
    )
{
	if(src == NULL) return NULL;
	
    size_t len = strlen(src) + 1;
    if(len <= 1) return NULL;
	
    char * dest = AnscAllocateMemory(len);
    if ( dest )
    {
        strncpy(dest, src, len);
        dest[len - 1] = 0;
    }
	
    return dest;
}

void _init_DM_List(int *num, Name_DM_t **pList, char *path, char *name)
{
    int i;
    char dm[200];
    char (*dmnames)[CDM_PATH_SZ]=NULL;
    int nname = 0;
    int dmlen;
    
    if(*pList != NULL){
        AnscFreeMemory(*pList);
        *pList = NULL;
    }
 
    if((CCSP_SUCCESS == Cdm_GetNames(path, 0, &dmnames, &nname)) && \
            (nname > 0))
    {
        *pList = AnscAllocateMemory(sizeof(Name_DM_t) * nname);
        memset(*pList, 0 , sizeof(Name_DM_t) * nname);
        if(NULL != *pList){
            for(i = 0; i < nname; i++){
			ULONG        ulEntryNameLen = NAME_DM_LEN ;
			parameterValStruct_t varStruct = {0};
			UCHAR      ucEntryParamName[NAME_DM_LEN] = {0}; 
			
			sprintf((*pList)[i].dm ,"%s", dmnames[i]);
			(*pList)[i].dm[strlen((*pList)[i].dm) - 1 ] = '\0';
			sprintf(ucEntryParamName ,"%s%s", dmnames[i], name);
			varStruct.parameterName = ucEntryParamName;
   			varStruct.parameterValue = (*pList)[i].name;
			COSAGetParamValueByPathName(bus_handle,&varStruct,&ulEntryNameLen);

            }
        }
    }

	/* 
	 * To avoid the memory leak of dmnames pointer value
	 */
	if( dmnames )
	{
	  Cdm_FreeNames(dmnames); 
	  dmnames = NULL;
	}
	
    *num = nname;
}

void _get_dmbyname(int num, Name_DM_t *list, char** dm, char* name)
{
    int i;
	
	if(name == NULL)
		return;
	
    for(i = 0; i < num; i++){
        if(NULL != strcasestr(list[i].name, name)){
            STRNCPY_NULL_CHK1((*dm), list[i].dm);
            break;
        }
    }
	
}

int LM_get_host_info()
{

	int i = 0;
	

	if(firstFlg == 0){
        firstFlg = 1;
        return 0;
    }
	
/*
	if(0 == Hosts_stop_scan()){
		printf("bridge mode return 0\n");
		Hosts_PollHost();
	}
*/
	_init_DM_List(&g_IPIfNameDMListNum, &g_pIPIfNameDMList, "Device.IP.Interface.", "Name");
#ifndef _CBR_PRODUCT_REQ_ 
	_init_DM_List(&g_MoCAADListNum, &g_pMoCAADList, "Device.MoCA.Interface.1.AssociatedDevice.", "MACAddress");
#endif
	_init_DM_List(&g_DHCPv4ListNum, &g_pDHCPv4List, "Device.DHCPv4.Server.Pool.1.Client.", "Chaddr");

	pthread_mutex_lock(&LmHostObjectMutex); 

	for(i = 0; i<lmHosts.numHost; i++){

		_get_dmbyname(g_IPIfNameDMListNum, g_pIPIfNameDMList, &(lmHosts.hostArray[i]->Layer3Interface), lmHosts.hostArray[i]->pStringParaValue[LM_HOST_Layer3InterfaceId]);

		if(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_Layer1InterfaceId] != NULL)
		{
			if(strstr(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_Layer1InterfaceId], "MoCA") != NULL){
	        	_get_dmbyname(g_MoCAADListNum, g_pMoCAADList, &(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_AssociatedDeviceId]), lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
			}
		}
		

		if((lmHosts.hostArray[i]->numIPv4Addr) && (lmHosts.hostArray[i]->pStringParaValue[LM_HOST_AddressSource] != NULL))
		{
			if(strstr(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_AddressSource],LM_ADDRESS_SOURCE_DHCP_STR)	!= NULL){
                _get_dmbyname(g_DHCPv4ListNum, g_pDHCPv4List, &(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_DHCPClientId]), lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
                lmHosts.hostArray[i]->LeaseTime = lmHosts.hostArray[i]->ipv4AddrArray->LeaseTime;    		
            }
		}
		
		
	}

	pthread_mutex_unlock(&LmHostObjectMutex); 
	return 0;
}

int XLM_get_host_info()
{

	int i = 0;
	

	if(xfirstFlg == 0){
        xfirstFlg = 1;
        return 0;
    }
	//XHosts_SyncWifi();
	_init_DM_List(&g_IPIfNameDMListNum, &g_pIPIfNameDMList, "Device.IP.Interface.", "Name");
	_init_DM_List(&g_DHCPv4ListNum, &g_pDHCPv4List, "Device.DHCPv4.Server.Pool.2.Client.", "Chaddr"); 

	for(i = 0; i<XlmHosts.numHost; i++){
		Xlm_wrapper_get_info(XlmHosts.hostArray[i]);
	pthread_mutex_lock(&XLmHostObjectMutex);
		_get_dmbyname(g_IPIfNameDMListNum, g_pIPIfNameDMList, &(XlmHosts.hostArray[i]->Layer3Interface), XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_Layer3InterfaceId]);

		if(XlmHosts.hostArray[i]->numIPv4Addr)
		{
			if(strstr(XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_AddressSource],LM_ADDRESS_SOURCE_DHCP_STR)	!= NULL){
                _get_dmbyname(g_DHCPv4ListNum, g_pDHCPv4List, &(XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_DHCPClientId]), XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
            }
		}
		
	pthread_mutex_unlock(&XLmHostObjectMutex);
	}

	return 0;

}

void Wifi_ServerSyncHost (char *phyAddr, char *AssociatedDevice, char *ssid, int RSSI, int Status)
{
	char 	*pos2			= NULL,
			*pos5			= NULL,
			*Xpos2			= NULL,
			*Xpos5			= NULL;
	char radio[32] 			= {0};

	CcspTraceWarning(("%s [%s %s %s %d %d]\n",
									__FUNCTION__,
									(NULL != phyAddr) ? phyAddr : "NULL",
									(NULL != AssociatedDevice) ? AssociatedDevice : "NULL",
									(NULL != ssid) ? ssid : "NULL",
									RSSI,
									Status));
	Xpos2	= strstr( ssid,".3" );
	Xpos5	= strstr( ssid,".4" );
	pos2	= strstr( ssid,".1" );
	pos5	= strstr( ssid,".2" );

	if( ( NULL != Xpos2 ) || \
		( NULL != Xpos5 ) 
	   )
	{
		PLmObjectHost pHost;

		pHost = XHosts_AddHostByPhysAddress(phyAddr);

		if ( pHost )
		{
			Xlm_wrapper_get_info(pHost);

			pthread_mutex_lock(&XLmHostObjectMutex);
			convert_ssid_to_radio(ssid, radio);
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Layer1Interface]), radio);
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), ssid);
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), AssociatedDevice);
			pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = RSSI;
			pHost->l1unReachableCnt = 1;
			pHost->bBoolParaValue[LM_HOST_ActiveId] = Status;
			pHost->activityChangeTime = time((time_t*)NULL);
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");

			if( Status ) 
			{
				if( pHost->ipv4Active == TRUE )
				{
					CcspTraceInfo(("XHS_CONNECTED_CLIENTS: WiFi XHS client online:%s,%s\n",
												pHost->pStringParaValue[LM_HOST_PhysAddressId],
												( pHost->pStringParaValue[LM_HOST_HostNameId] ) ? ( pHost->pStringParaValue[LM_HOST_HostNameId] ) : "NULL"  ));
				}
			}  
			else 
			{
				if( pHost->ipv4Active == TRUE )
				{
					CcspTraceInfo(("XHS_CONNECTED_CLIENTS: WiFi XHS client offline:%s,%s\n",
												pHost->pStringParaValue[LM_HOST_PhysAddressId],
												( pHost->pStringParaValue[LM_HOST_HostNameId] ) ? ( pHost->pStringParaValue[LM_HOST_HostNameId] ) : "NULL"  ));
				}
			}

			pthread_mutex_unlock(&XLmHostObjectMutex);
		}
	}
	else
	{

		LM_wifi_wsta_t hosts;
		memset(&hosts, 0, sizeof(hosts));
		if (AssociatedDevice) {
		    strncpy(hosts.AssociatedDevice, AssociatedDevice,
			sizeof(hosts.AssociatedDevice));
		}
		if (phyAddr) {
		    strncpy(hosts.phyAddr, phyAddr, sizeof(hosts.phyAddr));
		}
		hosts.phyAddr[17] = '\0';
		if (ssid) {
		    strncpy(hosts.ssid, ssid, sizeof(hosts.ssid));
		}
		hosts.RSSI = RSSI;
		hosts.Status = Status;
		EventQData EventMsg;
		mqd_t mq;
        char buffer[MAX_SIZE];
		
		mq = mq_open(EVENT_QUEUE_NAME, O_WRONLY);
        CHECK((mqd_t)-1 != mq);
		memset(buffer, 0, MAX_SIZE);
		EventMsg.MsgType = MSG_TYPE_WIFI;

		memcpy(EventMsg.Msg,&hosts,sizeof(hosts));
		memcpy(buffer,&EventMsg,sizeof(EventMsg));
		CHECK(0 <= mq_send(mq, buffer, MAX_SIZE, 0));
		CHECK((mqd_t)-1 != mq_close(mq));
	}
}

void Wifi_Server_Sync_Function( char *phyAddr, char *AssociatedDevice, char *ssid, int RSSI, int Status )
{
	ValidateHostQData ValidateHostMsg = {0};
	mqd_t mq;

	CcspTraceWarning(("%s [%s %s %s %d %d]\n",
						__FUNCTION__,
						(NULL != phyAddr) ? phyAddr : "NULL",
						(NULL != AssociatedDevice) ? AssociatedDevice : "NULL",
						(NULL != ssid) ? ssid : "NULL",
						RSSI,
						Status));
	mq = mq_open(VALIDATE_QUEUE_NAME, O_WRONLY);
    CHECK((mqd_t)-1 != mq);

	strcpy(ValidateHostMsg.phyAddr, phyAddr);
    strcpy(ValidateHostMsg.AssociatedDevice, AssociatedDevice);
    strcpy(ValidateHostMsg.ssid, ssid);
    ValidateHostMsg.RSSI = RSSI;
    ValidateHostMsg.Status = Status;

	CHECK(0 <= mq_send(mq, (char *)&ValidateHostMsg, MAX_SIZE_VALIDATE_QUEUE, 0));
	CHECK((mqd_t)-1 != mq_close(mq));
}

int Hosts_FindHostIndexByPhysAddress(char * physAddress)
{
    int i = 0;
    for(; i<lmHosts.numHost; i++){
        if(AnscEqualString(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId], physAddress, FALSE)){
            return i;
        }
    }
    return 0;
}

void DelAndShuffleAssoDevIndx(PLmObjectHost pHost)
{
	int x = 0,y = 0,tmp =0, tAP = 0;
	int token = 0,AP = 0;
	char str[100];
	
	x = Hosts_FindHostIndexByPhysAddress(pHost->pStringParaValue[LM_HOST_PhysAddressId]);

	if ( pHost->pStringParaValue[LM_HOST_AssociatedDeviceId] != NULL )
		sscanf(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId],"Device.WiFi.AccessPoint.%d.AssociatedDevice.%d",&AP,&token);

	CcspTraceWarning(("AP = %d token = %d\n",AP,token));
	//printf("AP = %d token = %d\n",AP,token);
// modify uper indexes from token index
    for(y = x-1;y >= 0; y--)
	{
		tmp = 0; tAP = 0;
		if(lmHosts.hostArray[y]->pStringParaValue[LM_HOST_AssociatedDeviceId] != NULL)
		{
		    sscanf(lmHosts.hostArray[y]->pStringParaValue[LM_HOST_AssociatedDeviceId],"Device.WiFi.AccessPoint.%d.AssociatedDevice.%d",&tAP,&tmp);
		}
		else
		continue;
	
		if(AP == tAP)
		{
			if((token < tmp))
			{
				if(strcmp(lmHosts.hostArray[y]->pStringParaValue[LM_HOST_AssociatedDeviceId],"empty"))
				{
					tmp = --tmp;
					memset(str, 0, sizeof(str));
					sprintf(str,"Device.WiFi.AccessPoint.%d.AssociatedDevice.%d",tAP,tmp);
					LanManager_CheckCloneCopy(&(lmHosts.hostArray[y]->pStringParaValue[LM_HOST_AssociatedDeviceId]), str);
				}
			}
		}
	}
	LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), "empty");
	x++;
// modify lower indexes from token index
	for(x;x<lmHosts.numHost;x++)
	{
		tmp = 0; tAP = 0;

		if(lmHosts.hostArray[x]->pStringParaValue[LM_HOST_AssociatedDeviceId] != NULL)
		{
	    		sscanf(lmHosts.hostArray[x]->pStringParaValue[LM_HOST_AssociatedDeviceId],"Device.WiFi.AccessPoint.%d.AssociatedDevice.%d",&tAP,&tmp);
		}
		else
		   continue;
		
		if(AP == tAP)
		{
			if(strcmp(lmHosts.hostArray[x]->pStringParaValue[LM_HOST_AssociatedDeviceId],"empty"))
			{
				if(token < tmp)
				{
					tmp = --tmp;
					memset(str, 0, sizeof(str));
					sprintf(str,"Device.WiFi.AccessPoint.%d.AssociatedDevice.%d",tAP,tmp);
					LanManager_CheckCloneCopy(&(lmHosts.hostArray[x]->pStringParaValue[LM_HOST_AssociatedDeviceId]), str);
				}
			}
		}
	}
}

void MoCA_Server_Sync_Function( char *phyAddr, char *AssociatedDevice, char *ssid, char* parentMac, char* deviceType, int RSSI, int Status )
{
	CcspTraceWarning(("%s [%s %s %s %s %s %d %d]\n",
									__FUNCTION__,
									(NULL != phyAddr) ? phyAddr : "NULL",
									(NULL != AssociatedDevice) ? AssociatedDevice : "NULL",
									(NULL != ssid) ? ssid : "NULL",
									(NULL != parentMac) ? parentMac : "NULL", 
									(NULL != deviceType) ? deviceType : "NULL", 
									RSSI,
									Status));

		LM_moca_cpe_t hosts;
		strcpy(hosts.AssociatedDevice,AssociatedDevice);
		strncpy(hosts.phyAddr,phyAddr,17);
		hosts.phyAddr[17] = '\0';
		strcpy(hosts.ssid,ssid);
		strcpy(hosts.parentMac,parentMac);
		strcpy(hosts.deviceType,deviceType);
		hosts.RSSI = RSSI;
		hosts.Status = Status;
		EventQData EventMsg;
		mqd_t mq;
        char buffer[MAX_SIZE];
		

		mq = mq_open(EVENT_QUEUE_NAME, O_WRONLY);
        CHECK((mqd_t)-1 != mq);
		memset(buffer, 0, MAX_SIZE);
		EventMsg.MsgType = MSG_TYPE_MOCA;

		memcpy(EventMsg.Msg,&hosts,sizeof(hosts));
		memcpy(buffer,&EventMsg,sizeof(EventMsg));
		CHECK(0 <= mq_send(mq, buffer, MAX_SIZE, 0));
		CHECK((mqd_t)-1 != mq_close(mq));
}


void Send_MoCA_Host_Sync_Req()
{
        parameterValStruct_t  value = {"Device.MoCA.X_RDKCENTRAL-COM_MoCAHost_Sync", "true", ccsp_boolean};
        char compo[256] = "eRT.com.cisco.spvtg.ccsp.moca";
        char bus[256] = "/com/cisco/spvtg/ccsp/moca";
        char* faultParam = NULL;
        int ret = CCSP_FAILURE;

        CcspTraceWarning(("%s : Get MoCA Clients \n",__FUNCTION__));
        //printf("%s : Get MoCA Clients \n",__FUNCTION__);
		CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;

        ret = CcspBaseIf_setParameterValues(
                  bus_handle,
                  compo,
                  bus,
                  0,
                  0,
                  &value,
                  1,
                  TRUE,
                  &faultParam
                  );

	if(ret != CCSP_SUCCESS)
	{
		CcspWifiTrace(("RDK_LOG_WARN,MoCA %s : Failed ret %d\n",__FUNCTION__,ret));
		if(faultParam)
		{
			bus_info->freefunc(faultParam);
		}
	}
}

void Send_Eth_Host_Sync_Req()
{
        parameterValStruct_t  value = {"Device.Ethernet.X_RDKCENTRAL-COM_EthHost_Sync", "true", ccsp_boolean};
        char compo[256] = "eRT.com.cisco.spvtg.ccsp.ethagent";
        char bus[256] = "/com/cisco/spvtg/ccsp/ethagent";
        char* faultParam = NULL;
        int ret = CCSP_FAILURE;

        CcspTraceWarning(("%s : Get Ethernet Clients \n",__FUNCTION__));
        //printf("%s : Get Ethernet Clients \n",__FUNCTION__);
		CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;

        ret = CcspBaseIf_setParameterValues(
                  bus_handle,
                  compo,
                  bus,
                  0,
                  0,
                  &value,
                  1,
                  TRUE,
                  &faultParam
                  );

	if(ret != CCSP_SUCCESS)
	{
		CcspWifiTrace(("RDK_LOG_WARN,Ethernet %s : Failed ret %d\n",__FUNCTION__,ret));
		if(faultParam)
		{
			bus_info->freefunc(faultParam);
		}
	}
}

void EthClient_AddtoQueue(char *phyAddr,int Status )
{
		EventQData EventMsg;
		Eth_data EthHost;
		mqd_t mq;
        char buffer[MAX_SIZE];
		
		mq = mq_open(EVENT_QUEUE_NAME, O_WRONLY);
        CHECK((mqd_t)-1 != mq);
		memset(buffer, 0, MAX_SIZE);
		EventMsg.MsgType = MSG_TYPE_ETH;

		strcpy(EthHost.MacAddr,phyAddr);
		EthHost.Active = Status;
		memcpy(EventMsg.Msg,&EthHost,sizeof(EthHost));
		memcpy(buffer,&EventMsg,sizeof(EventMsg));
		CHECK(0 <= mq_send(mq, buffer, MAX_SIZE, 0));
		CHECK((mqd_t)-1 != mq_close(mq));
}

void get_uptime(int *uptime)
{
    struct 	sysinfo info;
    sysinfo( &info );
    *uptime= info.uptime;
}

void convert_ssid_to_radio(char *ssid, char *radio)
{
    if(ssid == NULL){
        CcspTraceWarning(("Empty ssid\n"));
    }
    else{
        if(strstr(ssid,".1") || strstr(ssid,".3")){
               AnscCopyString(radio,"Device.WiFi.Radio.1");
        }
        else if(strstr(ssid,".2") || strstr(ssid,".4")){
               AnscCopyString(radio,"Device.WiFi.Radio.2");
        }
        else{
	       CcspTraceWarning(("Invalid ssid\n"));
        }
    
    }
}

/* LM_FindIPv4BaseFromLink(  ) */
PLmObjectHostIPAddress LM_FindIPv4BaseFromLink( PLmObjectHost pHost, char * ipAddress )
{
	  PLmObjectHostIPAddress pIpAddrList = NULL, pCur = NULL;

	  pIpAddrList = pHost->ipv4AddrArray;

	  for( pCur = pIpAddrList; pCur != NULL; pCur = pCur->pNext )
	  {
		if( AnscEqualString( pCur->pStringParaValue[LM_HOST_IPAddress_IPAddressId], ipAddress, FALSE ) )
		{
			return pCur;
		}
	  }

		return NULL;
}
