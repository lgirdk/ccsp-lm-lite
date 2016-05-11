/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include "slap_definitions.h"

BOOL
NetworkDevicesStatus_Default_GetParamUlongValue
    (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
    );


BOOL
NetworkDevicesStatus_GetParamUlongValue
    (
		ANSC_HANDLE                 hInsContext,
		char*                       ParamName,
		ULONG*                      puLong
    );

BOOL
NetworkDevicesStatus_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
       ULONG                      uValue
    );

BOOL
NetworkDevicesStatus_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );
    
BOOL
NetworkDevicesStatus_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
NetworkDevicesStatus_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );


ULONG
NetworkDevicesStatus_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
NetworkDevicesStatus_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
NetworkDevicesStatus_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_STATUS
CosaDmlLMLiteInit
    (
        ANSC_HANDLE                 hThisObject
    );
