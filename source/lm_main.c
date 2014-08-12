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

#include "ansc_platform.h"
#include "ccsp_base_api.h"
#include "lm_main.h"
#include "lm_util.h"
#include "lm_wrapper.h"
#include "lm_api.h"
#include "lm_wrapper_priv.h"

#define LM_IPC_SUPPORT
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

/* It may be updated by different threads at the same time? */
ULONG HostsUpdateTime = 0;

pthread_mutex_t PollHostMutex;
pthread_mutex_t LmHostObjectMutex;


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
            if(AnscEqualString(ipv4Addr, lmHosts.hostArray[i]->ipv4AddrArray[j]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId], FALSE))
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
                //Host_SetExtensionParameters(lmHosts.hostArray[i], userData, userDataType);
                break;
            }
        }
    }
}

void Hosts_FreeHost(PLmObjectHost pHost){
    int i;
    if(pHost == NULL)
        return;
    for(i=0; i<LM_HOST_NumStringPara; i++)
        if(NULL != pHost->pStringParaValue[i])
            LanManager_Free(pHost->pStringParaValue[i]);
    if(pHost->objectName != NULL)
        LanManager_Free(pHost->objectName);
    if(pHost->ipv4AddrArray != NULL){
        for(i = 0; i < pHost->numIPv4Addr; i++){
            if(pHost->ipv4AddrArray[i] == NULL)
                continue;
            if (pHost->ipv4AddrArray[i]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId])
            {
                LanManager_Free(pHost->ipv4AddrArray[i]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId]);
            }
            LanManager_Free(pHost->ipv4AddrArray[i]);
        }
        LanManager_Free(pHost->ipv4AddrArray);
    }
    if(pHost->ipv6AddrArray != NULL){
        for(i = 0; i < pHost->numIPv4Addr; i++){
            if(pHost->ipv6AddrArray[i] == NULL)
                continue;
            if (pHost->ipv6AddrArray[i]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId])
            {
                LanManager_Free(pHost->ipv6AddrArray[i]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId]);
            }
            LanManager_Free(pHost->ipv6AddrArray[i]);
        }
      LanManager_Free(pHost->ipv6AddrArray);
    }
    LanManager_Free(pHost);
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
    pHost->sizeIPv4Addr = 0;
    pHost->numIPv4Addr = 0;
    pHost->ipv6AddrArray = NULL;
    pHost->sizeIPv6Addr = 0;
    pHost->numIPv6Addr = 0;

    /* Default it is inactive. */
    pHost->bBoolParaValue[LM_HOST_ActiveId] = FALSE;
    pHost->ipv4Active = FALSE;
    pHost->ipv6Active = FALSE;
    pHost->availableInstanceNumIPv4Address = 1;
    pHost->availableInstanceNumIPv6Address = 1;
    ANSC_UNIVERSAL_TIME currentTime = {0};
    AnscGetLocalTime(&currentTime);
    pHost->activityChangeTime  = AnscCalendarToSecond(&currentTime);

    pHost->iIntParaValue[LM_HOST_X_CISCO_COM_ActiveTimeId] = -1;
    pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = INT_MAX;

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
PLmObjectHostIPv4Address
Host_AddIPv4Address
    (
        PLmObjectHost pHost,
        int instanceNum,
        char * ipv4Address
    )
{
/* USGv2 only support one IPv4 address */
    if(pHost->numIPv4Addr != 0){
        if(AnscEqualString(pHost->ipv4AddrArray[0]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId], ipv4Address, FALSE)){
            return pHost->ipv4AddrArray[0];
        }
        instanceNum = pHost->availableInstanceNumIPv4Address;
        pHost->availableInstanceNumIPv4Address++;
        if (pHost->ipv4AddrArray[0]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId])
        {
            LanManager_Free(pHost->ipv4AddrArray[0]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId]);
        }
        LanManager_Free(pHost->ipv4AddrArray[0]);
        LanManager_Free(pHost->ipv4AddrArray);
        pHost->ipv4AddrArray = NULL;
        pHost->numIPv4Addr = 0;
    }

    PLmObjectHostIPv4Address pIPv4Address = LanManager_Allocate(sizeof(LmObjectHostIPv4Address));
    if(pIPv4Address == NULL)
        return NULL;
    pHost->ipv4AddrArray = LanManager_Allocate(sizeof(PLmObjectHostIPv4Address));
    if(pHost->ipv4AddrArray == NULL){
        LanManager_Free(pIPv4Address);
        return NULL;
    }

    pIPv4Address->instanceNum = instanceNum;
    pIPv4Address->pStringParaValue[LM_HOST_IPv4Address_IPAddressId] = LanManager_CloneString(ipv4Address);

    pIPv4Address->id = 0;
    pHost->ipv4AddrArray[0] = pIPv4Address;
    pHost->numIPv4Addr++;
    return pIPv4Address;
}

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

#ifdef LM_IPC_SUPPORT

inline void _get_host_mediaType(enum LM_MEDIA_TYPE * m_type, char * l1Interfce)
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

inline enum LM_ADDR_SOURCE _get_addr_source(char *source)
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

inline void _get_host_ipaddress(LM_host_t *pDestHost, PLmObjectHost pHost)
{
    int i;
    pDestHost->ipv4AddrAmount = pHost->numIPv4Addr;
    pDestHost->ipv6AddrAmount = pHost->numIPv6Addr;
    LM_ip_addr_t *pIp;
    for(i = 0; i < pHost->numIPv4Addr && i < LM_MAX_IP_AMOUNT;i++){
        pIp = &(pDestHost->ipv4AddrList[i]);
        inet_pton(AF_INET, pHost->ipv4AddrArray[i]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId],pIp->addr);
        // !!!!!!!!!!!!!!!! Lm struct not support mutli address source
        pIp->addrSource = _get_addr_source(pHost->pStringParaValue[LM_HOST_AddressSource]);
    }
    for(i = 0; i < pHost->numIPv6Addr && i < LM_MAX_IP_AMOUNT ;i++){
        pIp = &(pDestHost->ipv6AddrList[i]);
        inet_pton(AF_INET6, pHost->ipv6AddrArray[i]->pStringParaValue[LM_HOST_IPv6Address_IPAddressId],pIp->addr);
        // !!!!!!!!!!!!!!!! Lm struct not support mutli address source
        pIp->addrSource = _get_addr_source(pHost->pStringParaValue[LM_HOST_AddressSource]);
    }
}

inline void _get_host_info(LM_host_t *pDestHost, PLmObjectHost pHost)
{
        mac_string_to_array(pHost->pStringParaValue[LM_HOST_PhysAddressId], pDestHost->phyAddr);
        pDestHost->online = (unsigned char)pHost->bBoolParaValue[LM_HOST_ActiveId];
        _get_host_mediaType(&(pDestHost->mediaType), pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]);
        STRNCPY_NULL_CHK(pDestHost->hostName, pHost->pStringParaValue[LM_HOST_HostNameId], sizeof(pDestHost->comments)-1);
        STRNCPY_NULL_CHK(pDestHost->l3IfName, pHost->pStringParaValue[LM_HOST_Layer3InterfaceId], sizeof(pDestHost->l3IfName)-1);
        STRNCPY_NULL_CHK(pDestHost->l1IfName, pHost->pStringParaValue[LM_HOST_Layer1InterfaceId], sizeof(pDestHost->l1IfName)-1);
        STRNCPY_NULL_CHK(pDestHost->comments, pHost->pStringParaValue[LM_HOST_Comments], sizeof(pDestHost->comments)-1);
        pDestHost->RSSI = pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId];
        _get_host_ipaddress(pDestHost, pHost);
}

inline void _get_hosts_info_cfunc(int fd, void* recv_buf, int buf_size)
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

inline void _get_host_by_mac_cfunc(int fd, void* recv_buf, int buf_size)
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

inline void _set_comment_cfunc(int fd, void* recv_buf, int buf_size)
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
inline void _get_online_device_cfunc(int fd, void* recv_buf, int buf_size){
    int i;
    int num = 0;
    LM_cmd_common_result_t result;
    memset(&result, 0, sizeof(result));
    pthread_mutex_lock(&LmHostObjectMutex);
    for(i = 0; i < lmHosts.numHost; i++){
        if(TRUE == lmHosts.hostArray[i]->bBoolParaValue[LM_HOST_ActiveId]){
            /* Do NOT count TrueStaticIP client */
            if ( 0 == strncmp(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_IPAddressId], "192.168", 7) ||
                 0 == strncmp(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_IPAddressId], "10.", 3) ||
                 0 == strncmp(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_IPAddressId], "172.", 4)
               )
            num++;
        }
    }
    pthread_mutex_unlock(&LmHostObjectMutex);
    result.result = LM_CMD_RESULT_OK;
    result.data.online_num = num;
    write(fd, &result, sizeof(result));
}

inline void _not_support_cfunc(int fd, void* recv_buf, int buf_size){

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
                {
                    LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]);
                }
                pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString(hosts[i].ssid);

                pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId] = hosts[i].RSSI;

                pHost->l1unReachableCnt = 1;

                pHost->bBoolParaValue[LM_HOST_ActiveId] = TRUE;
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
                    Host_AddIPv6Address(pHost, 1, hosts[i].ipAddr);
                    continue;
                }

                if ( pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] )
                {
                    LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer3InterfaceId]);
                }
                pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] = LanManager_CloneString(hosts[i].ifName);

                if ( hosts[i].status == LM_NEIGHBOR_STATE_REACHABLE )
                {
                    pHost->bBoolParaValue[LM_HOST_ActiveId] = TRUE;

                    pHost->l3unReachableCnt = 0;

                    Host_AddIPv4Address
                    (
                        pHost,
                        1,
                        hosts[i].ipAddr
                    );

                    if ( pHost->pStringParaValue[LM_HOST_AddressSource] )
                    {
                        LanManager_Free(pHost->pStringParaValue[LM_HOST_AddressSource]);
                    }
                    pHost->pStringParaValue[LM_HOST_AddressSource] = LanManager_CloneString("NONE");

                    if( pHost->pStringParaValue[LM_HOST_IPAddressId] )
                    {
                        LanManager_Free(pHost->pStringParaValue[LM_HOST_IPAddressId]);
                    }
                    if(pHost->numIPv4Addr)
                    {
                        pHost->pStringParaValue[LM_HOST_IPAddressId] = LanManager_CloneString(pHost->ipv4AddrArray[0]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId]);
                    }

                    _getLanHostComments((char*)hosts[i].phyAddr, comments);
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
                    Host_AddIPv4Address
                    (
                        pHost,
                        1,
                        hosts[i].ipAddr
                    );

                    if( pHost->pStringParaValue[LM_HOST_IPAddressId] )
                    {
                        LanManager_Free(pHost->pStringParaValue[LM_HOST_IPAddressId]);
                    }
                    if(pHost->numIPv4Addr)
                    {
                        pHost->pStringParaValue[LM_HOST_IPAddressId] = LanManager_CloneString(pHost->ipv4AddrArray[0]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId]);
                    }

                    if ( pHost->pStringParaValue[LM_HOST_AddressSource] )
                    {
                        LanManager_Free(pHost->pStringParaValue[LM_HOST_AddressSource]);
                    }
                    pHost->pStringParaValue[LM_HOST_AddressSource] = LanManager_CloneString("DHCP");
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
                pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = LanManager_CloneString("MoCA");

                pHost->l1unReachableCnt = 1;

                pHost->bBoolParaValue[LM_HOST_ActiveId] = TRUE;
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

void Hosts_StatSyncThreadFunc()
{
    int i,count;
    char cmd[64] = {0};
    int ret;

    PLmObjectHost   pHost      = NULL;
    LM_host_entry_t *hosts     = NULL;
    LM_wifi_wsta_t  *wifiHosts = NULL;
    LM_moca_cpe_t   *mocaHosts = NULL;

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
                    (pHost->bBoolParaValue[LM_HOST_ActiveId] = FALSE):(pHost->l1unReachableCnt++);
                    continue;
                }

                /* The left hosts are from ethernet.
                 * We will send arping to this host.
                 * When we see the host as STALE in arp table for 3 times,
                 * we think the host is offline.
                 */
                (pHost->l3unReachableCnt + 1 >= LM_HOST_RETRY_LIMIT)?
                     (pHost->bBoolParaValue[LM_HOST_ActiveId] = FALSE):(pHost->l3unReachableCnt++);

                /* Send arping to hosts which are from ethernet */
                if (pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] &&
                    pHost->pStringParaValue[LM_HOST_PhysAddressId]     &&
                    pHost->ipv4AddrArray[0]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId])
                {

                    ret = lm_arping_v4_send(pHost->pStringParaValue[LM_HOST_Layer3InterfaceId],
                    pHost->pStringParaValue[LM_HOST_PhysAddressId],
                    pHost->ipv4AddrArray[0]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId]);
                    PRINTD("%s: arping %s, %s, %s, ret %d\n",
                        __FUNCTION__,
                        pHost->pStringParaValue[LM_HOST_Layer3InterfaceId],
                        pHost->pStringParaValue[LM_HOST_PhysAddressId],
                        pHost->ipv4AddrArray[0]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId],
                        ret);
                }
            }
            pthread_mutex_unlock(&LmHostObjectMutex);

            Hosts_SyncWifi();
            Hosts_SyncMoCA();

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

                            pHost->l1unReachableCnt = 0;
                        }
                    }
                    else if ( pHost && pHost->l1unReachableCnt == 0 )
                    {
                        if ( hosts[i].status == LM_NEIGHBOR_STATE_REACHABLE )
                        {
                            pHost->bBoolParaValue[LM_HOST_ActiveId] = TRUE;
                            pHost->l3unReachableCnt = 0;
                        }
                    }
                    else if ( !pHost )
                    {
                        pHost = Hosts_AddHostByPhysAddress(hosts[i].phyAddr);

                        if ( pHost )
                        {
                            Host_AddIPv4Address
                            (
                                pHost,
                                1,
                                hosts[i].ipAddr
                            );

                            if( pHost->pStringParaValue[LM_HOST_IPAddressId] )
                            {
                                LanManager_Free(pHost->pStringParaValue[LM_HOST_IPAddressId]);
                            }
                            if(pHost->numIPv4Addr)
                            {
                                pHost->pStringParaValue[LM_HOST_IPAddressId] = LanManager_CloneString(pHost->ipv4AddrArray[0]->pStringParaValue[LM_HOST_IPv4Address_IPAddressId]);
                            }

                            if ( pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] )
                            {
                                LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer3InterfaceId]);
                            }
                            pHost->pStringParaValue[LM_HOST_Layer3InterfaceId] = LanManager_CloneString(hosts[i].ifName);
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
    Hosts_SyncMoCA();

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

void main()
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
