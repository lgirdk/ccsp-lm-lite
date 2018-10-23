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

#include "slap_definitions.h"

BOOL
InterfaceDevicesWifiExtender_Default_GetParamUlongValue
    (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
    );

BOOL
InterfaceDevicesWifiExtender_Default_SetParamUlongValue
   (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
       ULONG                      uValue
    );

BOOL
InterfaceDevicesWifiExtender_GetParamUlongValue
    (
		ANSC_HANDLE                 hInsContext,
		char*                       ParamName,
		ULONG*                      puLong
    );

BOOL
InterfaceDevicesWifiExtender_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
       ULONG                      uValue
    );

BOOL
InterfaceDevicesWifiExtender_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );
    
BOOL
InterfaceDevicesWifiExtender_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
InterfaceDevicesWifiExtender_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );


ULONG
InterfaceDevicesWifiExtender_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
InterfaceDevicesWifiExtender_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
InterfaceDevicesWifiExtender_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
InterfaceDevicesWifiExtender_Default_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

BOOL
InterfaceDevicesWifiExtender_Default_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
InterfaceDevicesWifiExtender_Default_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_STATUS
CosaDmlInterfaceDevicesWifiExtenderInit
    (
        ANSC_HANDLE                 hThisObject
    );
