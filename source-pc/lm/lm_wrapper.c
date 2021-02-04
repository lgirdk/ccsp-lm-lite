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

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <netpacket/packet.h>
#include "secure_wrapper.h"
#include "ansc_platform.h"
#include "ccsp_base_api.h"
#include "lm_wrapper.h"
#include "lm_main.h"
#include "lm_util.h"


/* Fix RDKB-499 */
#define DHCPV4_RESERVED_FORMAT  "%17[^,],%63[^,],%63[^,]"
#define LM_DHCP_CLIENT_FORMAT   "%63d %17s %63s %63s"
#define LM_ARP_ENTRY_FORMAT  "%63s %63s %63s %63s %17s %63s"

extern ANSC_HANDLE bus_handle;
char *pERTPAMComponentName = NULL;
char *pERTPAMComponentPath = NULL;
extern pthread_mutex_t LmHostObjectMutex;
#define WIFI_DM_CHANNEL      "Device.WiFi.Radio.%d.Channel"
#define WIFI_DM_AUTOCHAN     "Device.WiFi.Radio.%d.AutoChannelEnable"
#define WIFI_DM_BSS_SECURITY_MODE "Device.WiFi.AccessPoint.%d.Security.ModeEnabled"
#define WIFI_DM_BSS_SECURITY_ENCRYMODE "Device.WiFi.AccessPoint.%d.Security.X_CISCO_COM_EncryptionMethod"
#define WIFI_DM_SSID         "Device.WiFi.SSID.%d.SSID"
static char pAtomBRMac[32] = {0};
#define WIFI_DM_BSSID         "Device.WiFi.SSID.1.BSSID"


ExtenderList *extenderlist = NULL;
extern pthread_mutex_t XLmHostObjectMutex;
static int fd;
pthread_mutex_t GetARPEntryMutex;

int LanManager_DiscoverComponent
    (
    )
{
    char CrName[256] = {0};
    int ret = 0;

    CrName[0] = 0;
    #if 0//RDKB-EMULATOR
    strcpy(CrName, "eRT.");
    strcat(CrName, CCSP_DBUS_INTERFACE_CR);
    #endif
    strcpy(CrName, CCSP_DBUS_INTERFACE_CR);//RDKB-EMULATOR

    componentStruct_t **components = NULL;
    int compNum = 0;
    int res = CcspBaseIf_discComponentSupportingNamespace (
            bus_handle,
            CrName,
            "Device.WiFi.AccessPoint.",
            "",
            &components,
            &compNum);
    if(res != CCSP_SUCCESS || compNum < 1){
        CcspTraceError(("LanManager_DiscoverComponent find eRT PAM component error %d\n", res));
        ret = -1;
    }
    else{
        pERTPAMComponentName = LanManager_CloneString(components[0]->componentName);
        pERTPAMComponentPath = LanManager_CloneString(components[0]->dbusPath);
        CcspTraceInfo(("LanManager_DiscoverComponent find eRT PAM component %s--%s\n", pERTPAMComponentName, pERTPAMComponentPath));
    }
    free_componentStruct_t(bus_handle, compNum, components);
    return ret;
}

int mac_string_to_array(char *pStr, unsigned char array[6])
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

int ip_string_to_arrary(char* pStr, unsigned char array[4])
{
    int tmp[4],n,i;

    memset(array,0,4);
    n = sscanf(pStr, "%d.%d.%d.%d", &tmp[0],&tmp[1],&tmp[2],&tmp[3]);
    if (n==4)
    {
        for (i=0;i<n;i++)
        {
            array[i] = (unsigned char)tmp[i];
        }
        return 0;
    }

    return -1;
}

int lm_arping_v4_send(char netName[64], char strMac[17], unsigned char ip[]){
    struct ifreq ifr;
    char m_mac[6], m_ip[4];
    //struct sockaddr_in saddr;
    struct arp_pkt arp_req;
    struct sockaddr_ll reqsa;
    struct sockaddr_in *ip_sockaddr;
    unsigned char phyAddr[6];
    unsigned char t_ip[4];

    memset(&ifr, 0, sizeof(ifr));
    memset(&arp_req, 0, sizeof(arp_req));
    memset(&reqsa, 0, sizeof(reqsa));
    mac_string_to_array(strMac,phyAddr);
    ip_string_to_arrary(ip, t_ip);

    reqsa.sll_ifindex = if_nametoindex(netName);
    reqsa.sll_ifindex = if_nametoindex(netName);
    if(fd < 0){
        fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
        if (fd < 0)
        {
            printf(("LM %s create socket erro\nr", __FUNCTION__));
            return -1;
        }
    }

    strncpy(ifr.ifr_name, netName, IFNAMSIZ);

    /* get interface mac address */
    if(-1 == ioctl(fd, SIOCGIFHWADDR, &ifr)){
        printf(("LM %s ioctl get %s HW addr error\n", __FUNCTION__, netName));
        return -1;
    }
    memcpy(m_mac, ifr.ifr_hwaddr.sa_data, 6);
    if(-1 == ioctl(fd, SIOCGIFADDR, &ifr)){
        printf(("LM %s ioctl get %s IP addr error\n", __FUNCTION__, netName));
        return -1;
    }
    ip_sockaddr = (struct sockaddr_in *)&(ifr.ifr_addr);
    memcpy(m_ip, (void *)&(ip_sockaddr->sin_addr), 4);

    /* set ethernet header */
    memcpy(arp_req.ether_dhost, phyAddr, 6);
    memcpy(arp_req.ether_shost, m_mac , 6);

    /* set arp requrest */
    arp_req.ether_type = htons(0x0806);
    arp_req.hw_type = htons(1);
    arp_req.pro_type = htons(0x0800);
    arp_req.hw_size = 6;
    arp_req.pro_size = 4;
    arp_req.opcode = htons(1);
    memcpy(arp_req.sMac, m_mac, 6);
    memcpy(arp_req.sIP, m_ip, 4);
    memcpy(arp_req.tIP, t_ip, 4);

    /* send packet */
    if( 0 >= sendto(fd, &arp_req, sizeof(arp_req), 0, (struct sockaddr *)&reqsa, sizeof(reqsa))){
        printf("LM %s send packet error, errno = %d\n", __FUNCTION__, errno);
        return -1;
    }

    return 0;
}

int lm_wrapper_init(){
    int ret, i = 0;
    pthread_mutex_init(&GetARPEntryMutex, 0);
    ret = CCSP_Message_Bus_Init(
                "ccsp.lmbusclient",
                CCSP_MSG_BUS_CFG,
                &bus_handle,
                LanManager_Allocate,
                LanManager_Free
            );

    if ( ret != ANSC_STATUS_SUCCESS )
    {
        CcspTraceError((" !!! SSD Message Bus Init ERROR !!!\n"));
        return -1;
    }

    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if(fd < 0){
        printf(("LM %s create socket erro\nr", __FUNCTION__));
        return -1;
    }

    LanManager_DiscoverComponent();
    return 0;
}

int lm_wrapper_get_moca_cpe_list(char netName[LM_NETWORK_NAME_SIZE], int *pCount, LM_moca_cpe_t **ppArray)
{
    int n=0,i,ret;
    moca_cpe_list cpes[kMoca_MaxCpeList];
    LM_moca_cpe_t *pMoca = NULL;

#ifdef CONFIG_SYSTEM_MOCA
    ret = moca_GetMocaCPEs(0, cpes, &n);
#endif

    if(n <= 0){
        *pCount = 0;
        return 0;
    }

    pMoca = (LM_moca_cpe_t *)malloc(sizeof(LM_moca_cpe_t) * n );
    if(pMoca == NULL){
        *pCount = 0;
        return -1;
    }

    *pCount = n;
    *ppArray = pMoca;

    for(i = 0; i < n;i++){
         sprintf(pMoca->phyAddr, "%02x:%02x:%02x:%02x:%02x:%02x",cpes[i].mac_addr[0],cpes[i].mac_addr[1],cpes[i].mac_addr[2],cpes[i].mac_addr[3],cpes[i].mac_addr[4],cpes[i].mac_addr[5]);
         strncpy(pMoca->ncId, "Device.MoCA.Interface.1", LM_GEN_STR_SIZE);
         pMoca++;
    }

    return 0;
}
static int _get_field_pos( char *field[], int field_num, int *pos, parameterValStruct_t **val, int val_num){
    int i, j;
    for(j = 0; j < field_num; j++){
        pos[j] = -1;
    }
    for(i = 0; i < val_num; i++){
        for(j = 0; j < field_num; j++){
            if( NULL != strstr(val[i]->parameterName, field[j])){
               if(pos[j] != -1){
                  i -= pos[j];
                  goto OUT;
               }else
                  pos[j] = i;
            }
        }
    }
OUT:
    for(j = 0; j < field_num; j++){
        if(pos[j] == -1)
            return 0;
    }
    return (i);
}


int lm_wrapper_get_wifi_wsta_list(char netName[LM_NETWORK_NAME_SIZE], int *pCount, LM_wifi_wsta_t **ppWstaArray)
{
    char *tblName = DEVICE_WIFI_ACCESS_POINT;
    char *pWiFiComponentName = pERTPAMComponentName;
    char *pComponentPath = pERTPAMComponentPath;
    int interface_number = 0;
    int bkup_ifaceNumber = 0;
    int AssociatedDevice_number[LM_MAX_INTERFACE_NUMBER];
    parameterInfoStruct_t **interfaceInfo;
    int i, j, itmp;
    int ret;
    LM_wifi_wsta_t *pwifi_wsta = NULL;
    char *field[2] = {"MACAddress", "SignalStrength"};
    int field_num = 0;
    int pos[2];
    int rVal = -1;

    *pCount = 0;
    PRINTD("ENT %s\n", __FUNCTION__);

     char  *paramAtomMacName[1] ;
     parameterValStruct_t    **valStructs = NULL;
     int valNum = 0;
     char br0Mac[128]  = {'\0'};

    if(pComponentPath == NULL || pWiFiComponentName == NULL)
    {
        if(-1 == LanManager_DiscoverComponent()){
            CcspTraceError(("%s ComponentPath or pWiFiComponentName is NULL\n", __FUNCTION__));
            return rVal;
        }
    }

    /* Get parameter name of Device.WiFi.AccessPoint. */
    ret = CcspBaseIf_getParameterNames(
        bus_handle,
        pWiFiComponentName,
        pComponentPath,
        tblName,
        1,
        &bkup_ifaceNumber,
        &interfaceInfo);
    if(ret != CCSP_Message_Bus_OK) {
        CcspTraceError(("%s CcspBaseIf_getParameterNames %s error %d!\n", __FUNCTION__, tblName, ret));
        return -1;
    }

    /* if netName is "brlan0",  need to update only private network.*/
            interface_number = 2 ;
       //   interface_number = 1;

    char *(*pReferenceParaNameArray)[] = malloc(sizeof(char*) * interface_number);
    if(pReferenceParaNameArray == NULL)
        goto RET1;

    char *(*pAssociatedDeviceNameArray)[] = malloc(sizeof(char*) * interface_number * 2);
    if(pAssociatedDeviceNameArray == NULL)
        goto RET2;

    char* pReferenceParaNameBuf = malloc( interface_number * 100 );
    if(pReferenceParaNameBuf == NULL)
        goto RET3;

    char* pAssociatedDeviceNameBuf = malloc( interface_number * 100 * 2);
    if(pAssociatedDeviceNameBuf == NULL)
        goto RET4;

    char (* pReferenceParaName)[100] =  (char (*)[100])pReferenceParaNameBuf;
    char (* pAssociatedDeviceName)[100] = (char (*)[100])pAssociatedDeviceNameBuf;

    for(i=0; i < interface_number; i++){
        strcpy(pReferenceParaName[i], interfaceInfo[i]->parameterName);
        strcpy(pAssociatedDeviceName[i], interfaceInfo[i]->parameterName);
        strcpy(pAssociatedDeviceName[i + interface_number], interfaceInfo[i]->parameterName);
        strcat(pReferenceParaName[i], "SSIDReference");
        strcat(pAssociatedDeviceName[i + interface_number], "AssociatedDeviceNumberOfEntries");
        strcat(pAssociatedDeviceName[i], "AssociatedDevice.");
        (*pReferenceParaNameArray)[i] = pReferenceParaName[i];
        (*pAssociatedDeviceNameArray)[i] = pAssociatedDeviceName[i];
        (*pAssociatedDeviceNameArray)[i + interface_number] = pAssociatedDeviceName[i + interface_number];
    }

    /* get SSID reference
     * like Device.WiFi.SSID.1. */
    parameterValStruct_t **parametervalSSIDRef;
    int ref_size = 0 ;
    ret = CcspBaseIf_getParameterValues(
            bus_handle,
            pWiFiComponentName,
            pComponentPath,
            *pReferenceParaNameArray,
            interface_number,
            &ref_size,
            &parametervalSSIDRef);
    if(ret != CCSP_Message_Bus_OK) {
            CcspTraceError(("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, pReferenceParaName, ret));
            printf("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, pReferenceParaName, ret);
            goto RET5;
    }

    /* get Associated Device Number
     * Device.WiFi.AccessPoint.1.AssociatedDevice. */
    parameterValStruct_t **parametervalAssociatedDeviceNum;
    int num_size = 0;
    ret = CcspBaseIf_getParameterValues(
            bus_handle,
            pWiFiComponentName,
            pComponentPath,
            *pAssociatedDeviceNameArray,
            interface_number * 2,
            &num_size,
            &parametervalAssociatedDeviceNum);
    if(ret != CCSP_Message_Bus_OK){
        CcspTraceError(("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, pAssociatedDeviceName[0], ret));
        printf("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, pAssociatedDeviceName[0], ret);
        goto RET6;
    }
    //for(i = 0; i < num_size; i++){
    //    PRINTD("%s %s \n",parametervalAssociatedDeviceNum[i]->parameterName, parametervalAssociatedDeviceNum[i]->parameterValue);
    //}


    field_num = _get_field_pos(field, 2, pos, parametervalAssociatedDeviceNum, num_size - interface_number);
    if(field_num == 0)
        goto RET7;
    for(*pCount = 0, i = 0, j = num_size - interface_number; i < interface_number; i++, j++){
        AssociatedDevice_number[i] = atoi(parametervalAssociatedDeviceNum[j]->parameterValue);
        PRINTD("AssociatedDevice_number %d \n", AssociatedDevice_number[i]);
        (*pCount) += AssociatedDevice_number[i];
    }
    if(*pCount != (num_size - interface_number)/field_num)
        goto RET7;

    pwifi_wsta = malloc(sizeof(LM_wifi_wsta_t) * (*pCount));
    if(pwifi_wsta == NULL )
        goto RET7;

    *ppWstaArray = pwifi_wsta;


	/*Below lines are commented out and rewritten as with this logic  XHS and xfinity wifi devices are not copied with correct WIFI SSID ref and these  gets logged with wrong interface .
		This is observed when there are no devices connected over private wifi and devices are connected over other interfaces */

/*
    for(i = 0, j = 0; i < *pCount; i++)
    {
        strncpy(pwifi_wsta->phyAddr, parametervalAssociatedDeviceNum[i * field_num + pos[0]]->parameterValue, 18);
        itmp = strlen(parametervalAssociatedDeviceNum[i * field_num + pos[0]]->parameterName) - strlen(".MACAddress");
        itmp = (itmp > LM_GEN_STR_SIZE - 1) ? LM_GEN_STR_SIZE-1 : itmp;
        memcpy(pwifi_wsta->AssociatedDevice, parametervalAssociatedDeviceNum[i * field_num + pos[0]]->parameterName, itmp);
        pwifi_wsta->AssociatedDevice[itmp] = '\0';
        pwifi_wsta->RSSI = atoi(parametervalAssociatedDeviceNum[i * field_num + pos[1]]->parameterValue);
        if((AssociatedDevice_number[j]--) == 0){
            j++;
        }
        strncpy(pwifi_wsta->ssid, parametervalSSIDRef[j]->parameterValue, LM_GEN_STR_SIZE-1);
        pwifi_wsta->ssid[strlen(pwifi_wsta->ssid) - 1] = '\0';
        pwifi_wsta++;
    }
	*/
	int k = 0;
	int device_no= 0;
	int device_count= 0;
	for(k = 0; k < interface_number ; k++)
	{
		device_no = AssociatedDevice_number[k];
	    if(device_no > 0)
		{
			for(i = device_count, j = 1; i < *pCount && j <=device_no ; i++,device_count++,j++)
			{
				strncpy(pwifi_wsta->phyAddr, parametervalAssociatedDeviceNum[i * field_num + pos[0]]->parameterValue, 18);
				itmp = strlen(parametervalAssociatedDeviceNum[i * field_num + pos[0]]->parameterName) - strlen(".MACAddress");
				itmp = (itmp > LM_GEN_STR_SIZE - 1) ? LM_GEN_STR_SIZE-1 : itmp;
				memcpy(pwifi_wsta->AssociatedDevice, parametervalAssociatedDeviceNum[i * field_num + pos[0]]->parameterName, itmp);
				pwifi_wsta->AssociatedDevice[itmp] = '\0';
				pwifi_wsta->RSSI = atoi(parametervalAssociatedDeviceNum[i * field_num + pos[1]]->parameterValue);
				strncpy(pwifi_wsta->ssid, parametervalSSIDRef[k]->parameterValue, LM_GEN_STR_SIZE-1);
				pwifi_wsta->ssid[strlen(pwifi_wsta->ssid) - 1] = '\0';
				pwifi_wsta++;
			}
		}
	}

    rVal = 0;

    parameterValStruct_t **valStrchannel;
	parameterValStruct_t **valStrsecmode;
	parameterValStruct_t **valStrsecencrymode;
	parameterValStruct_t **valStrssid;
	int nval, retval;
	char str[2][80];
	char * name[2] = {(char*) str[0], (char*) str[1]};  
	int interface ;
	char secMode[128]  = {'\0'};
	char secEncMode[128] = {'\0'};
	char ssid[128] = {'\0'};
        char  *paramNames[1] ;
	int radioInt = 0;
	int currentChannel = 0;
        LM_wifi_wsta_t *hosts = *ppWstaArray;
	time_t     now;
	int activityTimeChangeDiff = 0;
	for (i = 0; i < *pCount ; i++)
        {
		PLmObjectHost pHost;
		char index;
  		index = hosts[i].ssid[strlen(hosts[i].ssid)-1];
		interface = index - '0';
		/* SSID index check. Disabling logging for xfinity wifi devices and XHS clients. As these devices are not getting saved, whenever this loop runs , everytime these devices are getting logged as New Client.
			TODO: Need to figure out the way to idenify new client. 
		1 and 2 private wifi - Log is enabled
	    3 - Home-security, 5 & 6 - hotspot index - Log is disabled.*/
		if(interface > 2) {
			//printf(" It is not private wifi Device. \n");
			continue;
		}
		/* disable logic ends */
		pHost = Hosts_FindHostByPhysAddress(hosts[i].phyAddr);
		if(pHost) {
		    if(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]) {
			     if((strstr(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId],"WiFi"))) {
				 continue;
			     } 
		    } 
		} else {
		   CcspWifiTrace(("RDK_LOG_WARN, New Wifi Client connected. Mac is %s \n",hosts[i].phyAddr));
		}
		
		if(interface%2 == 0)
			radioInt = 1;
		else
			radioInt = 0;

		snprintf(str[0], sizeof(str[0]),WIFI_DM_AUTOCHAN,radioInt+1);
		snprintf(str[1], sizeof(str[1]),WIFI_DM_CHANNEL,radioInt+1);
		ret = CcspBaseIf_getParameterValues(
		    bus_handle,
		    pWiFiComponentName,
		    pComponentPath,
		    &name,
		    2,
		    &nval,
		    &valStrchannel);

		if(ret != CCSP_Message_Bus_OK){
			CcspTraceError(("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, name, ret));
			printf("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, name, ret);
			goto RET1;
		}

		if(strncmp("true", valStrchannel[0]->parameterValue, 5)==0)
			retval = 0;

		currentChannel = atoi(valStrchannel[1]->parameterValue);
		
		snprintf(secMode, sizeof(secMode), WIFI_DM_BSS_SECURITY_MODE,interface);
		paramNames[0] = LanManager_CloneString(secMode);;
		ret = CcspBaseIf_getParameterValues(
		    bus_handle,
		    pWiFiComponentName,
		    pComponentPath,
		    paramNames,
		    1,
		    &nval,
		    &valStrsecmode);
		if(ret != CCSP_Message_Bus_OK){
			CcspTraceError(("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, secMode, ret));
			printf("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, secMode, ret);
		        goto RET1;
		} 

		snprintf(secEncMode, sizeof(secEncMode),WIFI_DM_BSS_SECURITY_ENCRYMODE,interface);
                paramNames[0] = LanManager_CloneString(secEncMode);
		ret = CcspBaseIf_getParameterValues(
		    bus_handle,
		    pWiFiComponentName,
		    pComponentPath,
		    paramNames,
		    1,
		    &nval,
		    &valStrsecencrymode);
		if(ret != CCSP_Message_Bus_OK){
			CcspTraceError(("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, secEncMode, ret));
			printf("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, secEncMode, ret);
			goto RET1;
		} 

		snprintf(ssid, sizeof(ssid),WIFI_DM_SSID,interface);
	        paramNames[0] = LanManager_CloneString(ssid);
		ret = CcspBaseIf_getParameterValues(
		    bus_handle,
		    pWiFiComponentName,
		    pComponentPath,
		    paramNames,
		    1,
		    &nval,
		    &valStrssid);
		if(ret != CCSP_Message_Bus_OK){
			CcspTraceError(("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, ssid, ret));
			printf("%s CcspBaseIf_getParameterValues %s error %d!\n", __FUNCTION__, ssid, ret);
			goto RET1;
		}
		CcspWifiTrace(("RDK_LOG_WARN, No of Wifi clients connected : %d \n",*pCount));
		CcspWifiTrace(("RDK_LOG_WARN, Device No : %d MAC - %s interface : ath%d \n",i+1,hosts[i].phyAddr,interface-1));
		if(!retval){	
		  CcspWifiTrace(("RDK_LOG_WARN, ssid : %s Auto channel enabled, channel : %d \n",valStrssid[0]->parameterValue,currentChannel));
		}else {
		 CcspWifiTrace(("RDK_LOG_WARN, ssid : %s  channel : %d \n",valStrssid[0]->parameterValue,currentChannel));
		}	
		CcspWifiTrace(("RDK_LOG_WARN, Security Mode : %s , Security Encryptionmode : %s \n",valStrsecmode[0]->parameterValue,valStrsecencrymode[0]->parameterValue));

	}


RET7:
    free_parameterValStruct_t(bus_handle, num_size, parametervalAssociatedDeviceNum);
RET6:
    free_parameterValStruct_t (bus_handle, ref_size, parametervalSSIDRef);
RET5:
    free(pAssociatedDeviceNameBuf);
RET4:
    free(pReferenceParaNameBuf);
RET3:
    free(pAssociatedDeviceNameArray);
RET2:
    free(pReferenceParaNameArray);
RET1:
    free_parameterInfoStruct_t (bus_handle, bkup_ifaceNumber, interfaceInfo);

    PRINTD("EXT %s\n", __FUNCTION__);
    return rVal;
}

void _get_shell_output(char * cmd, char * out, int len)
{
    FILE * fp;
    char   buf[256] = {0};
    char * p;
    fp = popen(cmd, "r");
    if (fp)
    {
        fgets(buf, sizeof(buf), fp);
        /*we need to remove the \n char in buf*/
        if ((p = strchr(buf, '\n'))) *p = 0;
        strncpy(out, buf, len-1);
        pclose(fp);        
    }
}

int lm_wrapper_get_arp_entries (char netName[LM_NETWORK_NAME_SIZE], int *pCount, LM_host_entry_t **ppArray)
{
    FILE *fp = NULL;
    char buf[200] = {0};
    int index = 0;
    char stub[64], status[32];;
    int ret;
    LM_host_entry_t *hosts = NULL;

    pthread_mutex_lock(&GetARPEntryMutex);

    unlink(ARP_CACHE_FILE);
   // snprintf(buf, sizeof(buf), "ip nei show | grep %s | grep -v 192.168.10 > %s", netName, ARP_CACHE_FILE);
   // This is added to remove atom mac from the connected device list.
    char            cmd[256]         = {0};
    char            out[32]         = {0};
	
	// XB6 Doesn't have this interface. Remove constant warnings.
#ifndef INTEL_PUMA7
     if(pAtomBRMac[0] == '\0' || pAtomBRMac[0] == ' ') {
		_ansc_sprintf(cmd, "ifconfig l2sd0 | grep HWaddr | awk '{print $5}' | cut -c 1-14\n" );
		_get_shell_output(cmd, out, sizeof(out));
		 strncpy(pAtomBRMac,out,sizeof(out));
		CcspTraceWarning(("Atom mac is %s \n",pAtomBRMac));
   	}
#endif
 
    if(pAtomBRMac[0] != '\0'  &&  pAtomBRMac[0] != ' ') {
    	v_secure_system("ip nei show | grep %s | grep -v 192.168.10  | grep -i -v %s > "ARP_CACHE_FILE, netName, pAtomBRMac);
    } else {
    	v_secure_system("ip nei show | grep %s | grep -v 192.168.10  > "ARP_CACHE_FILE, netName);
    }	

    if ( (fp=fopen(ARP_CACHE_FILE, "r")) == NULL )
    {
        *pCount = 0;
        return -1;
    }

    while ( fgets(buf, sizeof(buf), fp)!= NULL )
    {
        if ( strstr(buf, "FAILED") != 0 )
        {
            continue;
        }

        hosts = (LM_host_entry_t *) realloc(hosts, sizeof(LM_host_entry_t) * (index+1));

        if ( hosts == NULL )
        {
            fclose(fp);
            unlink(ARP_CACHE_FILE);
            *pCount = 0;
            return -1;
        }

        /*
        Sample:
        fe80::f07f:ef54:f9b3:69f4 dev brlan0 lladdr f0:de:f1:0b:39:65 STALE
        192.168.1.200 dev brlan0 lladdr f0:de:f1:0b:39:65 STALE
        192.168.1.206 dev brlan0 lladdr f0:de:f1:0b:39:65 REACHABLE
        192.168.100.3 dev lan0 lladdr 00:13:20:fa:72:25 STALE
        */
        ret = sscanf(buf, LM_ARP_ENTRY_FORMAT,
                 hosts[index].ipAddr,
                 stub,
                 hosts[index].ifName,
                 stub,
                 hosts[index].phyAddr,
                 status);  //todo: should IPv6 router be cared about?
        if(ret != 6)
            continue;

        if ( 0 == strcmp(status, "REACHABLE") )
        {
            hosts[index].status = LM_NEIGHBOR_STATE_REACHABLE;
        }
        else
        {
            hosts[index].status = LM_NEIGHBOR_STATE_STALE;
        }
        PRINTD("%s:%s %s %s %d\n", __FUNCTION__,
                hosts[index].phyAddr,
                hosts[index].ipAddr,
                hosts[index].ifName,
                hosts[index].status);

        index++;
    }

    (*pCount) = index;

    (*ppArray) = hosts;

    fclose(fp);

    unlink(ARP_CACHE_FILE);

    pthread_mutex_unlock(&GetARPEntryMutex);

    return 0;
}


void getAddressSource(char *physAddress, char *pAddressSource)
{

    FILE *fp = NULL;
    char buf[200] = {0};
    int ret;
    LM_host_entry_t dhcpHost;
    if ( (fp=fopen(DNSMASQ_LEASES_FILE, "r")) == NULL )
    {
        return;
    }

    while ( fgets(buf, sizeof(buf), fp)!= NULL )
    {
        /*
        Sample:
        6885 f0:de:f1:0b:39:65 10.0.0.96 shiywang-WS 01:f0:de:f1:0b:39:65 6765 MSFT 5.0
        6487 02:10:18:01:00:02 10.0.0.91 * * 6367 *
        */
        ret = sscanf(buf, LM_DHCP_CLIENT_FORMAT,
                 &(dhcpHost.LeaseTime),
                 dhcpHost.phyAddr,
                 dhcpHost.ipAddr,
                 dhcpHost.hostName
              );


        if(ret != 4)
            continue;

	if (!strcasecmp(physAddress,dhcpHost.phyAddr))
	{
		strcpy(pAddressSource,"DHCP");
		break;
	}

   }

    fclose(fp);
memset(buf,0,sizeof(buf));
   if ( (fp=fopen(DNSMASQ_RESERVED_FILE, "r")) == NULL )
    {
        return;
    }

    while ( fgets(buf, sizeof(buf), fp)!= NULL )
    {
        /*
        Sample:
        02:10:18:01:00:02,10.0.0.91,*
        */
        ret = sscanf(buf, DHCPV4_RESERVED_FORMAT,
                 dhcpHost.phyAddr,
                 dhcpHost.ipAddr,
                 dhcpHost.hostName
              );
        if(ret != 3)
            continue;

	if (!strcasecmp(physAddress,dhcpHost.phyAddr))
	{
		strcpy(pAddressSource,"Static");
		break;
	}

   }
    fclose(fp);

   return;
}

int getIPAddress(char *physAddress,char *IPAddress)
{

    FILE *fp = NULL;
    char output[50] = {0};

    v_secure_system("ip -4 nei show | grep brlan0 | grep -v 192.168.10 | grep -i %s | awk '{print $1}' | tail -1 > /tmp/LMgetIP.txt ", physAddress);
      
    fp = fopen ("/tmp/LMgetIP.txt", "r");
  
    if (fp != NULL) 
    {
        while(fgets(output, sizeof(output), fp)!=NULL);
        fclose (fp);
    }
    
    strcpy(IPAddress,output);
    return 0;
}

void lm_wrapper_get_dhcpv4_client()
{
    FILE *fp = NULL;
    char buf[200] = {0};
    char stub[64];
    int ret;
    PLmObjectHostIPAddress pIP;

    LM_host_entry_t dhcpHost;
    PLmObjectHost pHost;

    if ( (fp=fopen(DNSMASQ_LEASES_FILE, "r")) == NULL )
    {
        return;
    }

    while ( fgets(buf, sizeof(buf), fp)!= NULL )
    {
        /*
        Sample:
        6885 f0:de:f1:0b:39:65 10.0.0.96 shiywang-WS 01:f0:de:f1:0b:39:65 6765 MSFT 5.0
        6487 02:10:18:01:00:02 10.0.0.91 * * 6367 *
        */
	ret = sscanf(buf, LM_DHCP_CLIENT_FORMAT,
                 &(dhcpHost.LeaseTime),
                 dhcpHost.phyAddr,
                 dhcpHost.ipAddr,
                 dhcpHost.hostName
              );

        if(ret != 4)
            continue;
        pHost = Hosts_FindHostByPhysAddress(dhcpHost.phyAddr);

        if ( pHost )
        {
            PRINTD("%s: %s %s\n", __FUNCTION__, dhcpHost.phyAddr, dhcpHost.hostName);

            if ( pHost->pStringParaValue[LM_HOST_HostNameId] )
            {
                LanManager_Free(pHost->pStringParaValue[LM_HOST_HostNameId]);
            }
            if(dhcpHost.hostName == NULL || AnscEqualString(dhcpHost.hostName, "*", FALSE))
            {
                pHost->pStringParaValue[LM_HOST_HostNameId] = LanManager_CloneString(pHost->pStringParaValue[LM_HOST_PhysAddressId]);
            }else
                pHost->pStringParaValue[LM_HOST_HostNameId] = LanManager_CloneString(dhcpHost.hostName);

            pIP = Host_AddIPv4Address
            (
                pHost,
                dhcpHost.ipAddr
            );
            if(pIP != NULL)
            {
                if(pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressSourceId])
                    LanManager_Free(pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressSourceId]);
                pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressSourceId] = LanManager_CloneString("DHCP");
                pIP->LeaseTime = (dhcpHost.LeaseTime == 0 ? 0xFFFFFFFF: dhcpHost.LeaseTime); 
            }
        }
    }

    fclose(fp);

    return;
}

void lm_wrapper_get_dhcpv4_reserved()
{

    FILE *fp = NULL;
    char buf[200] = {0};
    char stub[64];
    int ret;

    PLmObjectHostIPAddress pIP;
    LM_host_entry_t dhcpHost;
    PLmObjectHost pHost;

    if ( (fp=fopen(DNSMASQ_RESERVED_FILE, "r")) == NULL )
    {
        return;
    }

    while ( fgets(buf, sizeof(buf), fp)!= NULL )
    {
        /*
        Sample:
        02:10:18:01:00:02,10.0.0.91,*
        */
        ret = sscanf(buf, DHCPV4_RESERVED_FORMAT,
                 dhcpHost.phyAddr,
                 dhcpHost.ipAddr,
                 dhcpHost.hostName
              );
        if(ret != 3)
            continue;

        pHost = Hosts_FindHostByPhysAddress(dhcpHost.phyAddr);

        if ( !pHost )
        {
            pHost = Hosts_AddHostByPhysAddress(dhcpHost.phyAddr);

            if ( pHost )
            {
                if ( pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] )
                {
                    LanManager_Free(pHost->pStringParaValue[LM_HOST_Layer1InterfaceId]);
                    pHost->pStringParaValue[LM_HOST_Layer1InterfaceId] = NULL;
                }


            }
        }

        if ( pHost )
        {
            PRINTD("%s: %s %s %s\n", __FUNCTION__, dhcpHost.phyAddr, dhcpHost.ipAddr, dhcpHost.hostName);
//RDKB-EMULATOR
      /*      if ( pHost->pStringParaValue[LM_HOST_HostNameId] )
            {
                LanManager_Free(pHost->pStringParaValue[LM_HOST_HostNameId]);
            }
            if(dhcpHost.hostName == NULL || AnscEqualString(dhcpHost.hostName, "*", FALSE))
            {
                pHost->pStringParaValue[LM_HOST_HostNameId] = LanManager_CloneString(pHost->pStringParaValue[LM_HOST_PhysAddressId]);
            }else
                pHost->pStringParaValue[LM_HOST_HostNameId] = LanManager_CloneString(dhcpHost.hostName);*/

            pIP = Host_AddIPv4Address
                (
                    pHost,
                    dhcpHost.ipAddr
                );
            if(pIP != NULL)
            {
                if(pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressSourceId])
                    LanManager_Free(pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressSourceId]);
                pIP->pStringParaValue[LM_HOST_IPAddress_IPAddressSourceId] = LanManager_CloneString("Static");
                pIP->LeaseTime = 0;
            }

        }
    }

    fclose(fp);

    return;
}

void Xlm_wrapper_get_info(PLmObjectHost pHost)
{
    FILE *fp = NULL;
    char buf[200] = {0};
    char stub[64];
    int ret;
    LM_host_entry_t dhcpHost;

    if ( (fp=fopen(DNSMASQ_LEASES_FILE, "r")) == NULL )
    {
        return;
    }

    while ( fgets(buf, sizeof(buf), fp)!= NULL )
    {
        /*
        Sample:sss
        6885 f0:de:f1:0b:39:65 10.0.0.96 shiywang-WS 01:f0:de:f1:0b:39:65 6765 MSFT 5.0
        6487 02:10:18:01:00:02 10.0.0.91 * * 6367 *
        */
        ret = sscanf(buf, LM_DHCP_CLIENT_FORMAT,
                 &(dhcpHost.LeaseTime),
                 dhcpHost.phyAddr,
                 dhcpHost.ipAddr,
                 dhcpHost.hostName
              );
        if(ret != 4)
            continue;

                if(strstr(dhcpHost.ipAddr,"172.16.12.") && AnscEqualString(dhcpHost.phyAddr,pHost->pStringParaValue[LM_HOST_PhysAddressId],FALSE))
                {
                        pthread_mutex_lock(&XLmHostObjectMutex);

                        if(dhcpHost.hostName == NULL || AnscEqualString(dhcpHost.hostName, "*", FALSE))
                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_HostNameId]), pHost->pStringParaValue[LM_HOST_PhysAddressId]);
                        else
                        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_HostNameId]), dhcpHost.hostName);
                        Host_AddIPv4Address ( pHost, dhcpHost.ipAddr);
                        pHost->LeaseTime  = (dhcpHost.LeaseTime == 0 ? 0xFFFFFFFF: dhcpHost.LeaseTime);

                        pthread_mutex_unlock(&XLmHostObjectMutex);
                        break;
                }
    }

    fclose(fp);

    return;
}

