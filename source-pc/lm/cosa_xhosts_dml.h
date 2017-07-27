/**************************************************************************

    module: cosa_hosts_dml.h

        For COSA Data Model Library Development

    -------------------------------------------------------------------
    description:

        This file defines the apis for objects to support Data Model Library.

    -------------------------------------------------------------------
***********************************************************************/
/***********************************************************************

 APIs for Object:

    XHosts.

    *  Hosts_GetParamUlongValue
    *  Hosts_SetParamUlongValue

***********************************************************************/

BOOL
XHosts_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      pUlong
    );

/***********************************************************************

 APIs for Object:

    XHosts.Host.{i}.

    *  XHost_GetEntryCount
    *  XHost_GetEntry
    *  XHost_IsUpdated
    *  XHost_GetParamBoolValue
    *  XHost_GetParamIntValue
    *  XHost_GetParamUlongValue
    *  XHost_GetParamStringValue
    *  XHost_SetParamBoolValue
    *  XHost_SetParamIntValue
    *  XHost_SetParamUlongValue
    *  XHost_SetParamStringValue
    *  XHost_Validate
    *  XHost_Commit
    *  XHost_Rollback

***********************************************************************/
ULONG
XHost_GetEntryCount
    (
        ANSC_HANDLE
    );

ANSC_HANDLE
XHost_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
XHost_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
XHost_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
XHost_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
XHost_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    );

BOOL
XHost_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      pUlong
    );

ULONG
XHost_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
XHost_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       strValue
    );

BOOL
XHost_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
XHost_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
XHost_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

/***********************************************************************

 APIs for Object:

    XHosts.XHost.{i}.XIPv4Address.{i}.

    *  XHost_IPv4Address_GetEntryCount
    *  XHost_IPv4Address_GetEntry
    *  XHost_IPv4Address_GetParamBoolValue
    *  XHost_IPv4Address_GetParamIntValue
    *  XHost_IPv4Address_GetParamUlongValue
    *  XHost_IPv4Address_GetParamStringValue

***********************************************************************/

ULONG
XHost_IPv4Address_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
XHost_IPv4Address_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

ULONG
XHost_IPv4Address_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

/***********************************************************************

 APIs for Object:

    XHosts.XHost.{i}.XIPv6Address.{i}.

    *  XHost_IPv6Address_GetEntryCount
    *  XHost_IPv6Address_GetEntry
    *  XHost_IPv6Address_GetParamBoolValue
    *  XHost_IPv6Address_GetParamIntValue
    *  XHost_IPv6Address_GetParamUlongValue
    *  XHost_IPv6Address_GetParamStringValue

***********************************************************************/

ULONG
XHost_IPv6Address_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
XHost_IPv6Address_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

ULONG
XHost_IPv6Address_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );
