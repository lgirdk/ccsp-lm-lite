/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include "slap_definitions.h"

BOOL
NetworkDevicesTraffic_Default_GetParamUlongValue
    (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
    );


BOOL
NetworkDevicesTraffic_GetParamUlongValue
    (
		ANSC_HANDLE                 hInsContext,
		char*                       ParamName,
		ULONG*                      puLong
    );

BOOL
NetworkDevicesTraffic_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
       ULONG                      uValue
    );

BOOL
NetworkDevicesTraffic_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );
    
BOOL
NetworkDevicesTraffic_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
NetworkDevicesTraffic_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );


ULONG
NetworkDevicesTraffic_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
NetworkDevicesTraffic_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
NetworkDevicesTraffic_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_STATUS
CosaDmlNetworkDevicesTrafficInit
    (
        ANSC_HANDLE                 hThisObject
    );
