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
#include "ansc_platform.h"
#include "ccsp_base_api.h"
#include "lm_main.h"
#include "lm_util.h"
#include "lm_wrapper.h"
#include "lm_api.h"
#include "lm_wrapper_priv.h"

#define LM_IPC_SUPPORT

#define DEBUG_INI_NAME  "/etc/debug.ini"
extern char*                                pComponentName;
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

#define TIME_NO_NEGATIVE(x) ((long)(x) < 0 ? 0 : (x))

#define STRNCPY_NULL_CHK(x, y, z) if((y) != NULL) strncpy((x),(y),(z)); else  *(unsigned char*)(x) = 0;
#define NAME_DM_LEN  257
#define CDM_PATH_SZ 257

#define STRNCPY_NULL_CHK1(dest, src) { if((dest) != NULL ) AnscFreeMemory((dest));\
                                           (dest) = _CloneString((src));}

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

int g_Client_Poll_interval;
LmObjectHosts lmHosts = {
    .pHostBoolParaName = {"Active"},
    .pHostIntParaName = {"X_CISCO_COM_ActiveTime", "X_CISCO_COM_InactiveTime", "X_CISCO_COM_RSSI"},
    .pHostUlongParaName = {"X_CISCO_COM_DeviceType", "X_CISCO_COM_NetworkInterface", "X_CISCO_COM_ConnectionStatus", "X_CISCO_COM_OSType"},
    .pHostStringParaName = {"Alias", "PhysAddress", "IPAddress", "DHCPClient", "AssociatedDevice", "Layer1Interface", "Layer3Interface", "HostName",
                                        "X_CISCO_COM_UPnPDevice", "X_CISCO_COM_HNAPDevice", "X_CISCO_COM_DNSRecords", "X_CISCO_COM_HardwareVendor",
                                        "X_CISCO_COM_SoftwareVendor", "X_CISCO_COM_SerialNumbre", "X_CISCO_COM_DefinedDeviceType",
                                        "X_CISCO_COM_DefinedHWVendor", "X_CISCO_COM_DefinedSWVendor", "AddressSource", "Comments"},
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

pthread_mutex_t LmHostObjectMutex;
pthread_mutex_t XLmHostObjectMutex;

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
static inline void LM_SET_ACTIVE_STATE_TIME_(int line, LmObjectHost *pHost,BOOL state){
    if(pHost->bBoolParaValue[LM_HOST_ActiveId] != state){

        char addressSource[20] = {0};
	char IPAddress[50] = {0};
	memset(addressSource,0,sizeof(addressSource));
	memset(IPAddress,0,sizeof(IPAddress));
		if ( ! pHost->pStringParaValue[LM_HOST_IPAddressId] )	
		{
			 getIPAddress(pHost->pStringParaValue[LM_HOST_PhysAddressId], IPAddress);
      			 pHost->pStringParaValue[LM_HOST_IPAddressId] = LanManager_CloneString(IPAddress);
		}

		if ( ! pHost->pStringParaValue[LM_HOST_AddressSource] )	
		{
    		        getAddressSource(pHost->pStringParaValue[LM_HOST_PhysAddressId], addressSource);	
   		        pHost->pStringParaValue[LM_HOST_AddressSource] = LanManager_CloneString(addressSource);
		}

		if((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi"))) {
			if(state) {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is WiFi, MacAddress is %s appeared online \n",pHost->pStringParaValue[LM_HOST_PhysAddressId]));
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: IP Address is  %s and address source is %s \n",pHost->pStringParaValue[LM_HOST_IPAddressId],pHost->pStringParaValue[LM_HOST_AddressSource]));
			}  else {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Wifi client with %s MacAddress gone offline \n",pHost->pStringParaValue[LM_HOST_PhysAddressId]));

			}
		}

		else if ((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"MoCA")))
		{
		      if(state) {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is MoCA, MacAddress is %s appeared online \n",pHost->pStringParaValue[LM_HOST_PhysAddressId]));
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: IP Address is  %s and address source is %s \n",pHost->pStringParaValue[LM_HOST_IPAddressId],pHost->pStringParaValue[LM_HOST_AddressSource]));
			}  else {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: MoCA client with %s MacAddress gone offline \n",pHost->pStringParaValue[LM_HOST_PhysAddressId]));

			}
		}

		else if ((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"Ethernet")))
		{
		      if(state) {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is Ethernet, MacAddress is %s appeared online \n",pHost->pStringParaValue[LM_HOST_PhysAddressId]));
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: IP Address is %s and address source is %s \n",pHost->pStringParaValue[LM_HOST_IPAddressId],pHost->pStringParaValue[LM_HOST_AddressSource]));
			}  else {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Ethernet client with %s MacAddress gone offline \n",pHost->pStringParaValue[LM_HOST_PhysAddressId]));

			}
		}

		else 
		{
		      if(state) {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: Client type is %s MacAddress is %s appeared online \n",pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],pHost->pStringParaValue[LM_HOST_PhysAddressId]));
				CcspTraceWarning(("RDK_LOG_WARN,RDKB_CONNECTED_CLIENTS: IP Address is  %s and address source is \n",pHost->pStringParaValue[LM_HOST_IPAddressId],pHost->pStringParaValue[LM_HOST_AddressSource]));
			}  else {
				CcspTraceWarning(("RDKB_CONNECTED_CLIENTS: %s client with %s MacAddress gone offline \n",pHost->pStringParaValue[LM_HOST_PhysAddressId]));

			}
		}
        pHost->bBoolParaValue[LM_HOST_ActiveId] = state;
        pHost->activityChangeTime = time((time_t*)NULL);
		logOnlineDevicesCount();
		PRINTD("%d: mac %s, state %d time %d\n",line ,pHost->pStringParaValue[LM_HOST_PhysAddressId], state, pHost->activityChangeTime);
    }
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

    /* Default it is inactive. */
    pHost->bBoolParaValue[LM_HOST_ActiveId] = FALSE;
    pHost->ipv4Active = FALSE;
    pHost->ipv6Active = FALSE;
    pHost->activityChangeTime  = time(NULL);

    pHost->iIntParaValue[LM_HOST_X_CISCO_COM_ActiveTimeId] = -1;
    pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = -200;

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
#define MACADDR_SZ          18

PLmObjectHost Hosts_AddHostByPhysAddress(char * physAddress)
{
    char comments[256] = {0};

    if(!physAddress || \
       0 == strcmp(physAddress, "00:00:00:00:00:00")) return NULL;

    if(strlen(physAddress) != MACADDR_SZ-1) return NULL;
    PLmObjectHost pHost = Hosts_FindHostByPhysAddress(physAddress);
    if(pHost) return pHost;

    pHost = Hosts_AddHost(lmHosts.availableInstanceNum);
    if(pHost){
        pHost->pStringParaValue[LM_HOST_PhysAddressId] = LanManager_CloneString(physAddress);
        pHost->pStringParaValue[LM_HOST_HostNameId] = LanManager_CloneString(physAddress);
        _getLanHostComments(physAddress, comments);
        if ( comments[0] != 0 )
        {
            pHost->pStringParaValue[LM_HOST_Comments] = LanManager_CloneString(comments);
        }
        pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString("Ethernet");
        lmHosts.availableInstanceNum++;
    }
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
        LanManager_Free(pIpAddrList);
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

    if(version == 4){
        num = &(pHost->numIPv4Addr);
        pIpAddrList = pHost->ipv4AddrArray;
        ppHeader = &(pHost->ipv4AddrArray);
    }else{
        num = &(pHost->numIPv6Addr);
        pIpAddrList = pHost->ipv6AddrArray;
        ppHeader = &(pHost->ipv6AddrArray);
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
    return pCur;
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
	//printf(" Address SRC is %s \n",pIp->addrSource);
        pIp->priFlg = pIpSrc->l3unReachableCnt;
        if(pIp->addrSource == LM_ADDRESS_SOURCE_DHCP)
	{
            pIp->LeaseTime = pIpSrc->LeaseTime;
            pHost->LeaseTime = pIpSrc->LeaseTime;
	}
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
        STRNCPY_NULL_CHK(pDestHost->hostName, pHost->pStringParaValue[LM_HOST_HostNameId], sizeof(pDestHost->comments)-1);
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
    if(0 == Hosts_stop_scan()){
        PRINTD("bridge mode return 0\n");
        Hosts_PollHost();
    }

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
    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", cmd->mac[0], cmd->mac[1], cmd->mac[2], cmd->mac[3], cmd->mac[4], cmd->mac[5]);
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
    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", cmd->mac[0], cmd->mac[1], cmd->mac[2], cmd->mac[3], cmd->mac[4], cmd->mac[5]);

    /* set comment value into syscfg */
    /* we don't check whether this device is in our LmObject list */
    result.result = LM_CMD_RESULT_INTERNAL_ERR;

    if (lm_wrapper_priv_set_lan_host_comments(cmd))
	goto END;

    /* But if this device is in LmObject list, update the comments value */
    pthread_mutex_lock(&LmHostObjectMutex);
    pHost = Hosts_FindHostByPhysAddress(mac);
    if(pHost){
        if(pHost->pStringParaValue[LM_HOST_Comments]){
            LanManager_Free(pHost->pStringParaValue[LM_HOST_Comments]);
        }
        pHost->pStringParaValue[LM_HOST_Comments] = LanManager_CloneString(cmd->comment);
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

void Hosts_SyncWifi()
{
    int count = 0;
    int i;

    PLmObjectHost pHost;
    LM_wifi_wsta_t *hosts = NULL;

    lm_wrapper_get_wifi_wsta_list("brlan0", &count, &hosts);

    if (count > 0)
    {
        pthread_mutex_lock(&LmHostObjectMutex);
        for (i = 0; i < count; i++)
        {
            PRINTD("%s: Process No.%d mac %s\n", __FUNCTION__, i+1, hosts[i].phyAddr);

            pHost = Hosts_FindHostByPhysAddress(hosts[i].phyAddr);

            if ( pHost )
            {
                if ( pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] )
                    LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]);                    
                
                pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString(hosts[i].ssid);
                
                pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = hosts[i].RSSI;

                if(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId] )
                    LanManager_Free(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]);
                
                pHost->pStringParaValue[LM_HOST_AssociatedDeviceId] = LanManager_CloneString(hosts[i].AssociatedDevice);

                pHost->l1unReachableCnt = 1;
                
                LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
            }
        }
        pthread_mutex_unlock(&LmHostObjectMutex);
    }

    if ( hosts )
    {
        free(hosts);
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

                if ( pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] )
                {
                    LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer3InterfaceId]);
                }
                pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] = LanManager_CloneString(hosts[i].ifName);

                if ( hosts[i].status == LM_NEIGHBOR_STATE_REACHABLE )
                {

                pHost->pStringParaValue[LM_HOST_IPAddressId] = LanManager_CloneString(hosts[i].ipAddr);
         
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
                        if ( pHost->pStringParaValue[LM_HOST_Comments] )
                        {
                            LanManager_Free(pHost->pStringParaValue[LM_HOST_Comments]);
                        }
                        pHost->pStringParaValue[LM_HOST_Comments] = LanManager_CloneString(comments);
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
    }

    return;
}

void Hosts_SyncDHCP()
{
    pthread_mutex_lock(&LmHostObjectMutex);
    lm_wrapper_get_dhcpv4_client();
    lm_wrapper_get_dhcpv4_reserved();
    pthread_mutex_unlock(&LmHostObjectMutex);
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
        pthread_mutex_lock(&LmHostObjectMutex);
        for (i = 0; i < count; i++)
        {
            PRINTD("%s: Process No.%d mac %s\n", __FUNCTION__, i+1, hosts[i].phyAddr);

            pHost = Hosts_FindHostByPhysAddress(hosts[i].phyAddr);

            if ( pHost )
            {
                if ( pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] )
                {
                    LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]);                    
                }
                pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString(hosts[i].ncId);

                pHost->l1unReachableCnt = 1;

                LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
            }
        }
        pthread_mutex_unlock(&LmHostObjectMutex);
    }

    if ( hosts )
    {
        free(hosts);
    }

    return;
}

void Hosts_SyncEthernetPort()
{
    int i;
    int port;
    char tmp[20];
    PLmObjectHost pHost;
//    pthread_mutex_lock(&LmHostObjectMutex);
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
                sprintf(tmp, "Ethernet.%d", port);
                pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString(tmp);
            }
        }
    }
//    pthread_mutex_unlock(&LmHostObjectMutex);
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
            pthread_mutex_lock(&LmHostObjectMutex);
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
                    (pHost->l1unReachableCnt >= LM_HOST_RETRY_LIMIT)?
                    (LM_SET_ACTIVE_STATE_TIME(pHost, FALSE)):(pHost->l1unReachableCnt++);
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
                if(offline)LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);

            }
            pthread_mutex_unlock(&LmHostObjectMutex);

            Hosts_SyncWifi();
#ifdef WIFI_MOCA_SUPPORT
            Hosts_SyncMoCA();
#endif
		
	    Hosts_SyncDHCP();
            lm_wrapper_get_arp_entries("brlan0", &count, &hosts);
            if ( count > 0 )
            {
                pthread_mutex_lock(&LmHostObjectMutex);
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
                            if ( pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] )
                            {
                                LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]);                            
                            }

                            pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString("Ethernet");
                            pHost->pStringParaValue[LM_HOST_IPAddressId] = LanManager_CloneString(hosts[i].ipAddr);
                            pHost->l1unReachableCnt = 0;
                        }
                    }
                    else if ( pHost && pHost->l1unReachableCnt == 0 )
                    {

                       pHost->pStringParaValue[LM_HOST_IPAddressId] = LanManager_CloneString(hosts[i].ipAddr);

                        if ( hosts[i].status == LM_NEIGHBOR_STATE_REACHABLE )
                        {
                            if(_isIPv6Addr(hosts[i].ipAddr))
                                pIP = Host_AddIPv6Address(pHost, hosts[i].ipAddr);
                            else
                                pIP = Host_AddIPv4Address(pHost, hosts[i].ipAddr);
                            if(pIP != NULL)
                                pIP->l3unReachableCnt = 0;

     
                            LM_SET_ACTIVE_STATE_TIME(pHost, TRUE);
                        }
			if ( ! pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] )
                        {
                            pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString("Ethernet");
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

                            if ( pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] )
                            {
                                LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer3InterfaceId]);
                            }
                            pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] = LanManager_CloneString(hosts[i].ifName);
			    pHost->pStringParaValue[LM_HOST_IPAddressId] = LanManager_CloneString(hosts[i].ipAddr);
                        }
                    }
                }
                pthread_mutex_unlock(&LmHostObjectMutex);
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
    Hosts_SyncWifi();
#ifdef WIFI_MOCA_SUPPORT
    Hosts_SyncMoCA();
#endif
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

        for(pCur = pIpAddrList, i=0; (pCur != NULL) && (i < nIndex); pCur =     pCur->pNext,i++);

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

void _get_LanMode(char LanMode[256]) //RDKB-EMU
{
	FILE *fp = NULL;
        char path[256] = {0};
        int count = 0;
        system("dmcli simu getv Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanMode | grep value | cut -f3 -d : | cut -f2 -d ' ' > /tmp/LanMode.txt");
        fp = popen("cat /tmp/LanMode.txt","r");
        if(fp == NULL)
        {
                printf("Failed to run command in Function %s\n",__FUNCTION__);
                return 0;
        }
        if(fgets(path, sizeof(path)-1, fp) != NULL)
        {
                for(count=0;path[count]!='\n';count++)
                        LanMode[count]=path[count];
                LanMode[count]='\0';
        }
        pclose(fp);
}
int LM_get_online_device()
{
	int i;
	int num = 0;
	PLmObjectHostIPAddress pIP4;
	char LanMode[256] = {0};
	_get_LanMode(LanMode);
	if(strcmp(LanMode,"bridge-static") == 0)
	{
		return num;
	}
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

void DelAndShuffleAssoDevIndx(PLmObjectHost pHost)
{
        int x = 0,y = 0,tmp =0, tAP = 0;
        int token = 0,AP = 0;
        char str[100];

        x = Hosts_FindHostIndexByPhysAddress(pHost->pStringParaValue[LM_HOST_PhysAddressId]);
        sscanf(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId],"Device.WiFi.AccessPoint.%d.AssociatedDevice.%d",&AP,&token);
        printf("AP = %d token = %d\n",AP,token);
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
    CcspTraceWarning(("New XHS host added sucessfully\n"));
    return pHost;
}

Wifi_Server_Sync_Function( char *phyAddr, char *AssociatedDevice, char *ssid, int RSSI, int Status )
{
        char    *pos2                   = NULL,
                        *pos5                   = NULL,
                        *Xpos2                  = NULL,
                        *Xpos5                  = NULL;

        CcspTraceWarning(("%s [%s %s %s %d %d]\n",
                                                                        __FUNCTION__,
                                                                        (NULL != phyAddr) ? phyAddr : "NULL",
                                                                        (NULL != AssociatedDevice) ? AssociatedDevice : "NULL",
                                                                        (NULL != ssid) ? ssid : "NULL",
                                                                        RSSI,
                                                                        Status));
        Xpos2   = strstr( ssid,".3" );
        Xpos5   = strstr( ssid,".4" );
        pos2    = strstr( ssid,".1" );
        pos5    = strstr( ssid,".2" );

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
                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]), ssid);
                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_AssociatedDeviceId]), AssociatedDevice);
                        pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = RSSI;
                        pHost->l1unReachableCnt = 1;
                        pHost->bBoolParaValue[LM_HOST_ActiveId] = Status;
                        pHost->activityChangeTime = time((time_t*)NULL);
                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]), getFullDeviceMac());
                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]), "empty");
                        pthread_mutex_unlock(&XLmHostObjectMutex);
                }
        }
        else
        {
                PLmObjectHost pHost;

                pthread_mutex_lock(&LmHostObjectMutex);
		 pHost = Hosts_AddHostByPhysAddress( phyAddr );

                if ( NULL != pHost )
                {
                        if((pHost->bBoolParaValue[LM_HOST_ActiveId] != Status) || ((pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]) && (strcmp(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],ssid))))
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
                                        if(!strcmp(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],ssid))
                                        {

                                           DelAndShuffleAssoDevIndx(pHost);
                                           LM_SET_ACTIVE_STATE_TIME(pHost, FALSE);
                                        }
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

LM_get_host_info(ANSC_HANDLE hContext,PULONG pulCount)
{

	int i = 0;

#if 1 //RDKB-EMU
	PCOSA_DML_HOST_ENTRY            pHostEntry    = NULL;
	LM_hosts_t hosts;
	LM_host_t *plmHost;
	PLmObjectHost pHost;
	char str[100];
	//    int i;
	int ret;
	BOOL   bridgeMode;

	Hosts_RmHosts();
	char LanMode[256] = {0};
	_get_LanMode(LanMode);
	if(strcmp(LanMode,"bridge-static") == 0)
	{
		*pulCount = 0;
	}
	else
	{
		if(LM_RET_SUCCESS == lm_get_all_hosts(&hosts))
		{
			*pulCount = hosts.count;
			for(i = 0; i < hosts.count; i++){
				plmHost = &(hosts.hosts[i]);
				/* filter unwelcome device */
				sprintf(str,"%02x:%02x:%02x:%02x:%02x:%02x", plmHost->phyAddr[0], plmHost->phyAddr[1], plmHost->phyAddr[2], plmHost->phyAddr[3], plmHost->phyAddr[4], plmHost->phyAddr[5]);
				pHost = Hosts_AddHostByPhysAddress(str);
				if(pHost == NULL)
					continue;
				_get_host_info(plmHost, pHost);
			}
		}else{
			*pulCount = 0;
		}
	}
#endif
#if 0
	if(firstFlg == 0){
		printf(" In FIrst Loop \n");
		firstFlg = 1;
		return;
	}
#endif

	/*
	   if(0 == Hosts_stop_scan()){
	   printf("bridge mode return 0\n");
	   Hosts_PollHost();
	   }
	 */
#if 0
	_init_DM_List(&g_IPIfNameDMListNum, &g_pIPIfNameDMList, "Device.IP.Interface.", "Name");
#if 0 //RDKB-EMU
#ifndef _CBR_PRODUCT_REQ_ 
	_init_DM_List(&g_MoCAADListNum, &g_pMoCAADList, "Device.MoCA.Interface.1.AssociatedDevice.", "MACAddress");
#endif
#endif
	_init_DM_List(&g_DHCPv4ListNum, &g_pDHCPv4List, "Device.DHCPv4.Server.Pool.1.Client.", "Chaddr");

	pthread_mutex_lock(&LmHostObjectMutex);

	for(i = 0; i<lmHosts.numHost; i++){

		_get_dmbyname(g_IPIfNameDMListNum, g_pIPIfNameDMList, &(lmHosts.hostArray[i]->Layer3Interface), lmHosts.hostArray[i]->pStringParaValue[LM_HOST_Layer3InterfaceId]);

		/*         if(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_Layer1InterfaceId] != NULL)
			   {
			   if(strstr(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_Layer1InterfaceId], "MoCA") != NULL){
			   _get_dmbyname(g_MoCAADListNum, g_pMoCAADList, &(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_AssociatedDeviceId]), lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
			   }
			   }


			   if(lmHosts.hostArray[i]->numIPv4Addr)
			   {
			   if(strstr(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_AddressSource],LM_ADDRESS_SOURCE_DHCP_STR)     != NULL){
			   _get_dmbyname(g_DHCPv4ListNum, g_pDHCPv4List, &(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_DHCPClientId]), lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
			   lmHosts.hostArray[i]->LeaseTime = lmHosts.hostArray[i]->ipv4AddrArray->LeaseTime;
			   }
			   }*/


	}

	pthread_mutex_unlock(&LmHostObjectMutex);
#endif
}

XLM_get_host_info()
{

        int i = 0;


        if(xfirstFlg == 0){
        xfirstFlg = 1;
        return;
    }
	_init_DM_List(&g_IPIfNameDMListNum, &g_pIPIfNameDMList, "Device.IP.Interface.", "Name");
        _init_DM_List(&g_DHCPv4ListNum, &g_pDHCPv4List, "Device.DHCPv4.Server.Pool.2.Client.", "Chaddr");

        for(i = 0; i<XlmHosts.numHost; i++){
                Xlm_wrapper_get_info(XlmHosts.hostArray[i]);
        pthread_mutex_lock(&XLmHostObjectMutex);
                _get_dmbyname(g_IPIfNameDMListNum, g_pIPIfNameDMList, &(XlmHosts.hostArray[i]->Layer3Interface), XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_Layer3InterfaceId]);

                if(XlmHosts.hostArray[i]->numIPv4Addr)
                {
                        if(strstr(XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_AddressSource],LM_ADDRESS_SOURCE_DHCP_STR)    != NULL){
                _get_dmbyname(g_DHCPv4ListNum, g_pDHCPv4List, &(XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_DHCPClientId]), XlmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
            }
                }

        pthread_mutex_unlock(&XLmHostObjectMutex);
        }



}

const char compName[25]="LOG.RDK.LM";
void LM_main()
{
    int res;
    void *status;
    pthread_mutex_init(&PollHostMutex, 0);
    pthread_mutex_init(&LmHostObjectMutex,0);
    lm_wrapper_init();
    lmHosts.hostArray = LanManager_Allocate(LM_HOST_ARRAY_STEP * sizeof(PLmObjectHost));
    lmHosts.sizeHost = LM_HOST_ARRAY_STEP;
    lmHosts.numHost = 0;
    lmHosts.availableInstanceNum = 1;


    pComponentName = compName;

#ifdef FEATURE_SUPPORT_RDKLOG
    rdk_logger_init(DEBUG_INI_NAME);
#endif
    CcspTraceWarning(("LMLite:rdk initialzed!\n"));

    Hosts_PollHost();

    sleep(5);

    pthread_t Hosts_StatSyncThread;
    res = pthread_create(&Hosts_StatSyncThread, NULL, Hosts_StatSyncThreadFunc, "Hosts_StatSyncThreadFunc");
    if(res != 0) {
        CcspTraceError(("Create Hosts_StatSyncThread error %d\n", res));
    }
#ifdef LM_IPC_SUPPORT
    pthread_t Hosts_CmdThread;
    res = pthread_create(&Hosts_CmdThread, NULL, lm_cmd_thread_func, "lm_cmd_thread_func");
    if(res != 0){
        CcspTraceError(("Create lm_cmd_thread_func error %d\n", res));
    }
#endif

    pthread_join(Hosts_StatSyncThread, &status);
    pthread_join(Hosts_CmdThread, &status);
    return 0;

}
