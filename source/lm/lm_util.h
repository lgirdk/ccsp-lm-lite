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

/*********************************************************************************

    description:

        This is the template file of ssp_internal.h for XxxxSsp.
        Please replace "XXXX" with your own ssp name with the same up/lower cases.

    ------------------------------------------------------------------------------

    revision:

        09/08/2011    initial revision.

**********************************************************************************/

#ifndef  _SSP_LMS_UTIL_H_
#define  _SSP_LMS_UTIL_H_

#include "ccsp_base_api.h"

#if 0

#define BoolStringTrue  "true"
#define BoolStringFalse "false"
#define BoolStringOne  "1"
#define BoolStringZero "0"

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

void
LanManager_ParamValueChanged
	(
		parameterSigStruct_t*		val,
		int				size,
        void            *user_data
	);

/*
 *  Derive MAC address from any IPv6 address according to RFC4291 Appendix A. 
 *  If the interface ID is not derived from MAC address, return interface ID.
 */
char * LanManager_GetMACAddrFromIPv6Addr
(
    const char * ipv6Address
);

/*
 *  Derive MAC address from IPv6 link-local address according to RFC4291 Appendix A. 
 *  If the interface ID is not derived from MAC address, return interface ID.
 *  If it is not link-local address [FE80::(64bits interface ID)] as defined in RFC4291 Section 2.5.6, return NULL. 
 */
char * LanManager_GetMACAddrFromIPv6LinkLocalAddr
(
    const char * ipv6Address
);

#endif

void LanManager_CheckCloneCopy (char **dest, const char *src);

#endif
