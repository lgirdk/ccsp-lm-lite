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

    module: cosa_apis_hosts.h

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

    -------------------------------------------------------------------

    description:

        This file defines the apis for objects to support Data Model Library.

    -------------------------------------------------------------------


    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        09/16/2011    initial revision.

**************************************************************************/


#ifndef  _LM_MAIN_H_
#define  _LM_MAIN_H_

#include "lm_api.h"

#define LM_HOST_POLLINGINTERVAL                 30 

#define LM_HOST_ActiveId                        0
#define LM_HOST_NumBoolPara                     1

#define LM_HOST_X_CISCO_COM_ActiveTimeId        0
#define LM_HOST_X_CISCO_COM_InactiveTimeId      1
#define LM_HOST_X_CISCO_COM_RSSIId              2
#define LM_HOST_NumIntPara                      3

#define LM_HOST_X_CISCO_COM_DeviceTypeId        0
#define LM_HOST_X_CISCO_COM_NetworkInterfaceId  1
#define LM_HOST_X_CISCO_COM_ConnectionStatusId  2
#define LM_HOST_X_CISCO_COM_OSType              3
#define LM_HOST_NumUlongPara                    4

//Unknown(1),Airport Base Station(2),AirTunes(3),AppleTV(4),Apple Device(5),Workstation(6),Network Storage(7),Set-Top Box(8),Router(9),Media Player(10), Printer(11)
#define LM_HOST_X_CISCO_COM_DeviceType_Unkown               1
#define LM_HOST_X_CISCO_COM_DeviceType_AirportBaseStation   2
#define LM_HOST_X_CISCO_COM_DeviceType_AirTunes             3
#define LM_HOST_X_CISCO_COM_DeviceType_AppleTV              4
#define LM_HOST_X_CISCO_COM_DeviceType_AppleDevice          5
#define LM_HOST_X_CISCO_COM_DeviceType_Workstation          6
#define LM_HOST_X_CISCO_COM_DeviceType_NetworkStorage       7
#define LM_HOST_X_CISCO_COM_DeviceType_SetTopBox            8
#define LM_HOST_X_CISCO_COM_DeviceType_Router               9
#define LM_HOST_X_CISCO_COM_DeviceType_MediaPlayer          10
#define LM_HOST_X_CISCO_COM_DeviceType_Printer              11
#define LM_HOST_X_CISCO_COM_DeviceType_TV                   12
#define LM_HOST_X_CISCO_COM_DeviceType_Scanner              13
#define LM_HOST_X_CISCO_COM_DeviceType_MediaServer          14
#define LM_HOST_X_CISCO_COM_DeviceType_MediaRenderer        15
#define LM_HOST_X_CISCO_COM_DeviceType_AccessPoint          16
#define LM_HOST_X_CISCO_COM_DeviceType_DigitalSecurityCamera 17
#define LM_HOST_X_CISCO_COM_DeviceType_CDPlayer             18

#define LM_HOST_X_CISCO_COM_OSType_Unknown                  0
#define LM_HOST_X_CISCO_COM_OSType_WindowsXP                1
#define LM_HOST_X_CISCO_COM_OSType_Windows7                 2
#define LM_HOST_X_CISCO_COM_OSType_MACOSX                   3
#define LM_HOST_X_CISCO_COM_OSType_Linux                    4
#define LM_HOST_X_CISCO_COM_OSType_iOS                      5
#define LM_HOST_X_CISCO_COM_OSType_Android                  6

typedef  struct
_LmObjectHostPossibleDeviceTypeKeyWords
{
    char  *keyWords[50];
    int deviceTypeId[50];
    int numKeyWords;
}
LmObjectHostPossibleDeviceTypeKeyWords,  *PLmObjectHostPossibleDeviceTypeKeyWords;

#define LM_HOST_X_CISCO_COM_NetworkInterface_Unkown     0
#define LM_HOST_X_CISCO_COM_NetworkInterface_WiFi       1
#define LM_HOST_X_CISCO_COM_NetworkInterface_Ethernet   2
#define LM_HOST_X_CISCO_COM_NetworkInterface_MoCA       3

#define LM_HOST_CONNECTION_STATUS_Good                  1
#define LM_HOST_CONNECTION_STATUS_Poor                  2
#define LM_HOST_CONNECTION_STATUS_Bad                   3
#define LM_HOST_CONNECTION_STATUS_Indeterminable        4

#define LM_HOST_AliasId                                 0
#define LM_HOST_PhysAddressId                           1
#define LM_HOST_IPAddressId                             2
#define LM_HOST_DHCPClientId                            3
#define LM_HOST_AssociatedDeviceId                      4
#define LM_HOST_Layer1InterfaceId                       5
#define LM_HOST_Layer3InterfaceId                       6
#define LM_HOST_HostNameId                              7
#define LM_HOST_X_CISCO_COM_UPnPDeviceId                8
#define LM_HOST_X_CISCO_COM_HNAPDeviceId                9
#define LM_HOST_X_CISCO_COM_DNSRecordsId                10
#define LM_HOST_X_CISCO_COM_HardwareVendorId            11
#define LM_HOST_X_CISCO_COM_SoftwareVendorId            12
#define LM_HOST_X_CISCO_COM_SerialNumbreId              13
#define LM_HOST_X_CISCO_COM_UserDefinedDeviceTypeId     14
#define LM_HOST_X_CISCO_COM_UserDefinedHardwareVendorId 15
#define LM_HOST_X_CISCO_COM_UserDefinedSoftwareVendorId 16
#define LM_HOST_AddressSource                           17
#define LM_HOST_Comments                                18
#define LM_HOST_X_RDKCENTRAL_COM_Parent                 19
#define LM_HOST_X_RDKCENTRAL_COM_DeviceType             20
#define LM_HOST_NumStringPara                           22

#define LM_HOST_IPAddress_IPAddressId     0
#define LM_HOST_IPAddress_IPAddressSourceId     1
#define LM_HOST_IPAddress_NumStringPara   2

#define LM_HOST_ARRAY_STEP                  20

#define CISCO_SYSTEMS_ENTERPRISE_NUMBER     9
#define LM_SubOPT_Manaufacturer_OUI         1
#define LM_SubOPT_Serial_Number             2
#define LM_SubOPT_Product_Class             3

#define  COSA_HOSTS_Extension1_Name                 "X_COMCAST-COM_LastChange"
#define IP_V6 6
#define IP_V4 4

#define LM_HOST_IPv4Address_IPAddressId     0
#define LM_HOST_IPv4Address_NumStringPara   1

#define LM_HOST_IPv6Address_IPAddressId     0
#define LM_HOST_IPv6Address_NumStringPara   1

#define LM_ADDRESS_SOURCE_DHCP_STR          "DHCP"
#define LM_ADDRESS_SOURCE_STATIC_STR        "Static"
#define LM_ADDRESS_SOURCE_RESERVED_STR      "ReservedIP"
#define LM_ADDRESS_SOURCE_AUTOIP_STR        "AUTOIP"

#define TIME_NO_NEGATIVE(x) ((long)(x) < 0 ? 0 : (x))
typedef  struct
_LmHostInfo
{
    char    Chaddr[32];
    char    IPv4Address[32];
    char    HostName[LM_GEN_STR_SIZE];
    char    Active[16];
    char    Interface[LM_GEN_STR_SIZE];
}LmHostInfo,  *PLmHostInfo;

typedef  struct
_LmObjectHostIPAddress
{
    int     active;
    int     l3unReachableCnt; 
    char    *pStringParaValue[LM_HOST_IPAddress_NumStringPara];
    int     LeaseTime;
    int     instanceNum;  /* instance number */
    struct _LmObjectHostIPAddress *pNext;
}
LmObjectHostIPAddress,  *PLmObjectHostIPAddress;

typedef  struct
_LmObjectHost
{
    int     id;  /* index */
    int     instanceNum;  /* instance number */
    BOOL    bBoolParaValue[LM_HOST_NumBoolPara];
    int     iIntParaValue[LM_HOST_NumIntPara];
    ULONG   ulUlongParaValue[LM_HOST_NumUlongPara];
    char    *pStringParaValue[LM_HOST_NumStringPara];

    char    *objectName;
    ULONG   l3unReachableCnt;
    ULONG   l1unReachableCnt;

    /* Host is active when any of the following is TRUE. 
     * So have to store the status here to derive the host active parameter. */
    BOOL    ipv4Active;
    BOOL    ipv6Active;
    /* Activity change time: the moment Active is changed, 
     * either from inactive to active, or from active to inactive. 
     * It is shown current time in seconds. */
    time_t   activityChangeTime;

    ULONG   LeaseTime;
    PLmObjectHostIPAddress ipv4AddrArray;
    int numIPv4Addr;

    PLmObjectHostIPAddress ipv6AddrArray;
    int numIPv6Addr;

    BOOL    bTrueStaticIPClient;
    char        *Layer3Interface;
    char backupHostname[64];
    BOOL bClientReady;


}
LmObjectHost,  *PLmObjectHost;

enum DeviceType
{
    UPnPRootDevice,
    UPnPDevice,
    UPnPService,
    HNAPDevice,
    MDNSDevice
};

typedef  struct
_LmObjectHosts
{
    char *pHostBoolParaName[LM_HOST_NumBoolPara]; 
    char *pHostIntParaName[LM_HOST_NumIntPara];
    char *pHostUlongParaName[LM_HOST_NumUlongPara]; 
    char *pHostStringParaName[LM_HOST_NumStringPara]; 
    char *pIPv4AddressStringParaName[LM_HOST_IPAddress_NumStringPara]; 
    char *pIPv6AddressStringParaName[LM_HOST_IPAddress_NumStringPara]; 
    int   availableInstanceNum;

    PLmObjectHost *hostArray;
    int sizeHost;
    int numHost;
    ULONG lastActivity;
}
LmObjectHosts,  *PLmObjectHosts;

typedef  enum
_COSA_DML_HOST_ADDR_SOURCE
{
    COSA_DML_HOST_ADDR_SOURCE_DHCP     = 1,
    COSA_DML_HOST_ADDR_SOURCE_Static,
    COSA_DML_HOST_ADDR_SOURCE_AutoIP,
    COSA_DML_HOST_ADDR_SOURCE_None
}
COSA_DML_HOST_ADDR_SOURCE, *PCOSA_DML_HOST_ADDR_SOURCE;

typedef  struct
_COSA_DML_HOST_ENTRY
{
    UCHAR                           PhysAddress[32];        /* Ususally the MAC address */
    ANSC_IPV4_ADDRESS               IPAddress;
    COSA_DML_HOST_ADDR_SOURCE       AddressSource;
    int                             LeaseTimeRemaining;
    char                            Layer1Interface[64];    /* Name of layer1 interface */
    char                            Layer3Interface[64];    /* Name of IP interface     */
    char                            HostName[64];
    BOOLEAN                         Active;

    USHORT                          VendorClassIDSize;
    USHORT                          ClientIDOffset;
    USHORT                          ClientIDSize;
    USHORT                          UserClassIDOffset;
    USHORT                          UserClassIDSize;
    /*
     *  VendorClassID is at offset 0; ClientID and UserClassID is
     *  determined by ClientIDOffset, ClientIDSize, UserClassIDOffset,
     *  UserClassIDSize
     */
    UCHAR                           Data[ANSC_ZERO_ARRAY_SIZE];
}
COSA_DML_HOST_ENTRY,  *PCOSA_DML_HOST_ENTRY;

/***********************************************************************

 APIs for Object:

    Hosts.

    *  Hosts_FindHostByIPv4Address
    *  Hosts_Init
    *  Hosts_SavePsmValueRecord

***********************************************************************/
void
Hosts_SynchronizeHost();

void Hosts_SyncDhcp();

#define Host_AddIPv6Address(x, y) Host_AddIPAddress(x,y,6)
#define Host_AddIPv4Address(x, y) Host_AddIPAddress(x,y,4)
PLmObjectHostIPAddress
Host_AddIPAddress
    (
        PLmObjectHost pHost,
        char * ipAddress,
        int version
    );


void
Hosts_PollHost();

void 
Hosts_FreeHost(PLmObjectHost pHost);

PLmObjectHost 
Hosts_AddHost(int instanceNum);


void Hosts_RmHosts();

#endif
