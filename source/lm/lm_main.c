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

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

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
#include <curl/curl.h>

#include "ansc_platform.h"
#include "ccsp_base_api.h"
#include "lm_main.h"
#include "lm_util.h"
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
typedef struct _Name_DM 
{
    char name[NAME_DM_LEN];
    char dm[NAME_DM_LEN];
}Name_DM_t;

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
extern ExtenderList *extenderlist;
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
#define LM_HOST_RETRY_LIMIT         3

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
                                    	"X_RDKCENTRAL-COM_Parent", "X_RDKCENTRAL-COM_DeviceType" },
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
                                    	"X_RDKCENTRAL-COM_Parent", "X_RDKCENTRAL-COM_DeviceType" },
    .pIPv4AddressStringParaName = {"IPAddress"},
    .pIPv6AddressStringParaName = {"IPAddress"}
};


/* It may be updated by different threads at the same time? */
ULONG HostsUpdateTime = 0;
ULONG XHostsUpdateTime = 0;


pthread_mutex_t PollHostMutex;
pthread_mutex_t LmHostObjectMutex;
#ifdef USE_NOTIFY_COMPONENT

extern ANSC_HANDLE bus_handle;

void Send_Notification(char* interface, char*mac , BOOL status)
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
	if(status)
		strcpy(status_str,"Connected");
	else
		strcpy(status_str,"Disconnected");
		
	snprintf(str,sizeof(str)/sizeof(str[0]),"Connected-Client,%s,%s,%s",interface,mac,status_str);
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
		printf("\n LMLite <%s> <%d >  Notification Failure %d \n",__FUNCTION__,__LINE__, ret);

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
}

#define LM_SET_ACTIVE_STATE_TIME(x, y) LM_SET_ACTIVE_STATE_TIME_(__LINE__, x, y)
static void LM_SET_ACTIVE_STATE_TIME_(int line, LmObjectHost *pHost,BOOL state){
	char interface[32] = {0};
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

		getAddressSource(pHost->pStringParaValue[LM_HOST_PhysAddressId], addressSource);
		if ( (pHost->pStringParaValue[LM_HOST_AddressSource]) && (strlen(addressSource)))	
		{
   		       LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AddressSource]) , addressSource);
		}
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
					Send_Notification(interface, pHost->pStringParaValue[LM_HOST_PhysAddressId] ,state);
					char buf[8];
					snprintf(buf,sizeof(buf),"%d",lmHosts.lastActivity);
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
		}
		else
		{
			
			{
				if(pHost->bNotify == FALSE)
				{
					
					CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is %s, MacAddress is %s and HostName is %s Connected  \n",interface,pHost->pStringParaValue[LM_HOST_PhysAddressId],pHost->pStringParaValue[LM_HOST_HostNameId]));
					lmHosts.lastActivity++;
					pHost->bClientReady = TRUE;
					//CcspTraceWarning(("RDKB_CONNECTED_CLIENTS:  %s pHost->bClientReady = %d \n",interface,pHost->bClientReady));
					Send_Notification(interface, pHost->pStringParaValue[LM_HOST_PhysAddressId] ,state);
					char buf[8];
					snprintf(buf,sizeof(buf),"%d",lmHosts.lastActivity);
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
        if(NULL != pHost->pStringParaValue[i])
            LanManager_Free(pHost->pStringParaValue[i]);
    if(pHost->objectName != NULL)
        LanManager_Free(pHost->objectName);
	if(pHost->Layer3Interface != NULL)
        LanManager_Free(pHost->Layer3Interface);
	
    Host_FreeIPAddress(pHost, 4);
    Host_FreeIPAddress(pHost, 6);
}

void Hosts_RmHosts(){
    int i;

    if(lmHosts.numHost == 0)
        return;

    for(i = 0; i < lmHosts.numHost; i++){
        Hosts_FreeHost(lmHosts.hostArray[i]);
    }
    LanManager_Free(lmHosts.hostArray);
    lmHosts.availableInstanceNum = 1;
    lmHosts.hostArray = NULL;
    lmHosts.numHost = 0;
    lmHosts.sizeHost = 0;
    lmHosts.lastActivity++;
	char buf[8];
	snprintf(buf,sizeof(buf),"%d",lmHosts.lastActivity);
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
	if(physAddress[2] == ':')
		if(physAddress[5] == ':')
			if(physAddress[8] == ':')
				if(physAddress[11] == ':')
					if(physAddress[14] == ':')
						return TRUE;
					
					
	return FALSE;
}

PLmObjectHost XHosts_AddHostByPhysAddress(char * physAddress)
{
    char comments[256] = {0};
    char ssid[LM_GEN_STR_SIZE]={0};
	if(!validate_mac(physAddress))
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
    return pHost;
}


PLmObjectHost Hosts_AddHostByPhysAddress(char * physAddress)
{
    char comments[256] = {0};
    char ssid[LM_GEN_STR_SIZE]={0};
	if(!validate_mac(physAddress))
	{
		CcspTraceWarning(("RDKB_CONNECTED_CLIENT: Invalid MacAddress ignored\n"));
		return NULL;
	}
		
    if(!physAddress || \
       0 == strcmp(physAddress, "00:00:00:00:00:00")) return NULL;

    if(strlen(physAddress) != MACADDR_SZ-1) return NULL;
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
		pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString("Ethernet");
        
		pHost->pStringParaValue[LM_HOST_AddressSource] = LanManager_CloneString("DHCP");
		pHost->bClientReady = FALSE;
		//CcspTraceWarning(("RDKB_CONNECTED_CLIENT: pHost->bClientReady = %d \n",pHost->bClientReady));
		lmHosts.availableInstanceNum++;
    }
#ifdef USE_NOTIFY_COMPONENT
	
	printf("LMlite-CLIENT <%s> <%d> : Connected Mac = %s \n",__FUNCTION__,__LINE__ ,pHost->pStringParaValue[LM_HOST_PhysAddressId]);
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
        *ppHeader = NULL;
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
    int *num;
    PLmObjectHostIPAddress pIpAddrList, pCur, pPre, *ppHeader;

	if(!ipAddress)
		return NULL;

    if(version == 4){
        num = &(pHost->numIPv4Addr);
        pIpAddrList = pHost->ipv4AddrArray;
        ppHeader = &(pHost->ipv4AddrArray);
		pHost->ipv4Active = TRUE;

									
    }else{
        num = &(pHost->numIPv6Addr);
        pIpAddrList = pHost->ipv6AddrArray;
        ppHeader = &(pHost->ipv6AddrArray);
		pHost->ipv6Active = TRUE;

								
    }

    for(pCur = pIpAddrList; pCur != NULL; pPre = pCur, pCur = pCur->pNext){
        if(AnscEqualString(pCur->pStringParaValue[LM_HOST_IPAddress_IPAddressId], ipAddress, FALSE)){
            if(pCur != pIpAddrList){
                pPre->pNext = pCur->pNext;
                pCur->pNext = pIpAddrList;
                *ppHeader = pCur;
            }
            return pCur;
        }
    }
    pCur = LanManager_Allocate(sizeof(LmObjectHostIPAddress));
    if(pCur == NULL)
        return NULL;

    pCur->pStringParaValue[LM_HOST_IPAddress_IPAddressId] = LanManager_CloneString(ipAddress);
    pCur->pNext = pIpAddrList;
    *ppHeader = pCur;
    (*num)++;
	pCur->instanceNum = *num;
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


char* FindParentIPInExtenderList(char* mac_address)
{
	ExtenderList *tmp = extenderlist;
	char* parent_ip = NULL;
    while(tmp)
    {
		CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender IP [%s] \n", __FUNCTION__, tmp->info->extender_ip));
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender ClientInfoResult [%s] \n", __FUNCTION__, tmp->info->client_info_result));
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender ClientInfolist [%x] \n", __FUNCTION__, tmp->info->list));

	    if(tmp->info->list)
	    {
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender ClientInfoList NumClients [%d] \n", __FUNCTION__, tmp->info->list->numClient));

	    ClientInfo* temp = tmp->info->list->connectedDeviceList;

	    while(temp)
	        {
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender MACAddress [%s] \n", __FUNCTION__, temp->MAC_Address));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender SSID_Type [%s] \n", __FUNCTION__, temp->SSID_Type));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender Device_Name [%s] \n", __FUNCTION__, temp->Device_Name));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender SSID_Name [%s] \n", __FUNCTION__, temp->SSID_Name));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender RSSI [%s] \n", __FUNCTION__, temp->RSSI));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender RxRate [%s] \n", __FUNCTION__, temp->RxRate));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender TxRate [%s] \n", __FUNCTION__, temp->TxRate));
	        if(!strcasecmp(temp->MAC_Address, mac_address))
	        	{
	        		parent_ip = tmp->info->extender_ip;
	        		break;
	        	}
	        temp = temp->next;
	        }
	    }
	tmp = tmp->next;
    }

    return parent_ip;
}

char* FindRSSIInExtenderList(char* mac_address)
{
	ExtenderList *tmp = extenderlist;
	char* rssi = NULL;
    while(tmp)
    {
		CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender IP [%s] \n", __FUNCTION__, tmp->info->extender_ip));
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender ClientInfoResult [%s] \n", __FUNCTION__, tmp->info->client_info_result));
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender ClientInfolist [%x] \n", __FUNCTION__, tmp->info->list));

	    if(tmp->info->list)
	    {
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender ClientInfoList NumClients [%d] \n", __FUNCTION__, tmp->info->list->numClient));

	    ClientInfo* temp = tmp->info->list->connectedDeviceList;

	    while(temp)
	        {
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender MACAddress [%s] \n", __FUNCTION__, temp->MAC_Address));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender SSID_Type [%s] \n", __FUNCTION__, temp->SSID_Type));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender Device_Name [%s] \n", __FUNCTION__, temp->Device_Name));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender SSID_Name [%s] \n", __FUNCTION__, temp->SSID_Name));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender RSSI [%s] \n", __FUNCTION__, temp->RSSI));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender RxRate [%s] \n", __FUNCTION__, temp->RxRate));
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Extender TxRate [%s] \n", __FUNCTION__, temp->TxRate));
	        if(!strcasecmp(temp->MAC_Address, mac_address))
	        	{
	        		rssi = temp->RSSI;
	        		break;
	        	}
	        temp = temp->next;
	        }
	    }
	tmp = tmp->next;
    }

    return rssi;
}

inline int _mac_string_to_array(char *pStr, unsigned char array[6])
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
                     0 == strncmp(pIP4->pStringParaValue[LM_HOST_IPAddress_IPAddressId], "10.", 3) 
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

LMDmlHostsSetHostComment
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
			Xlm_wrapper_get_leasetime();
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
        hosts=NULL;
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

void Hosts_SyncArp()
{
    char comments[256] = {0};
    int count = 0;
    int i;

    PLmObjectHost pHost = NULL;
    LM_host_entry_t *hosts = NULL;
    PLmObjectHostIPAddress pIP;
    lm_wrapper_get_arp_entries("brlan[02]", &count, &hosts);

    if (count > 0)
    {
        pthread_mutex_lock(&LmHostObjectMutex);

        for (i = 0; i < count; i++)
        {
            PRINTD("%s: Process No.%d mac %s\n", __FUNCTION__, i+1, hosts[i].phyAddr);

            pHost = Hosts_AddHostByPhysAddress(hosts[i].phyAddr);

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
                    continue;
                }

		  LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer3InterfaceId]), hosts[i].ifName );
                if ( hosts[i].status == LM_NEIGHBOR_STATE_REACHABLE )
                {

		        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_IPAddressId]), hosts[i].ipAddr );
                    LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);

                    pIP = Host_AddIPv4Address
                    (
                        pHost,
                        hosts[i].ipAddr
                    );

                    if(pIP != NULL){
                        Host_SetIPAddress(pIP, 0, "NONE"); 
                    }


                    _getLanHostComments(hosts[i].phyAddr, comments);
                    if ( comments[0] != 0 )
                    {
                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Comments]), comments);
                    }
                }
                else if(pHost->numIPv4Addr == 0)
                {

                    pIP = Host_AddIPv4Address
                    (
                        pHost,
                        hosts[i].ipAddr
                    );

                    if(pIP != NULL){
                        Host_SetIPAddress(pIP, LM_HOST_RETRY_LIMIT, "NONE"); 
                    }
                }
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
    //pthread_mutex_lock(&LmHostObjectMutex);
    lm_wrapper_get_dhcpv4_client();
    lm_wrapper_get_dhcpv4_reserved();
    //pthread_mutex_unlock(&LmHostObjectMutex);
}

void Hosts_SyncMoCA()
{
    int count = 0;
    int i,j;

    PLmObjectHost pHost;
    LM_moca_cpe_t *hosts = NULL;

    lm_wrapper_get_moca_cpe_list("brlan0", &count, &hosts);

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
		  		LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), hosts[i].ncId);
               	pHost->l1unReachableCnt = 1;
				pthread_mutex_unlock(&LmHostObjectMutex);

				CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Layer1Interface %s \n", __FUNCTION__, pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] ));
				CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s IPAddressId %s \n", __FUNCTION__, pHost->pStringParaValue[LM_HOST_IPAddressId] ));

				//if(!IsExtenderSynced(pHost->pStringParaValue[LM_HOST_IPAddressId]))
				int ret  = QueryMocaExtender(pHost->pStringParaValue[LM_HOST_IPAddressId]);
				if(!ret)						
				{
					//int ret  = QueryMocaExtender(pHost->pStringParaValue[LM_HOST_IPAddressId]);

					CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s QueryMocaExtender ret value %d \n", __FUNCTION__, ret));

					if(!ret)
					{
						pthread_mutex_lock(&LmHostObjectMutex);
						LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
						LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "extender");
						CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Parent Mac %s \n", __FUNCTION__, pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent] ));
						
						LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
						pthread_mutex_unlock(&LmHostObjectMutex);

					}
				}	
				else
				{
					pthread_mutex_lock(&LmHostObjectMutex);
				
					LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");
					CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s LM_HOST_PhysAddressId %s \n", __FUNCTION__, pHost->pStringParaValue[LM_HOST_PhysAddressId] ));

					char* parent_ipAddress = FindParentIPInExtenderList(pHost->pStringParaValue[LM_HOST_PhysAddressId]);
					CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s parent_ipAddress %s  FindMACByIPAddress %s \n", __FUNCTION__, parent_ipAddress, FindMACByIPAddress(parent_ipAddress)));

					if(parent_ipAddress != NULL)
						{
							LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), FindMACByIPAddress(parent_ipAddress));
							char* device_rssi = FindRSSIInExtenderList(pHost->pStringParaValue[LM_HOST_PhysAddressId]);	
                            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s FindRSSIInExtenderList ret value %s \n", __FUNCTION__, device_rssi));
							if(device_rssi)
								pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = atoi(device_rssi);
							LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
						}
					else
						{
							//If parent not determined previously, then it could be either offline device or 
							//a non-compliant extender. Active to be set TRUE for these devices.
							if( pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent] != NULL ) 
							{
								LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);
							}
							else
							{
								LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
							}
						}
					pthread_mutex_unlock(&LmHostObjectMutex);
				}
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

void Hosts_SyncEthernetPort()
{
    int i;
    int port;
    char tmp[20];
    PLmObjectHost pHost;
    pthread_mutex_lock(&LmHostObjectMutex);
    for ( i = 0; i < lmHosts.numHost; i++ )
    {


        pHost = lmHosts.hostArray[i];
        if ( !pHost )
            continue;
        if(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] != NULL &&\
                NULL != strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId], "Ethernet") && \
                pHost->bBoolParaValue[LM_HOST_ActiveId] == TRUE)
        {
            port = lm_wrapper_priv_getEthernetPort(pHost->pStringParaValue[LM_HOST_PhysAddressId]);
            if(port != -1){
                snprintf(tmp,sizeof(tmp)/sizeof(tmp[0]), "Ethernet.%d", port);
                LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), tmp);
            }
       
		LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
		LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");
	}    
    }
    pthread_mutex_unlock(&LmHostObjectMutex);
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

    PLmObjectHost   pHost      = NULL;
    LM_host_entry_t *hosts     = NULL;
    LM_wifi_wsta_t  *wifiHosts = NULL;
    LM_moca_cpe_t   *mocaHosts = NULL;
    PLmObjectHostIPAddress pIP;
    
    while (1)
    {
        PRINTD("\n%s start\n", __FUNCTION__);
        if(Hosts_stop_scan() ){
            PRINTD("\n%s bridge mode, remove all host information\n", __FUNCTION__);
            Hosts_RmHosts();
        }else{
            //pthread_mutex_lock(&LmHostObjectMutex);
            for ( i = 0; i < lmHosts.numHost; i++ )
            {
                pHost = lmHosts.hostArray[i];

                if ( !pHost )
                {
                    continue;
                }
		
                /* This is a WiFi or MoCA host if l1unReachableCnt is NOT zero.
                 * When we cannot find a host in WiFi or MoCA assocaite table for 3 times,
                 * we think the host is offline.
                 */
					if ( pHost->l1unReachableCnt != 0 )
					{
						if(pHost->l1unReachableCnt >= LM_HOST_RETRY_LIMIT)
						{
							/*
								#ifdef USE_NOTIFY_COMPONENT						
									#ifndef IW_EVENT_SUPPORT
															pHost->bNotify = TRUE;
									#endif
								#endif
							*/
							if ((pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]) && \
								 (NULL == strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi"))
								)
							{
								pthread_mutex_lock(&LmHostObjectMutex);
							    LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);
								pthread_mutex_unlock(&LmHostObjectMutex);
							}								
						}
						else
						{
							pHost->l1unReachableCnt++;
						}

						if ((pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]) && \
							 ((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi"))) && \
							 ( TRUE == pHost->bBoolParaValue[LM_HOST_ActiveId])
							)
						{
							pHost->l1unReachableCnt = 1;
						}								
						
						continue;
					}

                /* The left hosts are from ethernet.
                 * We will send arping to this host.
                 * When we see the host as STALE in arp table for 3 times,
                 * we think the host'IP is offline.
                 */
                int offline = 1;
                for(pIP = pHost->ipv4AddrArray; pIP != NULL; pIP = pIP->pNext)
                {
                    if(pIP->l3unReachableCnt + 1 >= LM_HOST_RETRY_LIMIT)
                        offline &= 1;
                    else
                    {
                        pIP->l3unReachableCnt++;
                        offline &=0;
                    }

                    /* Send arping to hosts which are from ethernet */
                    if (pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] &&
                        pHost->pStringParaValue[LM_HOST_PhysAddressId]     &&
                        pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressId])
                    {

                        ret = lm_arping_v4_send(pHost->pStringParaValue[LM_HOST_Layer3InterfaceId],
                                      pHost->pStringParaValue[LM_HOST_PhysAddressId],
                                      pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressId]);
                        PRINTD("%s: arping %s, %s, %s, ret %d\n",
                            __FUNCTION__,
                            pHost->pStringParaValue[LM_HOST_Layer3InterfaceId],
                            pHost->pStringParaValue[LM_HOST_PhysAddressId],
                            pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressId],
                            ret);
                    }
                }
                if(offline)
            	{
					if ((pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]) && \
						 (NULL == strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi"))
						)
					{
						pthread_mutex_lock(&LmHostObjectMutex);
						LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);
						pthread_mutex_unlock(&LmHostObjectMutex);
					}
            	}

            }

            Hosts_SyncMoCA();
            //Hosts_SyncWifi();

            //pthread_mutex_unlock(&LmHostObjectMutex);

            
			Hosts_SyncDHCP(); 

            lm_wrapper_get_arp_entries("brlan0", &count, &hosts);
            if ( count > 0 )
            {
 //               pthread_mutex_lock(&LmHostObjectMutex);
                for (i=0;i<count;i++)
                {
                    pHost = Hosts_FindHostByPhysAddress(hosts[i].phyAddr);
     
                    if ( pHost && pHost->l1unReachableCnt >= LM_HOST_RETRY_LIMIT )
                    {
                        /* This is very tricky. Sometime a mac, which is originally from WiFi or MoCA,
                        *  maybe moved to ethernet later. Its Layer1 unreachable counter will be
                        *  greater than LM_HOST_RETRY_LIMIT. But at the sametime, it is REACHABLE in 
                        *  arp table. We must mark this kind of host's Connection as Etherenet.
                        */
                        if ( hosts[i].status == LM_NEIGHBOR_STATE_REACHABLE )
                        {
                           /* Fix for RDKB-7223 */
                           pthread_mutex_lock(&LmHostObjectMutex);
                           if ( ! pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] || strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId], "MoCA") )
                            LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), "Ethernet" );

                            LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_IPAddressId]), hosts[i].ipAddr);
                            pthread_mutex_unlock(&LmHostObjectMutex);
                            pHost->l1unReachableCnt = 0;
                        }
                    }
                    else if ( pHost && pHost->l1unReachableCnt == 0 )
                    {

                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_IPAddressId]), hosts[i].ipAddr);
                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");
                        if ( hosts[i].status == LM_NEIGHBOR_STATE_REACHABLE )
                        {
                            if(_isIPv6Addr(hosts[i].ipAddr))
                                pIP = Host_AddIPv6Address(pHost, hosts[i].ipAddr);
                            else
                                pIP = Host_AddIPv4Address(pHost, hosts[i].ipAddr);
                            if(pIP != NULL)
                                pIP->l3unReachableCnt = 0;

     						pthread_mutex_lock(&LmHostObjectMutex);
                            LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
                            pthread_mutex_unlock(&LmHostObjectMutex);
                        }
						if ( ! pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] )
                        {
							pthread_mutex_lock(&LmHostObjectMutex);
                            LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), "Ethernet");
                            pthread_mutex_unlock(&LmHostObjectMutex);
                        }
                       	       	
                    }
					else if ( !pHost ) 
                    {
                        pHost = Hosts_AddHostByPhysAddress(hosts[i].phyAddr);

                        if ( pHost )
                        {
                            if(_isIPv6Addr(hosts[i].ipAddr))
                                pIP = Host_AddIPv6Address ( pHost, hosts[i].ipAddr);
                            else
                                pIP = Host_AddIPv4Address ( pHost, hosts[i].ipAddr);
				  pthread_mutex_lock(&LmHostObjectMutex);
			      LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer3InterfaceId]), hosts[i].ifName);
			      LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_IPAddressId]), hosts[i].ipAddr);
			      pthread_mutex_unlock(&LmHostObjectMutex);
                        }
                    }
                }
   //             pthread_mutex_unlock(&LmHostObjectMutex);
            }
            if ( hosts )
            {
                free(hosts);
                hosts = NULL;
            }
            count = 0;

            PRINTD("%s end\n\n", __FUNCTION__);
        }
        sleep(LM_HOST_POLLINGINTERVAL);
    }
}

void
Hosts_PollHost()
{
    pthread_mutex_lock(&PollHostMutex);
    Hosts_SyncArp();
    Hosts_SyncDHCP();
  /*  Hosts_SyncWifi();
    Hosts_SyncMoCA();*/
    Hosts_SyncEthernetPort(); 
    pthread_mutex_unlock(&PollHostMutex);
}

/* Periodically synchronize host table.
 * */
void
Hosts_PollHostThreadFunc()
{
    /* At the very beginning, wait a little bit for P&M to be fully loaded. */
    sleep(10);
    while(TRUE){
        Hosts_PollHost();
        if(LM_HOST_POLLINGINTERVAL > 0)
            sleep(LM_HOST_POLLINGINTERVAL);
        else break;

        //PRINTD("Hosts_PollHost end %d \n", bHostsUpdated);
    }
}

const char compName[25]="LOG.RDK.LM";
void LM_main()
{
    int res;
    void *status;
    char buf[8];
	char buf1[8];
    pthread_mutex_init(&PollHostMutex, 0);
    pthread_mutex_init(&LmHostObjectMutex,0);
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
   		    lmHosts.lastActivity = atoi(buf);
			
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

    curl_global_init(CURL_GLOBAL_ALL);

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
	printf("\n WIFI-CLIENT : Started Syncing WiFi\n");

	SyncWiFi( );
#endif /* 0 */
#endif
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
    char (*dmnames)[CDM_PATH_SZ];
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
        if(NULL != *pList){
            for(i = 0; i < nname; i++){
                dmlen = strlen(dmnames[i]) -1;
                snprintf((*pList)[i].dm ,NAME_DM_LEN -1, "%s%s", dmnames[i], name);
                (*pList)[i].dm[NAME_DM_LEN-1] = '\0';
                if(CCSP_SUCCESS == Cdm_GetParamString((*pList)[i].dm, (*pList)[i].name, NAME_DM_LEN)){
                    (*pList)[i].dm[dmlen] = '\0';
                }
                else
                    (*pList)[i].dm[0] = '\0';

            }
        }
        Cdm_FreeNames(dmnames); 
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

LM_get_host_info()
{

	int i = 0;
	

	if(firstFlg == 0){
        firstFlg = 1;
        return;
    }
	
/*
	if(0 == Hosts_stop_scan()){
		printf("bridge mode return 0\n");
		Hosts_PollHost();
	}
*/
	_init_DM_List(&g_IPIfNameDMListNum, &g_pIPIfNameDMList, "Device.IP.Interface.", "Name");
	_init_DM_List(&g_MoCAADListNum, &g_pMoCAADList, "Device.MoCA.Interface.1.AssociatedDevice.", "MACAddress");
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
		
		
		if(lmHosts.hostArray[i]->numIPv4Addr)
		{
			if(strstr(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_AddressSource],LM_ADDRESS_SOURCE_DHCP_STR)	!= NULL){
                _get_dmbyname(g_DHCPv4ListNum, g_pDHCPv4List, &(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_DHCPClientId]), lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
                lmHosts.hostArray[i]->LeaseTime = lmHosts.hostArray[i]->ipv4AddrArray->LeaseTime;    		
            }
		}
		
		
	}

	pthread_mutex_unlock(&LmHostObjectMutex); 

}

XLM_get_host_info()
{

	int i = 0;
	

	if(xfirstFlg == 0){
        xfirstFlg = 1;
        return;
    }
//	XHosts_SyncWifi();
	_init_DM_List(&g_IPIfNameDMListNum, &g_pIPIfNameDMList, "Device.IP.Interface.", "Name");
	_init_DM_List(&g_DHCPv4ListNum, &g_pDHCPv4List, "Device.DHCPv4.Server.Pool.2.Client.", "Chaddr");

	pthread_mutex_lock(&LmHostObjectMutex); 

	for(i = 0; i<XlmHosts.numHost; i++){

		_get_dmbyname(g_IPIfNameDMListNum, g_pIPIfNameDMList, &(XlmHosts.hostArray[i]->Layer3Interface), XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_Layer3InterfaceId]);

		if(XlmHosts.hostArray[i]->numIPv4Addr)
		{
			if(strstr(XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_AddressSource],LM_ADDRESS_SOURCE_DHCP_STR)	!= NULL){
                _get_dmbyname(g_DHCPv4ListNum, g_pDHCPv4List, &(XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_DHCPClientId]), XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
            }
		}
		
		
	}

	pthread_mutex_unlock(&LmHostObjectMutex); 

}

void Wifi_Server_Sync_Function( char *phyAddr, char *AssociatedDevice, char *ssid, int RSSI, int Status )
{
	char 	*pos2			= NULL,
			*pos5			= NULL,
			*Xpos2			= NULL,
			*Xpos5			= NULL;

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

		pHost = XHosts_FindHostByPhysAddress(phyAddr);
		
		if ( !pHost )
		{
			pHost = XHosts_AddHostByPhysAddress(phyAddr);
			if ( pHost )
			{	   
				CcspTraceWarning(("%s, %d New XHS host added sucessfully\n",__FUNCTION__, __LINE__));
			}
		}
		
		Xlm_wrapper_get_leasetime();

		pthread_mutex_lock(&LmHostObjectMutex);   
		Host_AddIPv4Address ( pHost, pHost->pStringParaValue[LM_HOST_IPAddressId]);
		LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), ssid);
		LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), AssociatedDevice);
		pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = RSSI;
		pHost->l1unReachableCnt = 1;
		pHost->bBoolParaValue[LM_HOST_ActiveId] = Status;
		pHost->activityChangeTime = time((time_t*)NULL);
		LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
		LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");
		pthread_mutex_unlock(&LmHostObjectMutex);
	}
	else
	{
		PLmObjectHost pHost;

		pthread_mutex_lock(&LmHostObjectMutex);   
		
		pHost = Hosts_AddHostByPhysAddress( phyAddr );
		
		if ( NULL != pHost )
		{
			if(pHost->bBoolParaValue[LM_HOST_ActiveId] != Status)
			{
				if( Status )
				{
					LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), ssid);
					LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), AssociatedDevice);
					pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = RSSI;
					pHost->l1unReachableCnt = 1;
					LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
				}
				else
				{
					LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);
				}
			}

			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
			LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");

		    if( Status )
		    {
			    if( NULL != pos2 )
			    {
				    CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is WiFi, MacAddress %s connected(2.4 GHz)\n",phyAddr));
				}
				else if( NULL != pos5 )
				{	
	    			CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is WiFi, MacAddress is %s connected(5 GHz)\n",phyAddr));
		    	}
		    }
		}

		pthread_mutex_unlock(&LmHostObjectMutex);
	}
}
