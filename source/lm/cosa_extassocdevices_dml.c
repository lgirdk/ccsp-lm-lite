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

static char *InterfaceDevicesWifiExtenderEnabled              = "eRT.com.cisco.spvtg.ccsp.lmlite.InterfaceDevicesWifiExtenderEnabled";
static char *InterfaceDevicesWifiExtenderReportingPeriod      = "eRT.com.cisco.spvtg.ccsp.lmlite.InterfaceDevicesWifiExtenderReportingPeriod";
static char *InterfaceDevicesWifiExtenderPollingPeriod        = "eRT.com.cisco.spvtg.ccsp.lmlite.InterfaceDevicesWifiExtenderPollingPeriod";
static char *InterfaceDevicesWifiExtenderDefReportingPeriod   = "eRT.com.cisco.spvtg.ccsp.lmlite.InterfaceDevicesWifiExtenderDefReportingPeriod";
static char *InterfaceDevicesWifiExtenderDefPollingPeriod     = "eRT.com.cisco.spvtg.ccsp.lmlite.InterfaceDevicesWifiExtenderDefPollingPeriod";



extern char* GetIDWSchemaBuffer();
extern int GetIDWSchemaBufferSize();
extern char* GetIDWSchemaIDBuffer();
extern int GetIDWSchemaIDBufferSize();


extern ANSC_STATUS GetNVRamULONGConfiguration(char* setting, ULONG* value);
extern ANSC_STATUS SetNVRamULONGConfiguration(char* setting, ULONG value);

ANSC_STATUS
CosaDmlInterfaceDevicesWifiExtenderInit
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    int retPsmGet = CCSP_SUCCESS;
    ULONG psmValue = 0;

    retPsmGet = GetNVRamULONGConfiguration(InterfaceDevicesWifiExtenderEnabled, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pReports->bIDWEnabled = psmValue;
        SetIDWHarvestingStatus(g_pReports->bIDWEnabled);
    }

    retPsmGet = GetNVRamULONGConfiguration(InterfaceDevicesWifiExtenderReportingPeriod, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pReports->uIDWReportingPeriod = psmValue;
        SetIDWReportingPeriod(g_pReports->uIDWReportingPeriod);
    }

    retPsmGet = GetNVRamULONGConfiguration(InterfaceDevicesWifiExtenderPollingPeriod, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pReports->uIDWPollingPeriod = psmValue;
        SetIDWPollingPeriod(g_pReports->uIDWPollingPeriod);
    }

    retPsmGet = GetNVRamULONGConfiguration(InterfaceDevicesWifiExtenderDefReportingPeriod, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pReports->uIDWReportingPeriodDefault = psmValue;
        SetIDWReportingPeriodDefault(g_pReports->uIDWReportingPeriodDefault);
    }

    retPsmGet = GetNVRamULONGConfiguration(InterfaceDevicesWifiExtenderDefPollingPeriod, &psmValue);
    if (retPsmGet == CCSP_SUCCESS) {
        g_pReports->uIDWPollingPeriodDefault = psmValue;
        SetIDWPollingPeriodDefault(g_pReports->uIDWPollingPeriodDefault);
    }

}

BOOL
InterfaceDevicesWifiExtender_GetParamBoolValue
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
        *pBool    =  GetIDWHarvestingStatus();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *pBool ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

BOOL
InterfaceDevicesWifiExtender_SetParamBoolValue
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
        g_pReports->bIDWEnabledChanged = true;
        g_pReports->bIDWEnabled = bValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, bValue ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

BOOL
InterfaceDevicesWifiExtender_GetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG*                      puLong
)
{
	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if ( AnscEqualString(ParamName, "PollingPeriod", TRUE))
    {
        *puLong =  GetIDWPollingPeriod();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        *puLong =  GetIDWReportingPeriod();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return FALSE;
}

BOOL
InterfaceDevicesWifiExtender_SetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG                       uValue
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if ( AnscEqualString(ParamName, "PollingPeriod", TRUE))
    {
        g_pReports->bIDWPollingPeriodChanged = true;
        g_pReports->uIDWPollingPeriod = uValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, uValue ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        g_pReports->bIDWReportingPeriodChanged = true;
        g_pReports->uIDWReportingPeriod = uValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, uValue ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

BOOL
InterfaceDevicesWifiExtender_GetParamStringValue
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
        int bufsize = GetIDWSchemaBufferSize();
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
            AnscCopyString(pValue, GetIDWSchemaBuffer());
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
        int bufsize = GetIDWSchemaIDBufferSize();
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
            AnscCopyString(pValue, GetIDWSchemaIDBuffer());
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
        InterfaceDevicesWifiExtender_Validate
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
InterfaceDevicesWifiExtender_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if(g_pReports->bIDWPollingPeriodChanged)
    {
        BOOL validated = ValidateIDWPeriod(g_pReports->uIDWPollingPeriod);    
        if(!validated)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : PollingPeriod Validation Failed : [%d] Value not Allowed \n", __FUNCTION__ , g_pReports->uIDWPollingPeriod));
            AnscCopyString(pReturnParamName, "PollingPeriod");
            *puLength = AnscSizeOfString("PollingPeriod");
            return FALSE;
        }
        if(GetIDWHarvestingStatus() && g_pReports->uIDWPollingPeriod > GetIDWPollingPeriod())
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : PollingPeriod Validation Failed : New Polling Period [%d] > Current Polling Period [%d] \n", __FUNCTION__ , g_pReports->uIDWPollingPeriod, GetIDWPollingPeriod() ));
            AnscCopyString(pReturnParamName, "PollingPeriod");
            *puLength = AnscSizeOfString("PollingPeriod");
            return FALSE;           
        }

        ULONG period = (g_pReports->bIDWReportingPeriodChanged == TRUE) ? g_pReports->uIDWReportingPeriod : GetIDWReportingPeriod();

        if(g_pReports->uIDWPollingPeriod > period )
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : PollingPeriod Validation Failed : New Polling Period [%d] > Current Reporting Period [%d] \n", __FUNCTION__ , g_pReports->uIDWPollingPeriod, period ));
            AnscCopyString(pReturnParamName, "PollingPeriod");
            *puLength = AnscSizeOfString("PollingPeriod");
            return FALSE;           
        }         
    }

    if(g_pReports->bIDWReportingPeriodChanged)
    {
        BOOL validated = ValidateIDWPeriod(g_pReports->uIDWReportingPeriod);    
        if(!validated)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : NeighboringAPPollingPeriod Validation Failed : [%d] Value not Allowed \n", __FUNCTION__ , g_pReports->uIDWReportingPeriod));
            AnscCopyString(pReturnParamName, "ReportingPeriod");
            *puLength = AnscSizeOfString("ReportingPeriod");
            return FALSE;
        }

        ULONG period = (g_pReports->bIDWPollingPeriodChanged == TRUE) ? g_pReports->uIDWPollingPeriod : GetIDWPollingPeriod();

        if(g_pReports->uIDWReportingPeriod < period )
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : ReportingPeriod Validation Failed : New Reporting Period [%d] < Current Polling Period [%d] \n", __FUNCTION__ , g_pReports->uIDWReportingPeriod, period ));
            AnscCopyString(pReturnParamName, "ReportingPeriod");
            *puLength = AnscSizeOfString("ReportingPeriod");
            return FALSE;           
        }
        
        if(GetIDWHarvestingStatus() && g_pReports->uIDWReportingPeriod > GetIDWReportingPeriod())
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : ReportingPeriod Validation Failed : New Reporting Period [%d] > Current Reporting Period [%d] \n", __FUNCTION__ , g_pReports->uIDWReportingPeriod, GetIDWReportingPeriod() ));
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
        InterfaceDevicesWifiExtender_Commit
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
InterfaceDevicesWifiExtender_Commit
(
    ANSC_HANDLE                 hInsContext
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));
    ULONG psmValue = 0;
    /* Network Device Parameters*/

    if(g_pReports->bIDWEnabledChanged)
    {
    SetIDWHarvestingStatus(g_pReports->bIDWEnabled);
    psmValue = g_pReports->bIDWEnabled;
    SetNVRamULONGConfiguration(InterfaceDevicesWifiExtenderEnabled, psmValue);
    g_pReports->bIDWEnabledChanged = false;
    }

    if(g_pReports->bIDWPollingPeriodChanged)
    {
    SetIDWPollingPeriod(g_pReports->uIDWPollingPeriod);
    SetIDWOverrideTTL(GetIDWOverrideTTLDefault());
    g_pReports->bIDWPollingPeriodChanged = false;
    }

    if(g_pReports->bIDWReportingPeriodChanged)
    {
    SetIDWReportingPeriod(g_pReports->uIDWReportingPeriod);
    SetIDWOverrideTTL(GetIDWOverrideTTLDefault());  
    g_pReports->bIDWReportingPeriodChanged = false;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        InterfaceDevicesWifiExtender_Rollback
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
InterfaceDevicesWifiExtender_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if(g_pReports->bIDWEnabledChanged)
    {
    g_pReports->bIDWEnabled = GetIDWHarvestingStatus();
    g_pReports->bIDWEnabledChanged = false;
    }

    if(g_pReports->bIDWPollingPeriodChanged)
    {
    g_pReports->uIDWPollingPeriod = GetIDWPollingPeriod();
    g_pReports->bIDWPollingPeriodChanged = false;
    }
    if(g_pReports->bIDWReportingPeriodChanged)
    {
    g_pReports->uIDWReportingPeriod = GetIDWReportingPeriod();
    g_pReports->bIDWReportingPeriodChanged = false;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

BOOL
InterfaceDevicesWifiExtender_Default_GetParamUlongValue
    (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if ( AnscEqualString(ParamName, "PollingPeriod", TRUE))
    {
        *puLong =  GetIDWPollingPeriodDefault();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        *puLong =  GetIDWReportingPeriodDefault();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "OverrideTTL", TRUE))
    {
        *puLong =  GetIDWOverrideTTLDefault();
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, *puLong ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return FALSE;
}

BOOL
InterfaceDevicesWifiExtender_Default_SetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG                       uValue
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if ( AnscEqualString(ParamName, "PollingPeriod", TRUE))
    {
        g_pReports->bIDWDefPollingPeriodChanged = true;
        g_pReports->uIDWPollingPeriodDefault = uValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, uValue ));
        return TRUE;
    }

    if ( AnscEqualString(ParamName, "ReportingPeriod", TRUE))
    {
        g_pReports->bIDWDefReportingPeriodChanged = true;
        g_pReports->uIDWReportingPeriodDefault = uValue;
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ParamName[%s] Value[%d] \n", __FUNCTION__ , ParamName, uValue ));
        return TRUE;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        InterfaceDevicesWifiExtender_Default_Validate
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
InterfaceDevicesWifiExtender_Default_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if(g_pReports->bIDWDefPollingPeriodChanged)
    {
        BOOL validated = ValidateIDWPeriod(g_pReports->uIDWPollingPeriodDefault);
        if(!validated)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Default PollingPeriod Validation Failed : [%d] Value not Allowed \n", __FUNCTION__ , g_pReports->uIDWPollingPeriodDefault));
            AnscCopyString(pReturnParamName, "PollingPeriod");
            *puLength = AnscSizeOfString("PollingPeriod");
            return FALSE;
        }

        ULONG period = (g_pReports->bIDWDefReportingPeriodChanged == TRUE) ? g_pReports->uIDWReportingPeriodDefault : GetIDWReportingPeriodDefault();

        if (g_pReports->uIDWPollingPeriodDefault > period )
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Default PollingPeriod Validation Failed : Newi Default Polling Period [%d] > Current Default Reporting Period [%d] \n", __FUNCTION__ , g_pReports->uIDWPollingPeriodDefault, period ));
            AnscCopyString(pReturnParamName, "PollingPeriod");
            *puLength = AnscSizeOfString("PollingPeriod");
            return FALSE;
        }
    }

    if(g_pReports->bIDWDefReportingPeriodChanged)
    {
        BOOL validated = ValidateIDWPeriod(g_pReports->uIDWReportingPeriodDefault);
        if(!validated)
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Default ReportingPeriod Validation Failed : [%d] Value not Allowed \n", __FUNCTION__ , g_pReports->uIDWReportingPeriodDefault));
            AnscCopyString(pReturnParamName, "ReportingPeriod");
            *puLength = AnscSizeOfString("ReportingPeriod");
            return FALSE;
        }

        ULONG period = (g_pReports->bIDWDefPollingPeriodChanged == TRUE) ? g_pReports->uIDWPollingPeriodDefault : GetIDWPollingPeriodDefault();

	if (g_pReports->uIDWReportingPeriodDefault < period )
        {
            CcspLMLiteTrace(("RDK_LOG_ERROR, LMLite %s : Default ReportingPeriod Validation Failed : New Default Reporting Period [%d] < Current Default Polling Period [%d] \n", __FUNCTION__ , g_pReports->uIDWReportingPeriodDefault, period ));
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
        InterfaceDevicesWifiExtender_Default_Rollback
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
InterfaceDevicesWifiExtender_Default_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

    if(g_pReports->bIDWDefPollingPeriodChanged)
    {
    g_pReports->uIDWPollingPeriodDefault = GetIDWPollingPeriodDefault();
    g_pReports->bIDWDefPollingPeriodChanged = false;
    }
    if(g_pReports->bIDWDefReportingPeriodChanged)
    {
    g_pReports->uIDWReportingPeriodDefault = GetIDWReportingPeriodDefault();
    g_pReports->bIDWDefReportingPeriodChanged = false;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        InterfaceDevicesWifiExtender_Default_Commit
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
InterfaceDevicesWifiExtender_Default_Commit
(
    ANSC_HANDLE                 hInsContext
)
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));
    ULONG psmValue = 0;

    if(g_pReports->bIDWDefReportingPeriodChanged)
    {
    SetIDWReportingPeriodDefault(g_pReports->uIDWReportingPeriodDefault);
    psmValue = g_pReports->uIDWReportingPeriodDefault;
    SetNVRamULONGConfiguration(InterfaceDevicesWifiExtenderDefReportingPeriod, psmValue);
    SetIDWOverrideTTL(GetIDWOverrideTTLDefault());
    g_pReports->bIDWDefReportingPeriodChanged = false;
    }

    if(g_pReports->bIDWDefPollingPeriodChanged)
    {
    SetIDWPollingPeriodDefault(g_pReports->uIDWPollingPeriodDefault);
    psmValue = g_pReports->uIDWPollingPeriodDefault;
    SetNVRamULONGConfiguration(InterfaceDevicesWifiExtenderDefPollingPeriod, psmValue);
    SetIDWOverrideTTL(GetIDWOverrideTTLDefault());
    g_pReports->bIDWDefPollingPeriodChanged = false;
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

    return 0;
}

