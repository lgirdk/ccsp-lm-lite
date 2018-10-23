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

    module: cosa_hosts_dml.c

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

        01/14/2011    initial revision.

**************************************************************************/
#include <time.h>
#include "ansc_platform.h"
#include "cosa_hosts_dml.h"
#include "lm_main.h"
#include "lm_util.h"

extern LmObjectHosts lmHosts;

extern ULONG HostsUpdateTime;
extern pthread_mutex_t LmHostObjectMutex;
extern int g_Client_Poll_interval;
//#define TIME_NO_NEGATIVE(x) ((long)(x) < 0 ? 0 : (x))
#define COSA_DML_USERS_USER_ACCESS_INTERVAL     10
#define TIME_NO_NEGATIVE(x) ((long)(x) < 0 ? 0 : (x))
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
/***********************************************************************

 APIs for Object:

    Hosts.

    *  Hosts_GetParamBoolValue
    *  Hosts_GetParamIntValue
    *  Hosts_GetParamUlongValue
    *  Hosts_GetParamStringValue
    *  Hosts_SetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Hosts_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Hosts_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Hosts_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Hosts_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Hosts_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Hosts_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "X_CISCO_COM_ConnectedDeviceNumber", TRUE))
    {
        /* collect value */
        //*puLong = CosaDmlHostsGetOnline();
        *puLong = LM_get_online_device();
        //*puLong = HostsConnectedDeviceNum;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_ConnectedWiFiNumber", TRUE))
    {
        /* collect value */
        *puLong = 0;
        return TRUE;
    }
    
    if( AnscEqualString(ParamName, "X_RDKCENTRAL-COM_HostVersionId", TRUE))
    {
        /* collect value */
        *puLong = lmHosts.lastActivity;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "X_RDKCENTRAL-COM_HostCountPeriod", TRUE))
    {
        /* collect value */
        *puLong = g_Client_Poll_interval;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "X_RDKCENTRAL-COM_LMHost_Sync", TRUE))
    {
		*puLong = 0;
        return TRUE;
    }
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

BOOL
Hosts_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "X_RDKCENTRAL-COM_HostCountPeriod", TRUE))
    {
		char buf1[8];
		memset(buf1, 0, sizeof(buf1));
        g_Client_Poll_interval = uValue;
		snprintf(buf1,sizeof(buf1),"%d",uValue);
			if (syscfg_set(NULL, "X_RDKCENTRAL-COM_HostCountPeriod" , buf1) != 0) {
                     		     return ANSC_STATUS_FAILURE;
             } else {

                    if (syscfg_commit() != 0)
						{
                            CcspTraceWarning(("X_RDKCENTRAL-COM_HostCountPeriod syscfg_commit failed\n"));
							return ANSC_STATUS_FAILURE;
						}
			 }
        return TRUE;
    }
 	if( AnscEqualString(ParamName, "X_RDKCENTRAL-COM_LMHost_Sync", TRUE))
    {
	
        return TRUE;
    }
    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Hosts_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Hosts_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "X_RDKCENTRAL-COM_LMHost_Sync_From_WiFi", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, "");
        return 0;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Hosts_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Hosts_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "X_RDKCENTRAL-COM_LMHost_Sync_From_WiFi", TRUE))
    {
#ifdef USE_NOTIFY_COMPONENT
		char *st,
			 *ssid, 
			 *AssociatedDevice, 
			 *phyAddr, 
			 *RSSI, 
			 *Status;
		int  iRSSI,
			 iStatus;
			 

        /* save update to backup */
		phyAddr 		 = strtok_r(pString, ",", &st);
		AssociatedDevice = strtok_r(NULL, ",", &st);
		ssid 			 = strtok_r(NULL, ",", &st);
		RSSI 			 = strtok_r(NULL, ",", &st);
		Status 			 = strtok_r(NULL, ",", &st);

		iRSSI 			 = atoi(RSSI);
		iStatus 		 = atoi(Status);

		Wifi_Server_Sync_Function( phyAddr, AssociatedDevice, ssid, iRSSI, iStatus );
#endif /* USE_NOTIFY_COMPONENT */
		
        return TRUE;
    }
    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/***********************************************************************

 APIs for Object:

    Hosts.Host.{i}.

    *  Host_GetEntryCount
    *  Host_GetEntry
    *  Host_IsUpdated
    *  Host_Synchronize
    *  Host_GetParamBoolValue
    *  Host_GetParamIntValue
    *  Host_GetParamUlongValue
    *  Host_GetParamStringValue
    *  Host_SetParamBoolValue
    *  Host_SetParamIntValue
    *  Host_SetParamUlongValue
    *  Host_SetParamStringValue
    *  Host_Validate
    *  Host_Commit
    *  Host_Rollback

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Host_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
Host_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
	ULONG host_count = 0;
	pthread_mutex_lock(&LmHostObjectMutex);   
	host_count = lmHosts.numHost;
    pthread_mutex_unlock(&LmHostObjectMutex);
	return host_count;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        Host_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
Host_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
	ANSC_HANDLE host = NULL;	
	pthread_mutex_lock(&LmHostObjectMutex); 
	*pInsNumber = lmHosts.hostArray[nIndex]->instanceNum;

    host = (ANSC_HANDLE) (lmHosts.hostArray[nIndex]);
	pthread_mutex_unlock(&LmHostObjectMutex);
	return host;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Host_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
Host_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    if ( HostsUpdateTime == 0 ) 
    {
        HostsUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
    
    if ( HostsUpdateTime >= TIME_NO_NEGATIVE(AnscGetTickInSeconds() - COSA_DML_USERS_USER_ACCESS_INTERVAL ) )
    {
        return FALSE;
    }
    else 
    {
        HostsUpdateTime = AnscGetTickInSeconds();

        return TRUE;
    }
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Host_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Host_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{
    ULONG count,i;

    //CosaDmlHostsGetHosts(NULL,&count);

	//LM_get_host_info();
	LM_get_host_info(NULL,&count);//RDKB-EMU
    HostsUpdateTime = AnscGetTickInSeconds();

    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Host_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    //printf("Host_GetParamBoolValue %p, %s\n", hInsContext, ParamName);
	pthread_mutex_lock(&LmHostObjectMutex); 
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;
    int i = 0;
    for(; i<LM_HOST_NumBoolPara; i++){
        if( AnscEqualString(ParamName, lmHosts.pHostBoolParaName[i], TRUE))
        {
            /* collect value */
            *pBool = pHost->bBoolParaValue[i];
			pthread_mutex_unlock(&LmHostObjectMutex); 
            return TRUE;
        }
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_TrueStaticIPClient", TRUE))
    {
        /* collect value */
        *pBool = pHost->bTrueStaticIPClient;
		pthread_mutex_unlock(&LmHostObjectMutex); 
        return TRUE;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
	pthread_mutex_unlock(&LmHostObjectMutex); 
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Host_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
	pthread_mutex_lock(&LmHostObjectMutex);  
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;
#if 0
    int i = 0;
    for(; i<LM_HOST_NumIntPara; i++){
        if( AnscEqualString(ParamName, lmHosts.pHostIntParaName[i], TRUE))
        {
            /* collect value */
            *pInt = pHost->iIntParaValue[i];
            return TRUE;
        }
    }
#endif
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "X_CISCO_COM_ActiveTime", TRUE))
    {
        /* collect dynamic value */
        if(pHost->bBoolParaValue[LM_HOST_ActiveId]){
            ANSC_UNIVERSAL_TIME currentTime = {0};
            AnscGetLocalTime(&currentTime);
            pHost->iIntParaValue[LM_HOST_X_CISCO_COM_ActiveTimeId] = 
                (int)(AnscCalendarToSecond(&currentTime) - pHost->activityChangeTime);
        }
        *pInt = pHost->iIntParaValue[LM_HOST_X_CISCO_COM_ActiveTimeId];
		pthread_mutex_unlock(&LmHostObjectMutex); 
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_InactiveTime", TRUE))
    {
        /* collect dynamic value */
        if(!pHost->bBoolParaValue[LM_HOST_ActiveId]){
            ANSC_UNIVERSAL_TIME currentTime = {0};
            AnscGetLocalTime(&currentTime);
            pHost->iIntParaValue[LM_HOST_X_CISCO_COM_InactiveTimeId] = 
                (int)(AnscCalendarToSecond(&currentTime) - pHost->activityChangeTime);
        }
        *pInt = pHost->iIntParaValue[LM_HOST_X_CISCO_COM_InactiveTimeId];
		pthread_mutex_unlock(&LmHostObjectMutex); 
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_RSSI", TRUE))
    {
        /* collect value */
        //*pInt = pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId];
        //X_CISCO_COM_RSSI is mapped to 0 from all PA's	
        //	*pInt=0;
        *pInt = pHost->iIntParaValue[LM_HOST_X_CISCO_COM_RSSIId];//RDKB-EMU
		pthread_mutex_unlock(&LmHostObjectMutex); 
        return TRUE;
    }

    if( AnscEqualString(ParamName, "LeaseTimeRemaining", TRUE))
    {
        time_t currentTime = time(NULL);
        if(pHost->LeaseTime == 0xffffffff){
            *pInt = -1;
        }else if(currentTime <  pHost->LeaseTime){
            *pInt = pHost->LeaseTime - currentTime;
        }else{
            *pInt = 0;
        }
		pthread_mutex_unlock(&LmHostObjectMutex); 
        return TRUE;
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
	pthread_mutex_unlock(&LmHostObjectMutex); 
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Host_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    //printf("Host_GetParamUlongValue %p, %s\n", hInsContext, ParamName);
	pthread_mutex_lock(&LmHostObjectMutex); 
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;
    int i = 0;
    for(; i<LM_HOST_NumUlongPara; i++){
        if( AnscEqualString(ParamName, COSA_HOSTS_Extension1_Name, TRUE))
        {
            time_t currentTime = time(NULL);
            if(currentTime > pHost->activityChangeTime){
                *puLong = currentTime - pHost->activityChangeTime;
            }else{
                *puLong = 0;
            }
			pthread_mutex_unlock(&LmHostObjectMutex); 
            return TRUE;
        }
        else if( AnscEqualString(ParamName, lmHosts.pHostUlongParaName[i], TRUE))
        {
            /* collect value */
            *puLong = pHost->ulUlongParaValue[i];
			pthread_mutex_unlock(&LmHostObjectMutex); 
            return TRUE;
        }
    }
#if 0
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "X_CISCO_COM_DeviceType", TRUE))
    {
        /* collect value */
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_NetworkInterface", TRUE))
    {
        /* collect value */
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_ConnectionStatus", TRUE))
    {
        /* collect value */
        return TRUE;
    }
#endif

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
	pthread_mutex_unlock(&LmHostObjectMutex); 
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Host_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Host_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    //printf("Host_GetParamStringValue %p, %s\n", hInsContext, ParamName);
	pthread_mutex_lock(&LmHostObjectMutex); 
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;
    int i = 0;
    for(; i<LM_HOST_NumStringPara; i++){
		if( AnscEqualString(ParamName, "Layer3Interface", TRUE))
	    {
	        /* collect value */
			AnscCopyString(pValue, pHost->Layer3Interface);
			pthread_mutex_unlock(&LmHostObjectMutex);
	        return 0;
	    }
        else if( AnscEqualString(ParamName, lmHosts.pHostStringParaName[i], TRUE))
        {
            /* collect value */
            size_t len = 0;
            if(pHost->pStringParaValue[i]) len = strlen(pHost->pStringParaValue[i]);
            if(*pUlSize <= len){
                *pUlSize = len + 1;
				pthread_mutex_unlock(&LmHostObjectMutex); 
                return 1;
            }
            AnscCopyString(pValue, pHost->pStringParaValue[i]);
			pthread_mutex_unlock(&LmHostObjectMutex); 
            return 0;
        }
    }
#if 0
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Alias", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "PhysAddress", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "IPAddress", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "DHCPClient", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "AssociatedDevice", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "Layer1Interface", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "Layer3Interface", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "HostName", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_UPnPDevice", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_HNAPDevice", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_DNSRecords", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_HardwareVendor", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_SoftwareVendor", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_SerialNumbre", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_DefinedDeviceType", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_DefinedHWVendor", TRUE))
    {
        /* collect value */
        return 0;
    }

    if( AnscEqualString(ParamName, "X_CISCO_COM_DefinedSWVendor", TRUE))
    {
        /* collect value */
        return 0;
    }
#endif

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
	pthread_mutex_unlock(&LmHostObjectMutex); 
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Host_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */
    
    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Host_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Host_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Host_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    /* check the parameter name and set the corresponding value */
	pthread_mutex_lock(&LmHostObjectMutex); 
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;

    if( AnscEqualString(ParamName, "Comments", TRUE))
    {
        /* save update to backup */
        LanManager_CheckCloneCopy(&(pHost->pStringParaValue[LM_HOST_Comments]) , pString);
		pthread_mutex_unlock(&LmHostObjectMutex); 
        return TRUE;
    }
	pthread_mutex_unlock(&LmHostObjectMutex); 
    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Host_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
Host_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Host_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Host_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
	pthread_mutex_lock(&LmHostObjectMutex);     
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;
	pthread_mutex_unlock(&LmHostObjectMutex); 
    
    LMDmlHostsSetHostComment(pHost->pStringParaValue[LM_HOST_PhysAddressId], pHost->pStringParaValue[LM_HOST_Comments]);

    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Host_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Host_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
	pthread_mutex_lock(&LmHostObjectMutex);     
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;
	pthread_mutex_unlock(&LmHostObjectMutex); 

    return 0;
}

/***********************************************************************

 APIs for Object:

    Hosts.Host.{i}.IPv4Address.{i}.

    *  IPv4Address_GetEntryCount
    *  IPv4Address_GetEntry
    *  IPv4Address_GetParamBoolValue
    *  IPv4Address_GetParamIntValue
    *  IPv4Address_GetParamUlongValue
    *  IPv4Address_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        IPv4Address_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
Host_IPv4Address_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
	ULONG count = 0;		
	pthread_mutex_lock(&LmHostObjectMutex);       
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;    
    //printf("IPv4Address_GetEntryCount %d\n", pHost->numIPv4Addr);
	count = pHost->numIPv4Addr;
	pthread_mutex_unlock(&LmHostObjectMutex);   
    return count;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        IPv4Address_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
Host_IPv4Address_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
	PLmObjectHostIPAddress IPArr = NULL;
	pthread_mutex_lock(&LmHostObjectMutex);     
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;    
    //printf("IPv4Address_GetEntry %p, %ld\n", pHost, nIndex);
	IPArr = LM_GetIPArr_FromIndex(pHost, nIndex, IP_V4);
	if(IPArr)
		*pInsNumber  = nIndex + 1;
	pthread_mutex_unlock(&LmHostObjectMutex); 
    return  (ANSC_HANDLE)IPArr;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPv4Address_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_IPv4Address_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPv4Address_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_IPv4Address_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPv4Address_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_IPv4Address_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        IPv4Address_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Host_IPv4Address_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    //printf("IPv4Address_GetParamStringValue %p, %s\n", hInsContext, ParamName);
	pthread_mutex_lock(&LmHostObjectMutex);
    PLmObjectHostIPAddress pIPv4Address = (PLmObjectHostIPAddress) hInsContext;
    int i = 0;
    for(; i<LM_HOST_IPv4Address_NumStringPara; i++){
        if( AnscEqualString(ParamName, lmHosts.pIPv4AddressStringParaName[i], TRUE))
        {
            /* collect value */
            size_t len = 0;
            if(pIPv4Address->pStringParaValue[i]) len = strlen(pIPv4Address->pStringParaValue[i]);
            if(*pUlSize <= len){
                *pUlSize = len + 1;
				pthread_mutex_unlock(&LmHostObjectMutex);
                return 1;
            }
            AnscCopyString(pValue, pIPv4Address->pStringParaValue[i]);
			pthread_mutex_unlock(&LmHostObjectMutex);
            return 0;
        }
    }
#if 0
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "IPAddress", TRUE))
    {
        /* collect value */
        return 0;
    }
#endif

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
	pthread_mutex_unlock(&LmHostObjectMutex);
    return -1;
}

/***********************************************************************

 APIs for Object:

    Hosts.Host.{i}.IPv6Address.{i}.

    *  IPv6Address_GetEntryCount
    *  IPv6Address_GetEntry
    *  IPv6Address_GetParamBoolValue
    *  IPv6Address_GetParamIntValue
    *  IPv6Address_GetParamUlongValue
    *  IPv6Address_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        IPv6Address_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
Host_IPv6Address_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
	ULONG count = 0;	
	pthread_mutex_lock(&LmHostObjectMutex);    
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;    
    //printf("IPv6Address_GetEntryCount %d\n", pHost->numIPv6Addr);
	count = pHost->numIPv6Addr;
	pthread_mutex_unlock(&LmHostObjectMutex);
    return count;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        IPv6Address_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
Host_IPv6Address_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
	PLmObjectHostIPAddress IPArr = NULL;
	pthread_mutex_lock(&LmHostObjectMutex);    
    PLmObjectHost pHost = (PLmObjectHost) hInsContext;    
    //printf("IPv6Address_GetEntry %p, %ld\n", pHost, nIndex);
	IPArr = LM_GetIPArr_FromIndex(pHost, nIndex, IP_V6);
	if(IPArr)
		*pInsNumber  = nIndex + 1;
	pthread_mutex_unlock(&LmHostObjectMutex);
    return  (ANSC_HANDLE)IPArr;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPv6Address_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_IPv6Address_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPv6Address_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_IPv6Address_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPv6Address_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Host_IPv6Address_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    /* check the parameter name and return the corresponding value */

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        IPv6Address_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Host_IPv6Address_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    //printf("IPv6Address_GetParamStringValue %p, %s\n", hInsContext, ParamName);
	pthread_mutex_lock(&LmHostObjectMutex);
    PLmObjectHostIPAddress pIPv6Address = (PLmObjectHostIPAddress) hInsContext;
    int i = 0;
    for(; i<LM_HOST_IPv6Address_NumStringPara; i++){
        if( AnscEqualString(ParamName, lmHosts.pIPv6AddressStringParaName[i], TRUE))
        {
            /* collect value */
            size_t len = 0;
            if(pIPv6Address->pStringParaValue[i]) len = strlen(pIPv6Address->pStringParaValue[i]);
            if(*pUlSize <= len){
                *pUlSize = len + 1;
				pthread_mutex_unlock(&LmHostObjectMutex);
                return 1;
            }
            AnscCopyString(pValue, pIPv6Address->pStringParaValue[i]);
			pthread_mutex_unlock(&LmHostObjectMutex);
            return 0;
        }
    }
#if 0
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "IPAddress", TRUE))
    {
        /* collect value */
        return 0;
    }
#endif

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
	pthread_mutex_unlock(&LmHostObjectMutex);
    return -1;
}

