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

/**************************************************************************

    module:

        lm_wrapper_priv.c

    -------------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

    -------------------------------------------------------------------

    description:

        stub out for rdk-b

    -------------------------------------------------------------------

    author: WSN


    -------------------------------------------------------------------

    revision:

        06/20/2014    initial revision.

**************************************************************************/

#include <utctx.h>
#include <utctx_api.h>
#include <utapi.h>
#include <utapi_util.h>

#include "libswctl.h"
#include "lm_api.h"


int lm_wrapper_priv_stop_scan()
{
    UtopiaContext ctx = {0};
    bridgeInfo_t bridge_info;
    
    if (Utopia_Init(&ctx))
    {
        Utopia_GetBridgeSettings(&ctx, &bridge_info);
        Utopia_Free(&ctx, 0);
        if(bridge_info.mode == BRIDGE_MODE_OFF)
            return 0;
        else
            return 1;
    }else
        return SUCCESS;
}



void lm_wrapper_priv_getLanHostComments(char *physAddress, char *pComments)
{

    char buffer[256] = {0};
    UtopiaContext ctx;

	pComments[0] = 0;

    if ( physAddress == NULL && pComments == NULL)
        return;

    if( !Utopia_Init(&ctx) )
        return;

    Utopia_GetNamed(&ctx, UtopiaValue_USGv2_Lan_Clients_Mac, physAddress, buffer, sizeof(buffer));

    Utopia_Free(&ctx, 0);

	if(buffer[0])
    {
		char *p;

		p = strchr(buffer, '+');
		if(p!=NULL)
        {
			strcpy(pComments,p+1);
		}
	}

    return;
}

// return 0 if success 
int lm_wrapper_priv_set_lan_host_comments( LM_cmd_comment_t *cmd)
{
    UtopiaContext ctx;

    if(!Utopia_Init(&ctx))
        return 1;

    Utopia_set_lan_host_comments(&ctx, cmd->mac, cmd->comment);
    Utopia_Free(&ctx, 1);

    return SUCCESS;
}

int lm_wrapper_priv_getEthernetPort(char *mac)
{
        int port;
        char tmp[6];
        mac_string_to_array(mac, tmp);
        if(SWCTL_OK == swctl_findMacAddress(tmp, &port))
            return port;
        else
            return -1;
}


