/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

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
#include "network_devices_status.h"
#include "ccsp_lmliteLog_wrapper.h"
#include "network_devices_status_avropack.h"
#include "lm_main.h"

static sem_t mutex;
static int ThreadStarted;
static struct timespec ts;
extern LmObjectHosts lmHosts;
ULONG NetworkDeviceStatusPeriods[10] = {30,60,300,900,1800,3600,10800,21600,43200,86400};

ULONG NDSPollingPeriodDefault = 300;
ULONG NDSReportingPeriodDefault = 300;

ULONG NDSPollingPeriod = 300;
ULONG NDSReportingPeriod = 300;

ULONG MinimumAPPollingPeriod = 5;
ULONG currentPollingPeriod = 0;
ULONG currentReportingPeriod = 0;
BOOL NDSReportStatus = FALSE;

ULONG NDSOverrideTTL = 300;
ULONG NDSOverrideTTLDefault = 300;

int tm_offset = 0;
bool isvalueinarray(ULONG val, ULONG *arr, int size);

void* StartNetworkDeviceStatusHarvesting( void *arg );
int _syscmd(char *cmd, char *retBuf, int retBufSize);
void add_to_list(PLmObjectHost host);
void print_list();
void delete_list();

static struct networkdevicestatusdata *headnode = NULL;
static struct networkdevicestatusdata *currnode = NULL;

extern pthread_mutex_t LmHostObjectMutex;

int getTimeOffsetFromUtc()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    char timezonecmd[128] = {0};
    char timezonearr[32] = {0};
    int ret  = 0;
    sprintf(timezonecmd, "dmcli eRT getv Device.Time.TimeOffset | grep value | awk '{print $5}'");
    ret = _syscmd(timezonecmd, timezonearr, sizeof(timezonearr));
    if(ret)
    {
        CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Executing Syscmd for DMCLI TimeOffset [%d] \n",__FUNCTION__, ret));
        return ret;
    }

    if (sscanf(timezonearr, "%d", &tm_offset) != 1)
    {
        CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Parsing Error for TimeOffset \n", __FUNCTION__ ));
        return -1;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s TimeOffset[%d] EXIT \n", __FUNCTION__, tm_offset ));

    return tm_offset;
}

char* GetCurrentTimeString()
{
    time_t current_time;
    char* c_time_string;

    /* Obtain current time. */
    current_time = time(NULL);

    /* Convert to local time format. */
    c_time_string = ctime(&current_time);

    return c_time_string;
}


ulong GetCurrentTimeInSecond()
{
    struct timespec ts;
    // get current time in second
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ulong)ts.tv_sec;
}


void WaitForSamaphoreTimeoutND()
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


bool isvalueinarray(ULONG val, ULONG *arr, int size)
{
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return true;
    }
    return false;
}


int SetNDSHarvestingStatus(BOOL status)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Old[%d] New[%d] \n", __FUNCTION__, NDSReportStatus, status ));

    if (NDSReportStatus != status)
        NDSReportStatus = status;
    else
        return 0;

    if (NDSReportStatus)
    {
        pthread_t tid;

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Starting Thread to start DeviceData Harvesting  \n", __FUNCTION__ ));

        if (pthread_create(&tid, NULL, StartNetworkDeviceStatusHarvesting, NULL))
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Failed to Start Thread to start DeviceData Harvesting  \n", __FUNCTION__ ));
            return ANSC_STATUS_FAILURE;
        }
    }
    
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

BOOL GetNDSHarvestingStatus()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDSReportStatus ));
    return NDSReportStatus;
}

int SetNDSReportingPeriod(ULONG period)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT Old[%d] New[%d] \n", __FUNCTION__, NDSReportingPeriod, period ));
    NDSReportingPeriod = period;
    return 0;
}

ULONG GetNDSReportingPeriod()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDSReportingPeriod ));
    return NDSReportingPeriod;
}

int SetNDSPollingPeriod(ULONG period)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT Old[%d] New[%d] \n", __FUNCTION__, NDSPollingPeriod, period ));
    NDSPollingPeriod = period;
    if(NDSPollingPeriod > 5)
        MinimumAPPollingPeriod = 5;
    else
        MinimumAPPollingPeriod = NDSPollingPeriod;

    return 0;
}

BOOL ValidateNDSPeriod(ULONG period)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    BOOL ret = FALSE;
    ret = isvalueinarray(period, NetworkDeviceStatusPeriods, 10);
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__ , ret ));
    return ret;
} 

ULONG GetNDSPollingPeriod()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDSPollingPeriod ));
    return NDSPollingPeriod;
}


ULONG GetNDSReportingPeriodDefault()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDSReportingPeriodDefault ));
    return NDSReportingPeriodDefault;
}

ULONG GetNDSPollingPeriodDefault()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDSPollingPeriodDefault ));
    return NDSPollingPeriodDefault;
}

ULONG GetNDSOverrideTTLDefault()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDSOverrideTTLDefault ));
    return NDSOverrideTTLDefault;
}

ULONG GetNDSOverrideTTL()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDSOverrideTTL ));
    return NDSOverrideTTL;
}

int SetNDSOverrideTTL(ULONG ttl)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT RET[%d] \n", __FUNCTION__, NDSOverrideTTL ));
    NDSOverrideTTL = ttl;
    return 0;
}

int _syscmd(char *cmd, char *retBuf, int retBufSize)
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

void add_to_list(PLmObjectHost host)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    struct networkdevicestatusdata *ptr = malloc(sizeof(*ptr));
    if (ptr == NULL)
    {
        CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s :  Linked List Allocation Failed \n", __FUNCTION__ ));
        return;
    }
    else
    {
        ptr->device_mac = strdup(host->pStringParaValue[LM_HOST_PhysAddressId]);
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, DeviceMAC[%s] \n",ptr->device_mac ));
        ptr->interface_name = strdup(host->pStringParaValue[LM_HOST_Layer1InterfaceId]);
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, InterfaceName[%s] \n",ptr->interface_name ));
        ptr->is_active = host->bBoolParaValue[LM_HOST_ActiveId];
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Active[%d] \n",ptr->is_active ));
        ptr->next = NULL;
        gettimeofday(&(ptr->timestamp), NULL);
        ptr->timestamp.tv_sec -= tm_offset;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Timestamp[%u] \n",ptr->timestamp.tv_sec ));
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

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return;
}

void print_list()
{
    int z = 0;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    struct networkdevicestatusdata  *ptr = headnode;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Head Ptr [%lx]\n", (ulong)headnode));
    while (ptr != NULL)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Head Ptr [%lx] TimeStamp[%d] for Node[%d] with SSID[%s] \n", __FUNCTION__ ,(ulong)ptr, (int)ptr->timestamp.tv_sec, z, ptr->device_mac));
        ptr = ptr->next;
        z++;
    }
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));
    return;
}

/* Function to delete the entire linked list */
void delete_list()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    currnode = headnode;
    struct networkdevicestatusdata* next = NULL;

    while (currnode != NULL)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Deleting ND Node Head Ptr [%lx] with SSID[%s] \n",__FUNCTION__, (ulong)currnode, currnode->device_mac));
        next = currnode->next;
        free(currnode->device_mac);
        free(currnode->interface_name);
        free(currnode);
        currnode = next;
    }
    headnode = currnode = NULL;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));

    return;
}

void GetLMHostData()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    int i = 0;
    pthread_mutex_lock(&LmHostObjectMutex);
    int array_size = lmHosts.numHost;
    if (array_size)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Hosts Array Size is %d \n", array_size));

        for(i = 0; i < array_size; i++)
        {
            add_to_list(lmHosts.hostArray[i]);
        }
        
        print_list();

    } // end of if statement
    else
    {
        CcspLMLiteTrace(("RDK_LOG_DEBUG, LMLite %s array_size [%d] \n",__FUNCTION__, array_size));
    } 

    pthread_mutex_unlock(&LmHostObjectMutex);
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));
}

void* StartNetworkDeviceStatusHarvesting( void *arg )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER \n", __FUNCTION__ ));

    ThreadStarted = TRUE;
    sem_init(&mutex, 0, 0);      /* initialize mutex - binary semaphore */
    clock_gettime(CLOCK_REALTIME, &ts);    // get current time
    ts.tv_sec += MinimumAPPollingPeriod; // next trigger time
    currentReportingPeriod = GetNDSReportingPeriod();
    getTimeOffsetFromUtc();

    do 
    {
        GetLMHostData();
        currentReportingPeriod = currentReportingPeriod + currentPollingPeriod;
        currentPollingPeriod = 0;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Sending to WebPA and AVRO currentReportingPeriod [%ld] GetNDSReportingPeriod()[%ld]  \n", currentReportingPeriod, GetNDSReportingPeriod()));

        if (currentReportingPeriod >= GetNDSReportingPeriod())
        {
            struct networkdevicestatusdata* ptr = headnode;
            if(ptr)
                {
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Sending to WebPA and AVRO currentPollingPeriod [%ld] NDSReportingPeriod[%ld]  \n", currentPollingPeriod, GetNDSReportingPeriod()));
                    network_devices_status_report(ptr);
                    delete_list();
                }
            currentReportingPeriod = 0;
        }

        if(GetNDSOverrideTTL())
        {
            SetNDSOverrideTTL(GetNDSOverrideTTL() - GetNDSPollingPeriod());
        }
        
        if(!GetNDSOverrideTTL())
        {
            SetNDSPollingPeriod(GetNDSPollingPeriodDefault());
            SetNDSReportingPeriod(GetNDSReportingPeriodDefault());
            SetNDSOverrideTTL(GetNDSOverrideTTLDefault());
        }

        do
        {
            currentPollingPeriod += MinimumAPPollingPeriod;
            WaitForSamaphoreTimeoutND();
            clock_gettime(CLOCK_REALTIME, &ts);    // Triggered, get current time
            ts.tv_sec += MinimumAPPollingPeriod; // setup next trigger time
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Sleeping Inside ND Loop for currentPollingPeriod[%ld] MinimumAPPollingPeriod[%ld] \n", currentPollingPeriod, MinimumAPPollingPeriod));
        } while (currentPollingPeriod < GetNDSPollingPeriod() && GetNDSHarvestingStatus());

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, currentPollingPeriod [%ld] GetNDSPollingPeriod[%ld]\n", currentPollingPeriod, GetNDSPollingPeriod()));

    } while (GetNDSHarvestingStatus());
    
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT \n", __FUNCTION__ ));

    return NULL; // shouldn't return;
}

// End of File

