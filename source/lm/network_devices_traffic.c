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
#include "network_devices_traffic.h"
#include "ccsp_lmliteLog_wrapper.h"
#include "network_devices_traffic_avropack.h"
#include "lm_main.h"

static sem_t mutex;
static int ThreadStarted;
static struct timespec ts;


extern int tm_offset;

ULONG NetworkDeviceTrafficPeriods[10] = {30,60,300,900,1800,3600,10800,21600,43200,86400};

ULONG NDTPollingPeriodDefault = 300;
ULONG NDTReportingPeriodDefault = 300;

ULONG NDTPollingPeriod = 300;
ULONG NDTReportingPeriod = 300;

ULONG MinimumNDTPollingPeriod = 5;
ULONG currentNDTPollingPeriod = 0;
ULONG currentNDTReportingPeriod = 0;
BOOL NDTReportStatus = FALSE;

ULONG NDTOverrideTTL = 300;
ULONG NDTOverrideTTLDefault = 300;

struct timeval reset_timestamp;


bool isvalueinarray_ndt(ULONG val, ULONG *arr, int size);

void* StartNetworkDevicesTrafficHarvesting( void *arg );
int _syscmd_ndt(char *cmd, char *retBuf, int retBufSize);
void add_to_list_ndt(char* ip_table_line);
void print_list_ndt();
void delete_list_ndt();

extern int getTimeOffsetFromUtc();

static struct networkdevicetrafficdata *headnode = NULL;
static struct networkdevicetrafficdata *currnode = NULL;

void WaitForSamaphoreTimeoutNDT()
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

struct networkdevicetrafficdata* FindMacInLinkedList(char* device_mac)
{
    struct networkdevicetrafficdata* ptr  = headnode;
    struct networkdevicetrafficdata* retptr  = NULL;

    while (ptr != NULL)
    {
        if(!strcmp(ptr->device_mac, device_mac))
        {
            retptr = ptr;
            break;
        }

        ptr = ptr->next;
    }

    return retptr;
}

bool isvalueinarray_ndt(ULONG val, ULONG *arr, int size)
{
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return true;
    }
    return false;
}

int ResetEBTables()
{
    char rxtxcmd[128] = {0};
    char rxtxarr[128] = {0};
    int ret  = 0;
    sprintf(rxtxcmd, "/usr/ccsp/tad/rxtx_sta.sh");
    ret = _syscmd_ndt(rxtxcmd, rxtxarr, sizeof(rxtxarr));
    if(ret)
    {
        CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Executing Syscmd for RXTX STA shell script [%d] [%s] \n",__FUNCTION__, ret, rxtxarr));
        return -1;
    }
    else
    {
        gettimeofday(&(reset_timestamp), NULL);
        reset_timestamp.tv_sec -= tm_offset;
        CcspLMLiteTrace(("RDK_LOG_DEBUG, LMLite %s : Executing Syscmd for RXTX STA shell script [%d] [%s] \n",__FUNCTION__, ret, rxtxarr));

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s Reset Timestamp[%u] \n", __FUNCTION__, reset_timestamp.tv_sec ));

        return 0;
    }
}

int SetNDTHarvestingStatus(BOOL status)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Old[%d] New[%d] \n", __FUNCTION__, NDTReportStatus, status ));

    if (NDTReportStatus != status)
        NDTReportStatus = status;
    else
        return 0;

    if (NDTReportStatus)
    {
        pthread_t tid;

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Starting Thread to start DeviceData Harvesting  \n", __FUNCTION__ ));

        if (pthread_create(&tid, NULL, StartNetworkDevicesTrafficHarvesting, NULL))
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Failed to Start Thread to start DeviceData Harvesting  \n", __FUNCTION__ ));
            return ANSC_STATUS_FAILURE;
        }
    }
    
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

BOOL GetNDTHarvestingStatus()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDTReportStatus ));
    return NDTReportStatus;
}

int SetNDTReportingPeriod(ULONG period)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT Old[%d] New[%d] \n", __FUNCTION__, NDTReportingPeriod, period ));
    NDTReportingPeriod = period;
    return 0;
}

ULONG GetNDTReportingPeriod()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDTReportingPeriod ));
    return NDTReportingPeriod;
}

int SetNDTPollingPeriod(ULONG period)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT Old[%d] New[%d] \n", __FUNCTION__, NDTPollingPeriod, period ));
    NDTPollingPeriod = period;
    if(NDTPollingPeriod > 5)
        MinimumNDTPollingPeriod = 5;
    else
        MinimumNDTPollingPeriod = NDTPollingPeriod;

    return 0;
}

BOOL ValidateNDTPeriod(ULONG period)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    BOOL ret = FALSE;
    ret = isvalueinarray(period, NetworkDeviceTrafficPeriods, 10);
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__ , ret ));
    return ret;
} 

ULONG GetNDTPollingPeriod()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDTPollingPeriod ));
    return NDTPollingPeriod;
}


ULONG GetNDTReportingPeriodDefault()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDTReportingPeriodDefault ));
    return NDTReportingPeriodDefault;
}

ULONG GetNDTPollingPeriodDefault()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDTPollingPeriodDefault ));
    return NDTPollingPeriodDefault;
}

ULONG GetNDTOverrideTTLDefault()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDTOverrideTTLDefault ));
    return NDTOverrideTTLDefault;
}

ULONG GetNDTOverrideTTL()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDTOverrideTTL ));
    return NDTOverrideTTL;
}

int SetNDTOverrideTTL(ULONG ttl)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDTOverrideTTL ));
    NDTOverrideTTL = ttl;
    return 0;
}

int _syscmd_ndt(char *cmd, char *retBuf, int retBufSize)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    FILE *f;
    char *ptr = retBuf;
    int bufSize = retBufSize, bufbytes = 0, readbytes = 0;

    if ((f = popen(cmd, "r")) == NULL) {
        CcspLMLiteTrace(("RDK_LOG_DEBUG, LMLite %s : popen %s error\n",__FUNCTION__, cmd));
        return -1;
    }

    while (!feof(f))
    {
        *ptr = 0;
        if (bufSize >= 128) {
            bufbytes = 128;
        } else {
            bufbytes = bufSize - 1;
        }

        fgets(ptr, bufbytes, f);
        readbytes = strlen(ptr);
        if ( readbytes == 0)
            break;
        bufSize -= readbytes;
        ptr += readbytes;
    }
    pclose(f);
    retBuf[retBufSize - 1] = 0;

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return 0;
}

void add_to_list_ndt(char* ip_table_line)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    const char * delim = "|";
    int rx_packets, tx_packets = 0;
    int external_bytes_down, external_bytes_up = 0;
    struct networkdevicetrafficdata *ptr = NULL;

    char* device_mac = strtok(ip_table_line, delim);
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, DeviceMAC[%s] \n", device_mac ));

    rx_packets =  atoi(strtok(NULL, delim));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, rx_packets[%d] \n",rx_packets ));

    external_bytes_down = atoi(strtok(NULL, delim));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, external_bytes_down[%d] \n",external_bytes_down ));

    tx_packets =  atoi(strtok(NULL, delim));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, tx_packets [%d] \n", tx_packets ));

    external_bytes_up = atoi(strtok(NULL, delim));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, external_bytes_up[%d] \n", external_bytes_up ));

    ptr = FindMacInLinkedList(device_mac);

    if(ptr == NULL)
    {
        ptr = malloc(sizeof(*ptr));
        if (ptr == NULL)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s :  Linked List Allocation Failed \n", __FUNCTION__ ));
            return;
        }

        gettimeofday(&(ptr->timestamp), NULL);
        ptr->timestamp.tv_sec -= tm_offset;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Timestamp[%u] \n",ptr->timestamp.tv_sec ));

        ptr->device_mac = strdup(device_mac);

        ptr->external_bytes_down = external_bytes_down;

        ptr->external_bytes_up = external_bytes_up;

        ptr->parent = strdup(NDT_DEFAULT_PARENT_MAC);

        ptr->device_type = strdup(NDT_DEFAULT_DEVICE_TYPE);

        ptr->is_updated = TRUE;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, is_updated[%d] \n",ptr->is_updated ));

        ptr->next = NULL;

        if (headnode == NULL)
        {
            headnode = currnode = ptr;
        }
        else
        {
            currnode->next = ptr;
            currnode = ptr;
        }
    }
    else
    {
        if((ptr->external_bytes_up != external_bytes_up) || (ptr->external_bytes_down != external_bytes_down))
        {
            ptr->external_bytes_up = external_bytes_up;
            ptr->external_bytes_down = external_bytes_down;

            gettimeofday(&(ptr->timestamp), NULL);
            ptr->timestamp.tv_sec -= tm_offset;
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Timestamp[%u] \n",ptr->timestamp.tv_sec ));

            ptr->is_updated = TRUE;
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, is_updated[%d] \n",ptr->is_updated ));
        }
        else
        {
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Timestamp[%u] \n",ptr->timestamp.tv_sec ));

            ptr->is_updated = FALSE;
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, is_updated[%d] \n",ptr->is_updated ));
        }
    }


    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return;
}

void print_list_ndt()
{
    int z = 0;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    struct networkdevicetrafficdata  *ptr = headnode;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Head Ptr [%lx]\n", (ulong)headnode));
    while (ptr != NULL)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Head Ptr [%lx] TimeStamp[%d] for Node[%d] with DeviceMAC[%s] Is_Updated[%d] \n", __FUNCTION__ ,(ulong)ptr, (int)ptr->timestamp.tv_sec, z, ptr->device_mac, ptr->is_updated));
        ptr = ptr->next;
        z++;
    }
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));
    return;
}

/* Function to delete the entire linked list */
void delete_list_ndt()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    currnode = headnode;
    struct networkdevicetrafficdata* next = NULL;

    while (currnode != NULL)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Deleting ND Node Head Ptr [%lx] with SSID[%s] \n",__FUNCTION__, (ulong)currnode, currnode->device_mac));
        next = currnode->next;
        free(currnode->device_mac);
        free(currnode->parent);
        free(currnode->device_type);                
        free(currnode);
        currnode = next;
    }
    headnode = currnode = NULL;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));

    return;
}

void GetIPTableData()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    int i = 0;
    char ip_table_line[256];
    FILE *fp = NULL;

    char rxtxcmd[128] = {0};
    char rxtxarr[128] = {0};
    int ret  = 0;
    sprintf(rxtxcmd, "/usr/ccsp/tad/rxtx_cur.sh");
    ret = _syscmd_ndt(rxtxcmd, rxtxarr, sizeof(rxtxarr));
    if(ret)
    {
        CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Executing Syscmd for RXTX Cur shell script [%d] [%s] \n",__FUNCTION__, ret, rxtxarr));
        return;
    }

    fp = fopen("/tmp/rxtx_cur.txt" , "r");
    if(!fp)
    {
        CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Error Opening File /tmp/rxtx_cur.txt \n",__FUNCTION__));
        return;
    }

    while (fgets(ip_table_line, 256, fp) != NULL)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Read Line from IP Table File is %s \n", ip_table_line));
        add_to_list_ndt(&ip_table_line);
    }

    fclose(fp);

    print_list_ndt();

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));
}




void* StartNetworkDevicesTrafficHarvesting( void *arg )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER \n", __FUNCTION__ ));

    ThreadStarted = TRUE;
    sem_init(&mutex, 0, 0);      /* initialize mutex - binary semaphore */
    clock_gettime(CLOCK_REALTIME, &ts);    // get current time
    ts.tv_sec += MinimumNDTPollingPeriod; // next trigger time
    currentNDTReportingPeriod = GetNDTReportingPeriod();
    getTimeOffsetFromUtc();

    if(GetNDTOverrideTTL() < currentNDTReportingPeriod)
    {
        SetNDTOverrideTTL(currentNDTReportingPeriod);
    }

    int ret = ResetEBTables();
    if(ret)
    {
        CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Failed to Reset EBTables  \n", __FUNCTION__ ));
    }

    do 
    {
        GetIPTableData();
        currentNDTReportingPeriod = currentNDTReportingPeriod + currentNDTPollingPeriod;
        currentNDTPollingPeriod = 0;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Sending to WebPA and AVRO currentNDTReportingPeriod [%ld] GetNDTReportingPeriod()[%ld]  \n", currentNDTReportingPeriod, GetNDTReportingPeriod()));

        if (currentNDTReportingPeriod >= GetNDTReportingPeriod())
        {
            struct networkdevicetrafficdata* ptr = headnode;
            if(ptr)
                {
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Sending to WebPA and AVRO currentNDTPollingPeriod [%ld] NDTReportingPeriod[%ld]  \n", currentNDTPollingPeriod, GetNDTReportingPeriod()));
                    network_devices_traffic_report(ptr, &reset_timestamp);
		    /* RDKB-7047 : Cleanup of headnode after report is sent */
    		    delete_list_ndt();
                }
            currentNDTReportingPeriod = 0;
        }
        
        if(!GetNDTOverrideTTL())
        {
            SetNDTPollingPeriod(GetNDTPollingPeriodDefault());
            SetNDTReportingPeriod(GetNDTReportingPeriodDefault());
            SetNDTOverrideTTL(GetNDTOverrideTTLDefault());
        }

        if(GetNDTOverrideTTL())
        {
            SetNDTOverrideTTL(GetNDTOverrideTTL() - GetNDTPollingPeriod());
        }

        do
        {
            currentNDTPollingPeriod += MinimumNDTPollingPeriod;
            WaitForSamaphoreTimeoutNDT();
            clock_gettime(CLOCK_REALTIME, &ts);    // Triggered, get current time
            ts.tv_sec += MinimumNDTPollingPeriod; // setup next trigger time
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Sleeping Inside ND Loop for currentNDTPollingPeriod[%ld] MinimumNDTPollingPeriod[%ld] \n", currentNDTPollingPeriod, MinimumNDTPollingPeriod));
        } while (currentNDTPollingPeriod < GetNDTPollingPeriod() && GetNDTHarvestingStatus());

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, currentNDTPollingPeriod [%ld] GetNDTPollingPeriod[%ld]\n", currentNDTPollingPeriod, GetNDTPollingPeriod()));

    } while (GetNDTHarvestingStatus());
    
    delete_list_ndt();

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));

    return NULL; // shouldn't return;
}

// End of File

