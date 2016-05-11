/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
   
#include "ansc_platform.h"
#include "cosa_ndstatus_dml.h"
#include "cosa_reports_internal.h"
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
extern COSA_DATAMODEL_REPORTS* g_pReports;

static char *NetworkDevicesTrafficEnabled              = "eRT.com.cisco.spvtg.ccsp.lmlite.NetworkDevicesTrafficEnabled";
static char *NetworkDevicesTrafficPollingPeriod        = "eRT.com.cisco.spvtg.ccsp.lmlite.NetworkDevicesTrafficPollingPeriod";
static char *NetworkDevicesTrafficReportingPeriod      = "eRT.com.cisco.spvtg.ccsp.lmlite.NetworkDevicesTrafficReportingPeriod";


extern char* GetNDTrafficSchemaBuffer();
extern int GetNDTrafficSchemaBufferSize();
extern char* GetNDTrafficSchemaIDBuffer();
extern int GetNDTrafficSchemaIDBufferSize();


extern ANSC_STATUS GetNVRamULONGConfiguration(char* setting, ULONG* value);
extern ANSC_STATUS SetNVRamULONGConfiguration(char* setting, ULONG value);

ANSC_STATUS
CosaDmlNetworkDevicesTrafficInit
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    int retPsmGet = CCSP_SUCCESS;
    ULONG psmValue = 0;

    retPsmGet = GetNVRamULONGConfiguration(NetworkDevicesTrafficEnabled, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pReports->bNDTEnabled = psmValue;
        SetNDTHarvestingStatus(g_pReports->bNDTEnabled);
    }

    retPsmGet = GetNVRamULONGConfiguration(NetworkDevicesTrafficPollingPeriod, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pReports->uNDTPollingPeriod = psmValue;
        SetNDTPollingPeriod(g_pReports->uNDTPollingPeriod);
    }

    retPsmGet = GetNVRamULONGConfiguration(NetworkDevicesTrafficReportingPeriod, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pReports->uNDTReportingPeriod = psmValue;
        SetNDTReportingPeriod(g_pReports->uNDTReportingPeriod);
    }  

}

BOOL
NetworkDevicesTraffic_Default_GetParamUlongValue
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
        *puLong =  GetNDTPollingPeriodDefault();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        *puLong =  GetNDTReportingPeriodDefault();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "OverrideTTL", TRUE))
    {
        *puLong =  GetNDTOverrideTTLDefault();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    fprintf(stderr, "RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ );
    return FALSE;
}

BOOL
NetworkDevicesTraffic_GetParamUlongValue
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
        *puLong =  GetNDTPollingPeriod();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        *puLong =  GetNDTReportingPeriod();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

	fprintf(stderr, "RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ );
    return FALSE;
}

BOOL
NetworkDevicesTraffic_SetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG                       uValue
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if ( AnscEqualString(ParamName, "PollingPeriod", TRUE))
    {
        g_pReports->bNDTPollingPeriodChanged = true;
        g_pReports->uNDTPollingPeriod = uValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, uValue ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        g_pReports->bNDTReportingPeriodChanged = true;
        g_pReports->uNDTReportingPeriod = uValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, uValue ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

BOOL
NetworkDevicesTraffic_GetParamStringValue
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
        int bufsize = GetNDTrafficSchemaBufferSize();
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
            AnscCopyString(pValue, GetNDTrafficSchemaBuffer());
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
        int bufsize = GetNDTrafficSchemaIDBufferSize();
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
            AnscCopyString(pValue, GetNDTrafficSchemaIDBuffer());
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
        NetworkDevicesTraffic_Validate
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
NetworkDevicesTraffic_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if(g_pReports->bNDTPollingPeriodChanged)
    {
        BOOL validated = ValidateNDTPeriod(g_pReports->uNDTPollingPeriod);    
        if(!validated)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : PollingPeriod Validation Failed : [%d] Value not Allowed \n", __FUNCTION__ , g_pReports->uNDTPollingPeriod));
            AnscCopyString(pReturnParamName, "PollingPeriod");
            *puLength = AnscSizeOfString("PollingPeriod");
            return FALSE;
        }
        if(g_pReports->uNDTPollingPeriod > GetNDTPollingPeriod())
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : PollingPeriod Validation Failed : New Polling Period [%d] > Current Polling Period [%d] \n", __FUNCTION__ , g_pReports->uNDTPollingPeriod, GetNDTPollingPeriod() ));
            AnscCopyString(pReturnParamName, "PollingPeriod");
            *puLength = AnscSizeOfString("PollingPeriod");
            return FALSE;           
        }
    }

    if(g_pReports->bNDTReportingPeriodChanged)
    {
        BOOL validated = ValidateNDTPeriod(g_pReports->uNDTReportingPeriod);    
        if(!validated)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : NeighboringAPPollingPeriod Validation Failed : [%d] Value not Allowed \n", __FUNCTION__ , g_pReports->uNDTReportingPeriod));
            AnscCopyString(pReturnParamName, "ReportingPeriod");
            *puLength = AnscSizeOfString("ReportingPeriod");
            return FALSE;
        }
        if(g_pReports->uNDTReportingPeriod < GetNDTPollingPeriod())
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : ReportingPeriod Validation Failed : New Reporting Period [%d] < Current Polling Period [%d] \n", __FUNCTION__ , g_pReports->uNDTReportingPeriod, GetNDTPollingPeriod() ));
            AnscCopyString(pReturnParamName, "ReportingPeriod");
            *puLength = AnscSizeOfString("ReportingPeriod");
            return FALSE;           
        }
        if(g_pReports->uNDTReportingPeriod > GetNDTReportingPeriod())
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : ReportingPeriod Validation Failed : New Reporting Period [%d] > Current Reporting Period [%d] \n", __FUNCTION__ , g_pReports->uNDTReportingPeriod, GetNDTReportingPeriod() ));
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
        NetworkDevicesTraffic_Commit
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
NetworkDevicesTraffic_Commit
(
    ANSC_HANDLE                 hInsContext
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));
    ULONG psmValue = 0;
    /* Network Device Parameters*/

    if(g_pReports->bNDTEnabledChanged)
    {
    SetNDTHarvestingStatus(g_pReports->bNDTEnabled);
    psmValue = g_pReports->bNDTEnabled;
    SetNVRamULONGConfiguration(NetworkDevicesTrafficEnabled, psmValue);
    g_pReports->bNDTEnabledChanged = false;
    }

    if(g_pReports->bNDTPollingPeriodChanged)
    {
    SetNDTPollingPeriod(g_pReports->uNDTPollingPeriod);
    SetNDTOverrideTTL(GetNDTOverrideTTLDefault());
    psmValue = g_pReports->uNDTPollingPeriod;
    SetNVRamULONGConfiguration(NetworkDevicesTrafficPollingPeriod, psmValue);
    g_pReports->bNDTPollingPeriodChanged = false;
    }

    if(g_pReports->bNDTReportingPeriodChanged)
    {
    SetNDTReportingPeriod(g_pReports->uNDTReportingPeriod);
    SetNDTOverrideTTL(GetNDTOverrideTTLDefault());
    psmValue = g_pReports->uNDTReportingPeriod;
    SetNVRamULONGConfiguration(NetworkDevicesTrafficReportingPeriod, psmValue);    
    g_pReports->bNDTReportingPeriodChanged = false;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        NetworkDevicesTraffic_Rollback
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
NetworkDevicesTraffic_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if(g_pReports->bNDTEnabledChanged)
    {
    g_pReports->bNDTEnabled = GetNDTHarvestingStatus();
    g_pReports->bNDTEnabledChanged = false;
    }

    if(g_pReports->bNDTPollingPeriodChanged)
    {
    g_pReports->uNDTPollingPeriod = GetNDTPollingPeriod();
    g_pReports->bNDTPollingPeriodChanged = false;
    }
    if(g_pReports->bNDTReportingPeriodChanged)
    {
    g_pReports->uNDTReportingPeriod = GetNDTReportingPeriod();
    g_pReports->bNDTReportingPeriodChanged = false;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

BOOL
NetworkDevicesTraffic_GetParamBoolValue
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
        *pBool    =  GetNDTHarvestingStatus();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *pBool ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

BOOL
NetworkDevicesTraffic_SetParamBoolValue
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
        g_pReports->bNDTEnabledChanged = true;
        g_pReports->bNDTEnabled = bValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, bValue ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

