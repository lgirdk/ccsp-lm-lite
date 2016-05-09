/*********************************************************************** 
  
    module: plugin_main.c

        Implement COSA Data Model Library Init and Unload apis.
 
    ---------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

    ---------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    ---------------------------------------------------------------

    revision:

        09/28/2011    initial revision.

**********************************************************************/

#include "ansc_platform.h"
#include "ansc_load_library.h"
#include "cosa_plugin_api.h"
#include "plugin_main.h"
#include "cosa_hosts_dml.h"

#define THIS_PLUGIN_VERSION                         1

int ANSC_EXPORT_API
COSA_Init
    (
        ULONG                       uMaxVersionSupported, 
        void*                       hCosaPlugInfo         /* PCOSA_PLUGIN_INFO passed in by the caller */
    )
{
    PCOSA_PLUGIN_INFO               pPlugInfo  = (PCOSA_PLUGIN_INFO)hCosaPlugInfo;

    if ( uMaxVersionSupported < THIS_PLUGIN_VERSION )
    {
      /* this version is not supported */
        return -1;
    }   
    
    pPlugInfo->uPluginVersion       = THIS_PLUGIN_VERSION;
    /* register the back-end apis for the data model */
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Hosts_GetParamUlongValue",  Hosts_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_GetEntryCount",  Host_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_GetEntry",  Host_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IsUpdated",  Host_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_Synchronize",  Host_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_GetParamBoolValue",  Host_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_GetParamIntValue",  Host_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_GetParamUlongValue",  Host_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_GetParamStringValue",  Host_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_SetParamBoolValue",  Host_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_SetParamIntValue",  Host_SetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_SetParamUlongValue",  Host_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_SetParamStringValue",  Host_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_Validate",  Host_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_Commit",  Host_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_Rollback",  Host_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv4Address_GetEntryCount",  Host_IPv4Address_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv4Address_GetEntry",  Host_IPv4Address_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv4Address_GetParamBoolValue",  Host_IPv4Address_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv4Address_GetParamIntValue",  Host_IPv4Address_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv4Address_GetParamUlongValue",  Host_IPv4Address_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv4Address_GetParamStringValue",  Host_IPv4Address_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv6Address_GetEntryCount",  Host_IPv6Address_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv6Address_GetEntry",  Host_IPv6Address_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv6Address_GetParamBoolValue",  Host_IPv6Address_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv6Address_GetParamIntValue",  Host_IPv6Address_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv6Address_GetParamUlongValue",  Host_IPv6Address_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Host_IPv6Address_GetParamStringValue",  Host_IPv6Address_GetParamStringValue);
    return  0;
}

BOOL ANSC_EXPORT_API
COSA_IsObjectSupported
    (
        char*                        pObjName
    )
{
    
    return TRUE;
}

void ANSC_EXPORT_API
COSA_Unload
    (
        void
    )
{
    /* unload the memory here */
}
