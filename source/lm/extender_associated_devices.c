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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include "extender_associated_devices.h"
#include "extender_associated_devices_avropack.h"
#include "ccsp_lmliteLog_wrapper.h"
#include "lm_wrapper.h"   
#include "lm_main.h"
#include "report_common.h"

#ifdef MLT_ENABLED
#include "rpl_malloc.h"
#include "mlt_malloc.h"
#endif

#define PUBLIC  0
#define PRIVATE 1

#define PUBLIC_WIFI_IDX_STARTS  4
#define PUBLIC_WIFI_IDX_ENDS  5
#define PRIVATE_WIFI_IDX_STARTS  0
#define PRIVATE_WIFI_IDX_ENDS  1

static sem_t mutex;
static int ThreadStarted;
static struct timespec ts;
extern LmObjectHosts lmHosts;

extern ExtenderList *extenderlist;
extern pthread_mutex_t LmHostObjectMutex;

ULONG AssociatedDevicePeriods[] = {1,5,10,15,30,60,300,900,1800,3600,10800,21600,43200,86400};

ULONG IDWPollingPeriodDefault = DEFAULT_POLLING_INTERVAL;
ULONG IDWReportingPeriodDefault = DEFAULT_REPORTING_INTERVAL;

ULONG IDWPollingPeriod = DEFAULT_POLLING_INTERVAL;
ULONG IDWReportingPeriod = DEFAULT_REPORTING_INTERVAL;

ULONG MinimumIDWPollingPeriod = 5;
ULONG currentIDWPollingPeriod = 0;
ULONG currentIDWReportingPeriod = 0;
BOOL IDWLMLiteStatus = FALSE;

ULONG IDWOverrideTTL = TTL_INTERVAL;
ULONG IDWOverrideTTLDefault = DEFAULT_TTL_INTERVAL;

bool isvalueinarray_idw(ULONG val, ULONG *arr, int size);

void* StartAssociatedDeviceHarvesting( void *arg );
void add_to_list_idw(struct associateddevicedata **headnode, char* ssid, ULONG devices, wifi_associated_dev_t* devicedata, char* freqband, ULONG channel, char* intfcmacid, char* extenderMac);
void print_list_idw( struct associateddevicedata *head );
void delete_list_idw( struct associateddevicedata *head );
int GetWiFiApGetAssocDevicesData(int ServiceType);

static struct associateddevicedata *headnodeprivate = NULL;
static struct associateddevicedata *headnodepublic = NULL;

extern int getTimeOffsetFromUtc();

void WaitForSamaphoreTimeoutIDW()
{
    if ( ThreadStarted == TRUE )
    {
        // First time thread starts, do not need to wait, report to Webpa now
        clock_gettime(CLOCK_REALTIME, &ts);  // get current to clear any wait 
        ThreadStarted = FALSE;
        return; // no need to wait
    }
    if (( GetCurrentTimeInSecond() == (int)ts.tv_sec ))
    {
        return; // no need to wait
    }
    sem_timedwait(&mutex, &ts); // Wait for trigger
}


bool isvalueinarray_idw(ULONG val, ULONG *arr, int size)
{
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return true;
    }
    return false;
}


int SetIDWHarvestingStatus(BOOL status)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Old[%d] New[%d] \n", __FUNCTION__, IDWLMLiteStatus, status ));

    if (IDWLMLiteStatus != status)
        IDWLMLiteStatus = status;
    else
        return 0;

    if (IDWLMLiteStatus)
    {
        pthread_t tid;

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Starting Thread to start DeviceData Harvesting  \n", __FUNCTION__ ));

        if (pthread_create(&tid, NULL, StartAssociatedDeviceHarvesting, NULL))
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Failed to Start Thread to start DeviceData Harvesting  \n", __FUNCTION__ ));
            return ANSC_STATUS_FAILURE;
        }
    }
    
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

BOOL GetIDWHarvestingStatus()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, IDWLMLiteStatus ));
    return IDWLMLiteStatus;
}

int SetIDWReportingPeriod(ULONG interval)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT Old[%ld] New[%ld] \n", __FUNCTION__, IDWReportingPeriod, interval ));
    if (IDWReportingPeriod != interval)
    {
        IDWReportingPeriod = interval;
    }
    else
    {
        return 0;
    }

    return 0;
}

ULONG GetIDWReportingPeriod()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%ld] \n", __FUNCTION__, IDWReportingPeriod ));
    return IDWReportingPeriod;
}

int SetIDWPollingPeriod(ULONG interval)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT Old[%ld] New[%ld] \n", __FUNCTION__, IDWPollingPeriod, interval ));
    if (IDWPollingPeriod != interval)
    {
       IDWPollingPeriod = interval;
       if(IDWPollingPeriod > 5)
            MinimumIDWPollingPeriod = 5;
       else
            MinimumIDWPollingPeriod = IDWPollingPeriod;
    }
    else
    {
        return 0;
    }

    return 0;
}

BOOL ValidateIDWPeriod(ULONG interval)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    BOOL ret = FALSE;
    ret = isvalueinarray_idw(interval, AssociatedDevicePeriods, ARRAY_SZ(AssociatedDevicePeriods));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__ , ret ));
    return ret;
} 

ULONG GetIDWPollingPeriod()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%ld] \n", __FUNCTION__, IDWPollingPeriod ));
    return IDWPollingPeriod;
}

int SetIDWReportingPeriodDefault(ULONG interval)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT Old[%ld] New[%ld] \n", __FUNCTION__, IDWReportingPeriodDefault, interval ));
    if (IDWReportingPeriodDefault != interval)
    {
        IDWReportingPeriodDefault = interval;
    }
    else
    {
        return 0;
    }

    return 0;
}

ULONG GetIDWReportingPeriodDefault()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%ld] \n", __FUNCTION__, IDWReportingPeriodDefault ));
    return IDWReportingPeriodDefault;
}

int SetIDWPollingPeriodDefault(ULONG interval)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT Old[%ld] New[%ld] \n", __FUNCTION__, IDWPollingPeriodDefault, interval ));
    if (IDWPollingPeriodDefault != interval)
    {
       IDWPollingPeriodDefault = interval;
    }
    else
    {
        return 0;
    }

    return 0;
}

ULONG GetIDWPollingPeriodDefault()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%ld] \n", __FUNCTION__, IDWPollingPeriodDefault ));
    return IDWPollingPeriodDefault;
}

ULONG GetIDWOverrideTTL()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%ld] \n", __FUNCTION__, IDWOverrideTTL ));
    return IDWOverrideTTL;
}

int SetIDWOverrideTTL(ULONG count)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT Old[%ld] New[%ld] \n", __FUNCTION__, IDWOverrideTTL, count ));
    IDWOverrideTTL = count;
    return 0;
}

ULONG GetIDWOverrideTTLDefault()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%ld] \n", __FUNCTION__, IDWOverrideTTLDefault ));
    return IDWOverrideTTLDefault;
}


void add_to_list_idw(struct associateddevicedata **headnode, char* ssid, ULONG devices, wifi_associated_dev_t* devicedata, char* freqband, ULONG channel, char* intfcmacid, char* extenderMac)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, SSID Input[%s] Devices[%ld] \n", ssid, devices));
    struct associateddevicedata *ptr = malloc(sizeof(*ptr));
    memset( ptr, 0, sizeof(*ptr));
    if (ptr == NULL)
    {
        CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s :  Linked List Allocation Failed \n", __FUNCTION__ ));
        return;
    }
    else
    {
        ptr->sSidName = strdup(ssid);
        ptr->bssid = strdup(intfcmacid);
        ptr->numAssocDevices = devices;
        ptr->devicedata = devicedata;
        ptr->radioOperatingFrequencyBand = strdup(freqband); //Possible value 2.4Ghz and 5.0 Ghz
        ptr->radioChannel = channel;
        ptr->parent = strdup(extenderMac);
        ptr->next = NULL;
        gettimeofday(&(ptr->timestamp), NULL);
        ptr->timestamp.tv_sec -= getTimeOffsetFromUtc();

        if (*headnode == NULL)
        {
            *headnode = ptr; //Important - headnode only assigned when it is a NEW list
        }
        else
        {
            struct associateddevicedata *prevnode, *currnode;

            // transverse to end of list and append new list
            prevnode = *headnode;
            currnode = (*headnode)->next;
            while ( currnode != NULL)
            {
                prevnode = currnode;
                currnode = currnode->next;
            };
            prevnode->next = ptr; // add to list
        }
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return;
}

void print_list_idw( struct associateddevicedata *headnode)
{
    int z = 0;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    struct associateddevicedata  *ptr = headnode;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Head Ptr [%lx]\n", (ulong)headnode));
    while (ptr != NULL)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Head Ptr [%lx] TimeStamp[%d] for Node[%d] with SSID[%s] \n", __FUNCTION__ ,(ulong)ptr, (int)ptr->timestamp.tv_sec, z, ptr->sSidName));
        ptr = ptr->next;
        z++;
    }
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));
    return;
}

/* Function to delete the entire linked list */
void delete_list_idw(  struct associateddevicedata *headnode )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    struct associateddevicedata *currnode = headnode;
    struct associateddevicedata* next = NULL;

    while (currnode != NULL)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Deleting IDW Node Head Ptr [%lx] with SSID[%s] \n",__FUNCTION__, (ulong)currnode, currnode->sSidName));
        next = currnode->next;
        free(currnode->sSidName);
        free(currnode->bssid);
        free(currnode->radioOperatingFrequencyBand);
        free(currnode->devicedata);
        free(currnode->parent);
        free(currnode);
        currnode = next;
    }
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));

    return;
}

int GetWiFiApGetAssocDevicesData(int ServiceType)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    BOOL enabled = FALSE;
    int ret = 0;
    UINT array_size = 1;
    char interfaceMAC[128] = {0};
    ULONG channel = 0;
    char freqband[128] = {0};
    ExtenderList *list = extenderlist;

    while(list)
    {
        int i = 0;

        if(list->info->list)
        {
        printf("Extender ClientInfoList NumClients [%d] \n", list->info->list->numClient);

        ClientInfo* tmp = list->info->list->connectedDeviceList;
        char* parentextender = FindMACByIPAddress(list->info->extender_ip);

        for(i = 0; i < list->info->list->numClient; i++, tmp = tmp->next)
            {
            wifi_associated_dev_t *wifi_associated_dev_array = NULL;
            struct associateddevicedata **headnode = NULL;
            int k = 0;
            char CpeMacHoldingBuf[ 20 ] = {0};

            printf("Extender Device %x\n", tmp);
            printf("Extender MACAddress [%s] \n", tmp->MAC_Address);
            printf("Extender SSID_Type [%s] \n", tmp->SSID_Type);
            printf("Extender Device_Name [%s] \n", tmp->Device_Name);
            printf("Extender SSID_Name [%s] \n", tmp->SSID_Name);
            printf("Extender RSSI [%s] \n", tmp->RSSI);

            wifi_associated_dev_array = malloc (sizeof(*wifi_associated_dev_array));


            for (k = 0; k < 6; k++ )
            {
            /* copy 2 bytes */
            CpeMacHoldingBuf[ k * 2 ] = tmp->MAC_Address[ k * 3 ];
            CpeMacHoldingBuf[ k * 2 + 1 ] = tmp->MAC_Address[ k * 3 + 1 ];
            wifi_associated_dev_array->cli_MACAddress[ k ] = (unsigned char)strtol(&CpeMacHoldingBuf[ k * 2 ], NULL, 16);
            }

            memset(wifi_associated_dev_array->cli_IPAddress, 0, sizeof wifi_associated_dev_array->cli_IPAddress);

            wifi_associated_dev_array->cli_AuthenticationState = 0;
            wifi_associated_dev_array->cli_LastDataDownlinkRate= 0;
            wifi_associated_dev_array->cli_LastDataUplinkRate= 0;
            wifi_associated_dev_array->cli_SignalStrength= atoi(tmp->RSSI) + 100;
            wifi_associated_dev_array->cli_Retransmissions= 0;
            wifi_associated_dev_array->cli_Active= TRUE;

            if((strstr(tmp->SSID_Type,"2.4"))) 
                {
                    strcpy(freqband, "_2_4GHz");
                }
            else
                {
                    strcpy(freqband, "_5GHz");
                }

            memset(wifi_associated_dev_array->cli_OperatingStandard, 0, sizeof wifi_associated_dev_array->cli_OperatingStandard);
            memset(wifi_associated_dev_array->cli_OperatingChannelBandwidth, 0, sizeof wifi_associated_dev_array->cli_OperatingChannelBandwidth);
            wifi_associated_dev_array->cli_SNR= 0; 
            memset(wifi_associated_dev_array->cli_InterferenceSources, 0, sizeof wifi_associated_dev_array->cli_InterferenceSources);
            wifi_associated_dev_array->cli_DataFramesSentAck= 0;
            wifi_associated_dev_array->cli_DataFramesSentNoAck= 0;
            wifi_associated_dev_array->cli_BytesSent= 0;
            wifi_associated_dev_array->cli_BytesReceived= 0;
            wifi_associated_dev_array->cli_RSSI = atoi(tmp->RSSI);
            wifi_associated_dev_array->cli_MinRSSI= 0;
            wifi_associated_dev_array->cli_MaxRSSI= 0;
            wifi_associated_dev_array->cli_Disassociations= 0;
            wifi_associated_dev_array->cli_AuthenticationFailures= 0;


            if ( ServiceType == PUBLIC )
            {
                headnode = (struct associateddevicedata **)headnodepublic;
                add_to_list_idw((struct associateddevicedata **)&headnode,  tmp->SSID_Name, array_size, wifi_associated_dev_array, (char*)&freqband, channel, (char*)&interfaceMAC, parentextender);
                headnodepublic = (struct associateddevicedata *)headnode; //Important - headnode only change when it is a NEW list
            }
            else
            {
                headnode = (struct associateddevicedata **)headnodeprivate;
                add_to_list_idw((struct associateddevicedata **)&headnode, tmp->SSID_Name, array_size, wifi_associated_dev_array, (char*)&freqband, channel, (char*)&interfaceMAC, parentextender);
                headnodeprivate = (struct associateddevicedata *)headnode; //Important - headnode only change when it is a NEW list
            }

            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,************Device Data Ends************* \n"));
            if ( ServiceType == PUBLIC )
                print_list_idw( headnodepublic );
            else
                print_list_idw( headnodeprivate );

            }

        }
        
        list = list->next;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));
    return ret;
}

void* StartAssociatedDeviceHarvesting( void *arg )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER \n", __FUNCTION__ ));
    CcspLMLiteEventTrace(("RDK_LOG_DEBUG, LMLite %s : Started Thread to start DeviceData Harvesting  \n", __FUNCTION__ ));

    int ret = 0;

    ThreadStarted = TRUE;
    sem_init(&mutex, 0, 0);      /* initialize mutex - binary semaphore */
    clock_gettime(CLOCK_REALTIME, &ts);    // get current time
    ts.tv_sec += MinimumIDWPollingPeriod; // next trigger time
    currentIDWReportingPeriod = GetIDWReportingPeriod();

    if(GetIDWOverrideTTL() < currentIDWReportingPeriod)
    {
        SetIDWOverrideTTL(currentIDWReportingPeriod);
    }

    while (!ret && GetIDWHarvestingStatus()) 
    {
        // scan extender WiFi devices
        ret = GetWiFiApGetAssocDevicesData(PRIVATE);
        if (ret)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : GetWiFiApGetAssocDevicesData returned error [%d] \n",__FUNCTION__, ret));
        }

        currentIDWReportingPeriod = currentIDWReportingPeriod + currentIDWPollingPeriod;
        currentIDWPollingPeriod = 0;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Sending to WebPA and AVRO currentIDWReportingPeriod [%ld] GetIDWReportingPeriod()[%ld]  \n", currentIDWReportingPeriod, GetIDWReportingPeriod()));

        if (currentIDWReportingPeriod >= GetIDWReportingPeriod())
        {
            if( headnodeprivate )
                {

                    int i = 0;
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Sending to WebPA and AVRO currentIDWPollingPeriod [%ld] IDWReportingPeriod[%ld]  \n", currentIDWPollingPeriod, GetIDWReportingPeriod()));
                    pthread_mutex_lock(&LmHostObjectMutex);
                    int array_size = lmHosts.numHost;
                    if (array_size)
                    {
                        for(i = 0; i < array_size; i++)
                        {
                            if((lmHosts.hostArray[i]->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType] != NULL) && !strcmp(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType], "extender"))
                                {
                                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Calling extender_report_associateddevices with Extender [%s] \n", lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId] ));
                                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Sending to WebPA and AVRO currentIDWPollingPeriod [%ld] IDWReportingPeriod[%ld]  \n", currentIDWPollingPeriod, GetIDWReportingPeriod()));
                                    extender_report_associateddevices(headnodeprivate, "PRIVATE", lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
                                }
                        }
                    }

                    pthread_mutex_unlock(&LmHostObjectMutex);
                    delete_list_idw( headnodeprivate );
                    headnodeprivate = NULL;
                }

            currentIDWReportingPeriod = 0;
        }

        if(!GetIDWOverrideTTL())
        {
            SetIDWPollingPeriod(GetIDWPollingPeriodDefault());
            SetIDWReportingPeriod(GetIDWReportingPeriodDefault());
            SetIDWOverrideTTL(GetIDWOverrideTTLDefault());
        }

        if(GetIDWOverrideTTL())
        {
            SetIDWOverrideTTL(GetIDWOverrideTTL() - GetIDWPollingPeriod());
        }

        do
        {
            currentIDWPollingPeriod += MinimumIDWPollingPeriod;
            WaitForSamaphoreTimeoutIDW();
            clock_gettime(CLOCK_REALTIME, &ts);    // Triggered, get current time
            ts.tv_sec += MinimumIDWPollingPeriod; // setup next trigger time
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Sleeping Inside IDW Loop for currentIDWPollingPeriod[%ld] MinimumIDWPollingPeriod[%ld] \n", currentIDWPollingPeriod, MinimumIDWPollingPeriod));
        }
        while (currentIDWPollingPeriod < GetIDWPollingPeriod() && GetIDWHarvestingStatus());

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, currentIDWPollingPeriod [%ld] GetIDWPollingPeriod[%ld]\n", currentIDWPollingPeriod, GetIDWPollingPeriod()));
    }
    
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));
    CcspLMLiteEventTrace(("RDK_LOG_DEBUG, LMLite %s : Thread Stopped DeviceData Harvesting  \n", __FUNCTION__ ));

    return NULL;
}

// End of File

