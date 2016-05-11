/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
   
#include "ansc_platform.h"
#include "cosa_ndstatus_dml.h"
#include "cosa_ndstatus_internal.h"
#include "ssp_global.h"
#include "base64.h"
#include "ccsp_trace.h"
#include "ccsp_psm_helper.h"
#include "lm_main.h"

/*Added for rdkb-4343*/
#include "ccsp_custom_logs.h"
#include "ccsp_lmliteLog_wrapper.h"

extern ANSC_HANDLE bus_handle;
extern char g_Subsystem[32];
extern COSA_DATAMODEL_LMLITE* g_pLMLite;
extern char* GetNDSchemaBuffer();
extern int GetNDSchemaBufferSize();

static char *NetworkDevicesEnabled              = "eRT.com.cisco.spvtg.ccsp.lmlite.NetworkDevicesEnabled";
static char *NetworkDevicesPollingPeriod        = "eRT.com.cisco.spvtg.ccsp.lmlite.NetworkDevicesPollingPeriod";
static char *NetworkDevicesReportingPeriod      = "eRT.com.cisco.spvtg.ccsp.lmlite.NetworkDevicesReportingPeriod";


ANSC_STATUS GetNVRamULONGConfiguration(char* setting, ULONG* value)
{
    char *strValue = NULL;
    int retPsmGet = CCSP_SUCCESS;

    retPsmGet = PSM_Get_Record_Value2(bus_handle,g_Subsystem, setting, NULL, &strValue);
    if (retPsmGet == CCSP_SUCCESS) {
        *value = _ansc_atoi(strValue);
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(strValue);
    }
    return retPsmGet;
}

ANSC_STATUS SetNVRamULONGConfiguration(char* setting, ULONG value)
{
    int retPsmSet = CCSP_SUCCESS;
    char psmValue[32] = {};

    sprintf(psmValue,"%d",value);
    retPsmSet = PSM_Set_Record_Value2(bus_handle,g_Subsystem, setting, ccsp_string, psmValue);
    if (retPsmSet != CCSP_SUCCESS) 
        {
        fprintf(stderr, "%s PSM_Set_Record_Value2 returned ERROR [%d] while setting [%s] Value [%d]\n",__FUNCTION__, retPsmSet, setting, value); 
        }
    else
        {
        fprintf(stderr, "%s PSM_Set_Record_Value2 returned SUCCESS[%d] while Setting [%s] Value [%d]\n",__FUNCTION__, retPsmSet, setting, value); 
        }
    return retPsmSet;
}

ANSC_STATUS
CosaDmlLMLiteInit
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    int retPsmGet = CCSP_SUCCESS;
    ULONG psmValue = 0;

    retPsmGet = GetNVRamULONGConfiguration(NetworkDevicesEnabled, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pLMLite->bEnabled = psmValue;
        SetNDHarvestingStatus(g_pLMLite->bEnabled);
    }

    retPsmGet = GetNVRamULONGConfiguration(NetworkDevicesPollingPeriod, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pLMLite->uPollingPeriod = psmValue;
        SetNDPollingPeriod(g_pLMLite->uPollingPeriod);
    }

    retPsmGet = GetNVRamULONGConfiguration(NetworkDevicesReportingPeriod, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pLMLite->uReportingPeriod = psmValue;
        SetNDReportingPeriod(g_pLMLite->uReportingPeriod);
    }  

}

BOOL
NetworkDevicesStatus_Default_GetParamUlongValue
    (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));
    fprintf(stderr, "RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ );

    if ( AnscEqualString(ParamName, "PollingPeriod", TRUE))
    {
        *puLong =  GetNDPollingPeriodDefault();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        *puLong =  GetNDReportingPeriodDefault();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "OverrideTTL", TRUE))
    {
        *puLong =  GetNDOverrideTTLDefault();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    fprintf(stderr, "RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ );
    return FALSE;
}

BOOL
NetworkDevicesStatus_GetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG*                      puLong
)
{
	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));
	fprintf(stderr, "RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ );

    if ( AnscEqualString(ParamName, "PollingPeriod", TRUE))
    {
        *puLong =  GetNDPollingPeriod();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        *puLong =  GetNDReportingPeriod();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

	fprintf(stderr, "RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ );
    return FALSE;
}

BOOL
NetworkDevicesStatus_SetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG                       uValue
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if ( AnscEqualString(ParamName, "PollingPeriod", TRUE))
    {
        g_pLMLite->bPollingPeriodChanged = true;
        g_pLMLite->uPollingPeriod = uValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, uValue ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        g_pLMLite->bReportingPeriodChanged = true;
        g_pLMLite->uReportingPeriod = uValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, uValue ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

BOOL
NetworkDevicesStatus_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if( AnscEqualString(ParamName, "Schema", TRUE))
    {
        /* collect value */
        int bufsize = GetNDSchemaBufferSize();
        if(!bufsize)
        {
            char result[1024] = "Schema Buffer is empty";
            AnscCopyString(pValue, (char*)&result);
            return FALSE;
        }
        else
	{
	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Buffer Size [%d] InputSize [%d]\n", bufsize, *pUlSize));
        if (bufsize < *pUlSize)
        {
            AnscCopyString(pValue, GetNDSchemaBuffer());
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, pValue Buffer Size [%d] \n", (int)strlen(pValue)));
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
            return FALSE;
        }
        else
        {
            *pUlSize = bufsize + 1;
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
            return TRUE;
        }
	}
    }

    if( AnscEqualString(ParamName, "SchemaID", TRUE))
    {
        /* collect value */
        int bufsize = GetNDSchemaIDBufferSize();
        if(!bufsize)
        {
            char result[1024] = "SchemaID Buffer is empty";
            AnscCopyString(pValue, (char*)&result);
            return FALSE;
        }
        else
        {

	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Buffer Size [%d] InputSize [%d]\n", bufsize, *pUlSize));
        if (bufsize < *pUlSize)
        {
            AnscCopyString(pValue, GetNDSchemaIDBuffer());
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, pValue Buffer Size [%d] \n", (int)strlen(pValue)));
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
            return FALSE;
        }
        else
        {
            *pUlSize = bufsize + 1;
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
            return TRUE;
        }
	}
    }

    AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return FALSE;
}



/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        NetworkDevicesStatus_Validate
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
NetworkDevicesStatus_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if(g_pLMLite->bPollingPeriodChanged)
    {
        BOOL validated = ValidateNDPeriod(g_pLMLite->uPollingPeriod);    
        if(!validated)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : PollingPeriod Validation Failed : [%d] Value not Allowed \n", __FUNCTION__ , g_pLMLite->uPollingPeriod));
            AnscCopyString(pReturnParamName, "PollingPeriod");
            *puLength = AnscSizeOfString("PollingPeriod");
            return FALSE;
        }
        if(g_pLMLite->uPollingPeriod > GetNDPollingPeriod())
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : PollingPeriod Validation Failed : New Polling Period [%d] > Current Polling Period [%d] \n", __FUNCTION__ , g_pLMLite->uPollingPeriod, GetNDPollingPeriod() ));
            AnscCopyString(pReturnParamName, "PollingPeriod");
            *puLength = AnscSizeOfString("PollingPeriod");
            return FALSE;           
        }
    }

    if(g_pLMLite->bReportingPeriodChanged)
    {
        BOOL validated = ValidateNDPeriod(g_pLMLite->uReportingPeriod);    
        if(!validated)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : NeighboringAPPollingPeriod Validation Failed : [%d] Value not Allowed \n", __FUNCTION__ , g_pLMLite->uReportingPeriod));
            AnscCopyString(pReturnParamName, "ReportingPeriod");
            *puLength = AnscSizeOfString("ReportingPeriod");
            return FALSE;
        }
        if(g_pLMLite->uReportingPeriod < GetNDPollingPeriod())
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : ReportingPeriod Validation Failed : New Reporting Period [%d] < Current Polling Period [%d] \n", __FUNCTION__ , g_pLMLite->uReportingPeriod, GetNDPollingPeriod() ));
            AnscCopyString(pReturnParamName, "ReportingPeriod");
            *puLength = AnscSizeOfString("ReportingPeriod");
            return FALSE;           
        }
        if(g_pLMLite->uReportingPeriod > GetNDReportingPeriod())
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : ReportingPeriod Validation Failed : New Reporting Period [%d] > Current Reporting Period [%d] \n", __FUNCTION__ , g_pLMLite->uReportingPeriod, GetNDReportingPeriod() ));
            AnscCopyString(pReturnParamName, "ReportingPeriod");
            *puLength = AnscSizeOfString("ReportingPeriod");
            return FALSE;           
        }
    }

     CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        NetworkDevicesStatus_Commit
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
NetworkDevicesStatus_Commit
(
    ANSC_HANDLE                 hInsContext
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));
    ULONG psmValue = 0;
    /* Network Device Parameters*/

    if(g_pLMLite->bEnabledChanged)
    {
    SetNDHarvestingStatus(g_pLMLite->bEnabled);
    psmValue = g_pLMLite->bEnabled;
    SetNVRamULONGConfiguration(NetworkDevicesEnabled, psmValue);
    g_pLMLite->bEnabledChanged = false;
    }

    if(g_pLMLite->bPollingPeriodChanged)
    {
    SetNDPollingPeriod(g_pLMLite->uPollingPeriod);
    SetNDOverrideTTL(GetNDOverrideTTLDefault());
    psmValue = g_pLMLite->uPollingPeriod;
    SetNVRamULONGConfiguration(NetworkDevicesPollingPeriod, psmValue);
    g_pLMLite->bPollingPeriodChanged = false;
    }

    if(g_pLMLite->bReportingPeriodChanged)
    {
    SetNDReportingPeriod(g_pLMLite->uReportingPeriod);
    SetNDOverrideTTL(GetNDOverrideTTLDefault());
    psmValue = g_pLMLite->uReportingPeriod;
    SetNVRamULONGConfiguration(NetworkDevicesReportingPeriod, psmValue);    
    g_pLMLite->bReportingPeriodChanged = false;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        NetworkDevicesStatus_Rollback
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
NetworkDevicesStatus_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if(g_pLMLite->bEnabledChanged)
    {
    g_pLMLite->bEnabled = GetNDHarvestingStatus();
    g_pLMLite->bEnabledChanged = false;
    }

    if(g_pLMLite->bPollingPeriodChanged)
    {
    g_pLMLite->uPollingPeriod = GetNDPollingPeriod();
    g_pLMLite->bPollingPeriodChanged = false;
    }
    if(g_pLMLite->bReportingPeriodChanged)
    {
    g_pLMLite->uReportingPeriod = GetNDReportingPeriod();
    g_pLMLite->bReportingPeriodChanged = false;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

BOOL
NetworkDevicesStatus_GetParamBoolValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    BOOL*                       pBool
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "Enabled", TRUE))
    {
        /* collect value */
        *pBool    =  GetNDHarvestingStatus();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *pBool ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

BOOL
NetworkDevicesStatus_SetParamBoolValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    BOOL                        bValue
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));
    /* check the parameter name and set the corresponding value */

    if ( AnscEqualString(ParamName, "Enabled", TRUE))
    {
        g_pLMLite->bEnabledChanged = true;
        g_pLMLite->bEnabled = bValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, bValue ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

