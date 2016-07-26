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
void add_to_list(PLmObjectHost host, struct networkdevicestatusdata **head);
void print_list(struct networkdevicestatusdata *head);
void delete_list(struct networkdevicestatusdata **head);

static struct networkdevicestatusdata *headnode = NULL;
static struct networkdevicestatusdata *headnodeextender = NULL;

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

void add_to_list(PLmObjectHost host, struct networkdevicestatusdata **head)
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

        if(host->pStringParaValue[LM_HOST_Layer1InterfaceId])
                ptr->interface_name = strdup(host->pStringParaValue[LM_HOST_Layer1InterfaceId]);
        else
                ptr->interface_name = strdup("Unknown");

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, InterfaceName[%s] \n",ptr->interface_name ));
        ptr->is_active = host->bBoolParaValue[LM_HOST_ActiveId];
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Active[%d] \n",ptr->is_active ));
        ptr->next = NULL;
        gettimeofday(&(ptr->timestamp), NULL);
        ptr->timestamp.tv_sec -= tm_offset;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Timestamp[%u] \n",ptr->timestamp.tv_sec ));

         if(host->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent])
                ptr->parent = strdup(host->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]);
        else
                ptr->parent = strdup("Unknown");

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Parent[%s] \n",ptr->parent ));

         if(host->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType])
                ptr->device_type = strdup(host->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType]);
        else
                ptr->device_type = strdup("Unknown");
                       
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, DeviceType[%s] \n",ptr->device_type ));



        if (*head == NULL)
        {
            *head = ptr;
        }
        else
        {
            struct networkdevicestatusdata *prevnode, *currnode;

            // transverse to end of list and append new list
            prevnode = *head;
            currnode = (*head)->next;
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

void print_list(struct networkdevicestatusdata *head)
{
    int z = 0;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));
    struct networkdevicestatusdata  *ptr = head;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Head Ptr [%lx]\n", (ulong)head));
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
void delete_list(struct networkdevicestatusdata **head)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    struct networkdevicestatusdata* currnode = *head;
    struct networkdevicestatusdata* next = NULL;

    while (currnode != NULL)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : Deleting ND Node Head Ptr [%lx] with SSID[%s] \n",__FUNCTION__, (ulong)currnode, currnode->device_mac));
        next = currnode->next;
        free(currnode->device_mac);
        free(currnode->interface_name);
        free(currnode->parent);
        free(currnode->device_type);        
        free(currnode);
        currnode = next;
    }
    
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

            int LeaseTimeRemaining = lmHosts.hostArray[i]->LeaseTime - time(NULL);
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LeaseTime [%d]  CurrentTime[%d] LeaseTimeRemaining [%d] \n", lmHosts.hostArray[i]->LeaseTime, time(NULL), LeaseTimeRemaining));
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, AddressSource [%s] \n", lmHosts.hostArray[i]->pStringParaValue[LM_HOST_AddressSource]));
            
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, bClientReady [%d] \n", lmHosts.hostArray[i]->bClientReady ));

            if (lmHosts.hostArray[i]->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent] != NULL)
                {
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Parent [%x] \n", lmHosts.hostArray[i]->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent]));
                }
            else
                {
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Parent pointer is NULL \n"));
                }

            printf("RDK_LOG_DEBUG, bClientReady [%d] \n", lmHosts.hostArray[i]->bClientReady);

            if ( (!lmHosts.hostArray[i]->bClientReady) && (!strcmp(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_AddressSource], "DHCP")) && ( LeaseTimeRemaining <= 0 ))
                continue;

            if (lmHosts.hostArray[i]->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent] != NULL)
            {
                if(!strcasecmp(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_Parent], getFullDeviceMac()))
                    {
                        add_to_list(lmHosts.hostArray[i], &headnode);
                    }
                else
                    {
                        add_to_list(lmHosts.hostArray[i], &headnodeextender);
                    }
            }
            else
            {
                add_to_list(lmHosts.hostArray[i], &headnode);
            }

        }
        
        print_list(headnode);
        print_list(headnodeextender);

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

    if(GetNDSOverrideTTL() < currentReportingPeriod)
    {
         SetNDSOverrideTTL(currentReportingPeriod);
    }

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
                    network_devices_status_report(ptr, FALSE, getFullDeviceMac());
                    delete_list(&headnode);
                    headnode = NULL;
                }

            ptr = headnodeextender;
            if(ptr)
                {
                    int i = 0;
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Sending to WebPA and AVRO currentPollingPeriod [%ld] NDSReportingPeriod[%ld]  \n", currentPollingPeriod, GetNDSReportingPeriod()));
                    pthread_mutex_lock(&LmHostObjectMutex);
                    int array_size = lmHosts.numHost;
                    if (array_size)
                    {
                        for(i = 0; i < array_size; i++)
                        {
                            if((lmHosts.hostArray[i]->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType] != NULL) && !strcmp(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_X_RDKCENTRAL_COM_DeviceType], "extender"))
                                {
                                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before Calling network_devices_status_report with Extender [%s] \n", lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId] ));
                                    network_devices_status_report(ptr, TRUE, lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId]);
                                }
                        }
                    }

                    pthread_mutex_unlock(&LmHostObjectMutex);
                    delete_list(&headnodeextender);
                    headnodeextender = NULL;
                }

            currentReportingPeriod = 0;
        }


        
        if(!GetNDSOverrideTTL())
        {
            SetNDSPollingPeriod(GetNDSPollingPeriodDefault());
            SetNDSReportingPeriod(GetNDSReportingPeriodDefault());
            SetNDSOverrideTTL(GetNDSOverrideTTLDefault());
        }

        if(GetNDSOverrideTTL())
        {
            SetNDSOverrideTTL(GetNDSOverrideTTL() - GetNDSPollingPeriod());
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

