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

#include "ansc_platform.h"
#include "lm_util.h"
#include "lm_main.h"
#include "ccsp_memory.h"
#include <ctype.h>
#ifdef MLT_ENABLED
#include "rpl_malloc.h"
#include "mlt_malloc.h"
#endif
#include "safec_lib_common.h"

#if 0

/* Listent to value change signal on the following parameters:
 * Device.DHCPv4.Server.Pool.{i}.ClientNumberOfEntries
 * Device.DHCPv4.Server.Pool.{i}.StaticAddressNumberOfEntries
 * Device.DHCPv6.Server.Pool.{i}.ClientNumberOfEntries
 * Has set notify on in COSAXcalibur_CNS.XML, so do not need to set attribute here.
 * Add this function as the parameter value change signal call back function.
 */
void
LanManager_ParamValueChanged
	(
		parameterSigStruct_t*		val,
		int				size,
        void            *user_data
	)
{
    UNREFERENCED_PARAMETER(user_data);
    int i = 0;
    for(; i < size; i++){
        if(strstr(val[i].parameterName, "Device.DHCPv4.Server.Pool.") != NULL
            || strstr(val[i].parameterName, "Device.DHCPv6.Server.Pool.") != NULL)
        {
            CcspTraceDebug(("get value change signal from %s \n", val[i].parameterName));
            Hosts_PollHost();
        }
    }
}

/*
 *  Derive MAC address from any IPv6 address according to RFC4291 Appendix A. 
 *  If the interface ID is not derived from MAC address, return interface ID.
 */
char * LanManager_GetMACAddrFromIPv6Addr
(
    const char * ipv6Address
)
{
    if(!ipv6Address) return NULL;
    size_t len = strlen(ipv6Address);
    if(len <= 0) return NULL;
    char ipv6AddrOrg[200] = {0};
    /* turn to upper case. */
    int i;
    for(i=0; i<(int)len; i++) ipv6AddrOrg[i] = toupper(ipv6Address[i]);
    /* Cut the prefix part after '/'. */
    for(i=len-1; i>=0; i--){
        if(ipv6AddrOrg[i] == '/'){
            ipv6AddrOrg[i] = 0;
            len = i;
            break;
        }
    }
    /* Check how many '::' and ':' in this string. */
    int colonNum = 0;
    for(i=0; i<(int)len; i++) {
        if(ipv6AddrOrg[i] == ':'){
            colonNum++;
        }
    }
    char *doubleColon = NULL;
    if(colonNum < 7){
        doubleColon = strstr(ipv6AddrOrg, "::");
        if(!doubleColon) return NULL;
    } else {
       /*CID: 74063 Explicit null dereferenced*/
       return NULL;
    }
    char ipv6Addr[200] = {0};
    errno_t rc = -1;
    /* Copy to new string ipv6Addr and fill in all missed ':' to "::" */

    *doubleColon = 0;
    rc = strcpy_s(ipv6Addr,  sizeof(ipv6Addr),ipv6AddrOrg);
    ERR_CHK(rc);
    for(i=0; i<=(7-colonNum); i++){
        rc = strcat_s(ipv6Addr,  sizeof(ipv6Addr),":");
        ERR_CHK(rc);
    }
    doubleColon++;
    rc = strcat_s(ipv6Addr, sizeof(ipv6Addr),doubleColon);
    ERR_CHK(rc);
    len = strlen(ipv6Addr);

    /* Cut off the interface ID. Fill in omitted 0. */
    char interfaceId[20] = {0};
    interfaceId[19] = 0;
    int ipv6AddrId = len - 1;
    int ipv6AddrLastColonId = len;
    int ipv6AddrColonCountedNum = 0; 
    int interfaceIdId = 18;
    while(ipv6AddrId >= 0 && interfaceIdId >= 0 && ipv6AddrColonCountedNum < 4)
    {
        if(ipv6Addr[ipv6AddrId] == ':'){
            /* Fill in omitted 0. */
            int omittedCharNum = 5 - ipv6AddrLastColonId + ipv6AddrId;
            if(omittedCharNum > 0) { 
                for(i=0; i<omittedCharNum; i++){
                    interfaceId[interfaceIdId--] = '0';
                }
            }
            interfaceId[interfaceIdId] = ':';
            ipv6AddrColonCountedNum++;
            ipv6AddrLastColonId = ipv6AddrId;
        }
        else {
            interfaceId[interfaceIdId] = ipv6Addr[ipv6AddrId];
        }
        interfaceIdId--;
        ipv6AddrId--;
    }

    /* Check if interface ID is derived from MAC address: */
    if(interfaceId[7] != 'F' || interfaceId[8] != 'F' || interfaceId[10] != 'F' || interfaceId[11] != 'E')
        return AnscCloneString(interfaceId);
    char flag = (interfaceId[1] >= 'A' && interfaceId[1] <= 'F') ? 10+interfaceId[1]-'A' : interfaceId[1]-'0';
    /* It is not cc1g */
    if((flag & 0x02) == 0) return AnscCloneString(interfaceId);
    flag = flag & 0xfd; 
    /* Derive mac address. */
    char mac[18];
    mac[17] = 0;
    mac[0] = interfaceId[0];
    mac[1] = (flag >= 10) ? flag+'A'-10 : flag+'0';
    mac[2] = ':';
    for(i=3; i<=7; i++) mac[i] = interfaceId[i-1];
    mac[8] = ':';
    for(i=9; i<=13; i++) mac[i] = interfaceId[i+3];
    mac[14] = ':';
    mac[15] = interfaceId[17];
    mac[16] = interfaceId[18];
    return AnscCloneString(mac);
}

/*
 *  Derive MAC address from IPv6 link-local address according to RFC4291 Appendix A. 
 *  If the interface ID is not derived from MAC address, return interface ID.
 *  If it is not link-local address [FE80::(64bits interface ID)] as defined in RFC4291 Section 2.5.6, return NULL. 
 */
char * LanManager_GetMACAddrFromIPv6LinkLocalAddr
(
    const char * ipv6Address
)
{
    if(!ipv6Address) return NULL;
    size_t len = strlen(ipv6Address);
    if(len < 6) return NULL;
    int i;
    char ipv6Addr[200] = {0};
    /* turn to upper case. */
    for(i=0; i<(int)len; i++) ipv6Addr[i] = toupper(ipv6Address[i]);
    /* Not a local-link address. */
    if(strstr(ipv6Addr, "FE80:") != ipv6Addr) return NULL;
    /* Cut the prefix part after '/'. */
    for(i=len-1; i>=0; i--) {
        if(ipv6Addr[i] == '/'){
            ipv6Addr[i] = 0;
            len = i;
            break;
        }
    }
    /* Cut off the interface ID. Fill in omitted 0. */
    char interfaceId[20] = {0};
    interfaceId[19] = 0;
    int ipv6AddrId = len - 1;
    int ipv6AddrLastColonId = len;
    int ipv6AddrColonCountedNum = 0; 
    int interfaceIdId = 18;
    while(ipv6AddrId >= 0 && interfaceIdId >= 0 && ipv6AddrColonCountedNum < 4)
    {
        if(ipv6Addr[ipv6AddrId] == ':'){
            /* Fill in omitted 0. */
            int omittedCharNum = 5 - ipv6AddrLastColonId + ipv6AddrId;
            if(omittedCharNum > 0) { 
                for(i=0; i<omittedCharNum; i++){
                    interfaceId[interfaceIdId--] = '0';
                }
            }
            interfaceId[interfaceIdId] = ':';
            ipv6AddrColonCountedNum++;
            ipv6AddrLastColonId = ipv6AddrId;
        }
        else {
            interfaceId[interfaceIdId] = ipv6Addr[ipv6AddrId];
        }
        interfaceIdId--;
        ipv6AddrId--;
    }

    /* Check if interface ID is derived from MAC address: */
    if(interfaceId[7] != 'F' || interfaceId[8] != 'F' || interfaceId[10] != 'F' || interfaceId[11] != 'E')
        return AnscCloneString(interfaceId);
    char flag = (interfaceId[1] >= 'A' && interfaceId[1] <= 'F') ? 10+interfaceId[1]-'A' : interfaceId[1]-'0';
    /* It is not cc1g */
    if((flag & 0x02) == 0) return AnscCloneString(interfaceId);
    flag = flag & 0xfd; 
    /* Derive mac address. */
    char mac[18];
    mac[17] = 0;
    mac[0] = interfaceId[0];
    mac[1] = (flag >= 10) ? flag+'A'-10 : flag+'0';
    mac[2] = ':';
    for(i=3; i<=7; i++) mac[i] = interfaceId[i-1];
    mac[8] = ':';
    for(i=9; i<=13; i++) mac[i] = interfaceId[i+3];
    mac[14] = ':';
    mac[15] = interfaceId[17];
    mac[16] = interfaceId[18];
    return AnscCloneString(mac);
}

#endif

void LanManager_CheckCloneCopy (char **dest, const char *src)
{
	size_t src_len;

	if (src == NULL)
		return;

	src_len = strlen (src);

	/*
	   If src is an empty string then abort. This follows the original
	   implementation but it looks wrong (it means this function can't be
	   used to set dest to an empty string). There seem to be various
	   workarounds in the code for this limitation (e.g. passing src as
	   "empty" or " ") so leave it as-is for now, but it needs review.
	*/
	if (src_len == 0)
		return;

	/*
	   If dest has already been allocated and the new string is the same
	   length or shorter than the old one then reuse the old buffer. Else
	   free the buffer and allocate again.
	*/
	if (*dest) {
		size_t dest_len = strlen (*dest);
		if (src_len <= dest_len) {
			memcpy (*dest, src, src_len + 1);
			return;
		}
		free (*dest);
	}

	*dest = malloc (src_len + 1);

	if (*dest == NULL)
		return;

	memcpy (*dest, src, src_len + 1);
}
