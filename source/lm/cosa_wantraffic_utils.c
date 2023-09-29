/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
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


/**************************************************************************

    module: cosa_wantraffic_utils.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file defines the apis for objects to support Data Model Library.

    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        06/13/2022    initial revision.

**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "utapi/utapi.h"
#include "ansc_platform.h"
#include "syscfg/syscfg.h"
#include "safec_lib_common.h"
#include "cosa_wantraffic_api.h"
#include "cosa_wantraffic_utils.h"

#define ETH_WAN_ENABLE_STRING    "eth_wan_enabled"

#ifdef _SR300_PRODUCT_REQ_
extern rbusHandle_t rbus_handle;
#endif

static UINT EnabledDscpCount = 0;

static INT SetMemoryslab(INT i);
static pstDSCPInfo_t NewNode(UINT Dscp);
static pstDSCPInfo_t InsertDscpNode(pstDSCPInfo_t DscpTree, UINT dscp);
static pstDSCPInfo_t DeleteDisabledDscp(pstDSCPInfo_t DscpTree);
static pstDSCPInfo_t DeleteDisabledClients(pstDSCPInfo_t DscpTree);


/**********************************************************************
    function:
        RemoveSpaces
    description:
        This function is called to remove spaces in dscp list
    argument:
        CHAR*    str,    dscp list
    return:
        CHAR*    dscp list
**********************************************************************/
CHAR* RemoveSpaces(CHAR *str)
{
    INT i=0,j=0;
    while(str[i] != '\0')
    {
        if (str[i] != ' ')
            str[j++] = str[i];
        i++;
    }
    str[j] = '\0';
    return str;
}
#ifdef _SR300_PRODUCT_REQ_
/**********************************************************************
    function:
        GetCurrentActiveInterface
    description:
        This function is called to get active interface from rbus
    output argument:
        CHAR*    str,    interface name which is active
    return:
        INT    status
**********************************************************************/
INT GetCurrentActiveInterface(CHAR *ifname)
{
    rbusValue_t value;
    int rc = RBUS_ERROR_SUCCESS;
    char* val = NULL, *token = NULL, c;

    rc = rbus_get(rbus_handle, TR181_ACTIVE_INTERFACE, &value);

    if(rc == RBUS_ERROR_SUCCESS)
    {
        val = rbusValue_ToString(value,0,0);
        if(val)
        {
            if(strlen(val) > 0)
            {
                WTC_LOG_INFO("WAN Interfaces status = %s",val);
            }
            else
            {
                WTC_LOG_INFO("rbus val is empty for active interface");
                return 0;
            }
        }
        else
        {
            WTC_LOG_INFO("rbus val NULL for active interface");
            return 0;
        }
    }
    else
    {
        WTC_LOG_INFO("rbus get failed for %s", TR181_ACTIVE_INTERFACE);
        return 0;
    }

   //DSL,1|WANOE,0|ADSL,0
    token = strtok(val, "|"); 
    while (token != NULL) 
    {
        c = token[strlen(token)-1]; // last char in token
        if(c == '1') 
        {
            if((strlen(token)-2) < BUFLEN_64) 
            {
                strncpy(ifname, token, strlen(token)-2); 
                WTC_LOG_INFO("Current active interface = %s",ifname);
                return 1;
            }
            else
            {
                WTC_LOG_INFO("Active interface is invalid");
                return 0;
            }
        }
        token = strtok(NULL, "|");
    }
    
    WTC_LOG_INFO("Getting current active interface failed");

    return 0;

}
#endif
/**********************************************************************
    function:
        GetEthWANIndex
    description:
        This function is called to retrieve the Wan mode
    argument:
        None
    return:     WAN_INTERFACE if succeeded;
                0 if error.
**********************************************************************/

WAN_INTERFACE GetEthWANIndex(VOID)
{
    errno_t rc = -1;
    INT ind = -1;

    #ifdef _SR300_PRODUCT_REQ_
    /* Use wan manager data model and rbus APIS for sky platform
    "Device.X_RDK_WanManager.InterfaceActiveStatus"
    //DSL,0|WANOE,0|ADSL,0 -> Initial value
    //DSL,1|WANOE,0|ADSL,0 -> when DSL is up(vdsl or adsl) -> Interface.1
    //DSL,0|WANOE,1|ADSL,0 -> when WANoE is up -> Interface.2
    */
    CHAR active_interface_name[BUFLEN_64]={'\0'};
    if(GetCurrentActiveInterface(active_interface_name) == 1)
    {

        rc = strcmp_s(active_interface_name, strlen(active_interface_name), "WANOE", &ind);
        ERR_CHK(rc);
        if ((rc == EOK) && (!ind))
        {
            WTC_LOG_INFO("EWAN Mode");
            return EWAN;
        }
        else
        {
            WTC_LOG_INFO("DSL Mode");
            return DSL;
        }
    }
    else 
    {
        WTC_LOG_INFO("Active interface is INVALID");
        return INVALID_MODE;
    }
    // other platforms which uses ETH_WAN_ENABLE_STRING
    #else
    CHAR eth_wan_enabled[BUFLEN_64]={'\0'};
    if(syscfg_get(NULL,ETH_WAN_ENABLE_STRING,eth_wan_enabled,sizeof(eth_wan_enabled)) == 0 )
    {
        rc = strcmp_s(eth_wan_enabled, strlen(eth_wan_enabled), "true", &ind);
        ERR_CHK(rc);
        if ((rc == EOK) && (!ind))
        {
            WTC_LOG_INFO("EWAN Mode");
            return EWAN;
        }
        else
        {
            WTC_LOG_INFO("DOCSIS Mode");
            return DOCSIS;
        }
    }
    else
    {
        WTC_LOG_ERROR("Syscfg_get failed to get wan mode");
        return INVALID_MODE;
    }
    #endif
}

/**********************************************************************
    function:
        IsBridgeMode
    description:
        This function is called to retrieve the bridge mode
    argument:
        None
    return:     0 if Router mode;
                1 if Bridge mode.
**********************************************************************/

INT IsBridgeMode(VOID)
{
    UtopiaContext ctx = {0};
    bridgeInfo_t bridge_info = {0};

    if (Utopia_Init(&ctx))
    {
        Utopia_GetBridgeSettings(&ctx, &bridge_info);
        Utopia_Free(&ctx, 0);
        if(bridge_info.mode == BRIDGE_MODE_OFF)
        {
            WTC_LOG_INFO("Router Mode");
            return 0;
        }
        else
        {
            WTC_LOG_INFO("Bridge Mode");
            return 1;
        }
    }
    else
    {
        WTC_LOG_ERROR("Utopia Init failure. Unable to get Bridge mode");
        return INVALID_MODE;
    }
}

/**********************************************************************
    function:
        SetMemoryslab
    description:
        This function is called to set the memory slab based on the incoming clients.
    argument:
        INT    i  -  Number of clients
    return:
        INT
**********************************************************************/
static INT SetMemoryslab(INT i)
{
    return (!!i) * CLIENT_ALLOC_SLAB * ((i/10)+1);
}

/**********************************************************************
    function:
        CheckForAllDscpValuePresence
    description:
        This function is called to find if invalid dscp -1 is present in input string or not.
    argument:
        CHAR*           str    - Dscp string
    return:
        TRUE            if -1 present
        FALSE           if -1 not present
**********************************************************************/
BOOL CheckForAllDscpValuePresence(CHAR *str)
{
    if(strstr(str, ALL_DSCP_VALUE))
    {
        WTC_LOG_INFO("ALL_DSCP_VALUE -1 is present");
        return TRUE;
    }
    WTC_LOG_INFO("ALL_DSCP_VALUE -1 is not present");
    return FALSE;
}

/**********************************************************************
    function:
        NewNode
    description:
        This function is called to create a new node for a dscp value.
    argument:
        INT    Dscp    -   Dscp value
    return:
        pstDSCPInfo_t
**********************************************************************/
static pstDSCPInfo_t NewNode(UINT Dscp)
{
    pstDSCPInfo_t DscpNode = (stDCSPInfo_t *) malloc (sizeof(stDCSPInfo_t));
    if(DscpNode == NULL)
    {
        WTC_LOG_ERROR("New Node creation failed.");
        return NULL;
    }
    DscpNode->Dscp = Dscp;
    DscpNode->NumClients = 0;
    DscpNode->Left = NULL;
    DscpNode->Right = NULL;
    DscpNode->ClientList = NULL;
    DscpNode->IsUpdated = TRUE;
    DscpNode->MemorySlab = CLIENT_ALLOC_SLAB;
    return DscpNode;
}

/**********************************************************************
    function:
        InsertDscpNode
    description:
        This function is called to Insert a new Dscp node to the DSCP tree.
    argument:
        pstDSCPInfo_t    DscpTree,       -  Dscp tree
        UINT             dscp,           -  Dscp value
    return:
        pstDSCPInfo_t
**********************************************************************/
static pstDSCPInfo_t InsertDscpNode(pstDSCPInfo_t DscpTree, UINT dscp)
{
    if (DscpTree == NULL)
        return NewNode(dscp);

    if(dscp == DscpTree->Dscp)
    {
        WTC_LOG_INFO("Dscp node already exists, Dscp = %d",DscpTree->Dscp);
        DscpTree->IsUpdated = TRUE;
        return DscpTree;
    }

    if (dscp < DscpTree->Dscp)
        DscpTree->Left = InsertDscpNode(DscpTree->Left, dscp);
    else if (dscp > DscpTree->Dscp)
        DscpTree->Right = InsertDscpNode(DscpTree->Right, dscp);
    return DscpTree;
}

/**********************************************************************
    function:
        ResetIsUpdatedFlag
    description:
        This function is called to disable IsUpdated flag of every node in DscpTree.
    argument:
        pstDSCPInfo_t    DscpTree,       -  Dscp tree
    return:
        pstDSCPInfo_t
**********************************************************************/
pstDSCPInfo_t ResetIsUpdatedFlag(pstDSCPInfo_t DscpTree)
{
    if (DscpTree == NULL)
        return NULL;
    ResetIsUpdatedFlag(DscpTree->Left);
    ResetIsUpdatedFlag(DscpTree->Right);
    DscpTree->IsUpdated = FALSE;
    for (UINT i=0; i<DscpTree->NumClients; i++)
    {
        DscpTree->ClientList[i].IsUpdated = FALSE;
    }
    return DscpTree;
}

/**********************************************************************
    function:
        UpdateDscpCount
    description:
        This function is called to update the EnabledDscpCount value.
    argument:
        CHAR*           Enabled_DSCP_List,    - Dscp string
        pstDSCPInfo_t   DscpTree,             - DscpTree
    return:
        INT
**********************************************************************/
pstDSCPInfo_t UpdateDscpCount(CHAR* Enabled_DSCP_List, pstDSCPInfo_t DscpTree)
{
    INT count = 0, dscp;
    CHAR *token = strtok(Enabled_DSCP_List, ",");

    ResetIsUpdatedFlag(DscpTree);
    while (token != NULL)
    {
        count++;
        dscp = atoi(token);
        DscpTree = InsertDscpNode(DscpTree, dscp);
        token = strtok(NULL, ",");
    }
    EnabledDscpCount = count;
    DscpTree =  DeleteDisabledDscp(DscpTree);
    return DscpTree;
}

/**********************************************************************
    function:
        DeleteDscpTree
    description:
        This function is called to delete the DscpTree.
    argument:
        pstDSCPInfo_t    DscpTree       -  Dscp tree
    return:
        pstDSCPInfo_t
**********************************************************************/
pstDSCPInfo_t DeleteDscpTree(pstDSCPInfo_t DscpTree)
{
    if (DscpTree == NULL)
        return DscpTree;
    else if (DscpTree->Left != NULL)
        DeleteDscpTree(DscpTree->Left);
    else if (DscpTree->Right != NULL)
        DeleteDscpTree(DscpTree->Right);
    else
    {
        if (DscpTree->ClientList != NULL)
            free(DscpTree->ClientList);
        free(DscpTree);
	/* CID: 279988  Use after free (USE_AFTER_FREE) */
	return NULL;
    }
    return DscpTree;
}

/**********************************************************************
    function:
        DeleteDisabledDscp
    description:
        This function is called to delete the disabled Dscp nodes.
    argument:
        pstDSCPInfo_t    DscpTree       -  Dscp tree
    return:
        pstDSCPInfo_t
**********************************************************************/
static pstDSCPInfo_t DeleteDisabledDscp(pstDSCPInfo_t DscpTree)
{
    if (DscpTree == NULL)
    {
        WTC_LOG_INFO("DscpTree NULL");
        return NULL;
    }

    DscpTree->Left = DeleteDisabledDscp(DscpTree->Left);
    DscpTree->Right = DeleteDisabledDscp(DscpTree->Right);
    if (DscpTree->IsUpdated == FALSE)
    {
        pstDSCPInfo_t temp;
        if (DscpTree->Left == NULL)
        {
            temp = DscpTree->Right;
        }
        else if (DscpTree->Right == NULL)
        {
            temp = DscpTree->Left;
        }
        else
        {
            pstDSCPInfo_t temparent = DscpTree;
            temp = DscpTree->Right;
            if (temp->Left)
            {
               while (temparent=temp, temp = temp->Left, temp->Left);
               temparent->Left = temp->Right;
               temp->Right = DscpTree->Right;
            }
            temp->Left = DscpTree->Left;
        }
        free(DscpTree);
        DscpTree = NULL;
        EnabledDscpCount--;
        return temp;
    }
    return DscpTree;
}

/**********************************************************************
    function:
        DeleteDisabledClients
    description:
        This function is called to delete the disconnected clients from DscpNode
    argument:
        pstDSCPInfo_t    DscpTree   -  Dscp tree
    return:
        pstDSCPInfo_t
**********************************************************************/
static pstDSCPInfo_t DeleteDisabledClients(pstDSCPInfo_t DscpTree)
{
    UINT index = DscpTree->NumClients;
    errno_t rc = -1;
    for(UINT i=0; i<index; i++)
    {
        if (DscpTree->ClientList[i].IsUpdated == FALSE)
        {
            rc = memcpy_s(&DscpTree->ClientList[i], sizeof(stClientInfo_t),
                          &DscpTree->ClientList[index-1], sizeof(stClientInfo_t));
            ERR_CHK(rc);
            rc = memset_s(&DscpTree->ClientList[index-1], sizeof(stClientInfo_t), 0,
                          sizeof(stClientInfo_t));
            ERR_CHK(rc);
            i--;
            DscpTree->NumClients--;
            index = DscpTree->NumClients;
        }
    }
    return DscpTree;
}

/**********************************************************************
    function:
        InsertClient
    description:
        This function is called to get the Wan traffic counts.
    argument:
        pstDSCPInfo_t    DscpTree   -  Dscp tree
        pDSCP_list_t      CliList    -  Incoming Client list from HAL
    return:
        pstDSCPInfo_t
**********************************************************************/
pstDSCPInfo_t InsertClient(pstDSCPInfo_t DscpTree, pDSCP_list_t CliList)
{
    if ( (DscpTree != NULL) && (CliList != NULL) )
    {
        for(UINT k=0; k<CliList->numElements; k++)
        {
            if(DscpTree->Dscp == CliList->DSCP_Element[k].dscp_value)
            {
                UINT count = 0;
                UINT dscpIndex = DscpTree->NumClients;
                UINT cliIndex = CliList->DSCP_Element[k].numClients;

                if( (cliIndex == 0) && (dscpIndex > 0) )
                {
                    //No clients in new incoming and but had few clients in the past iterations
                   free(DscpTree->ClientList);
                   DscpTree->ClientList = NULL;
                }

                if(cliIndex > 0)
                {
                    if( (((DscpTree->MemorySlab-cliIndex)/CLIENT_ALLOC_SLAB)>0) || (dscpIndex==0) )
                    {
                        DscpTree->MemorySlab = SetMemoryslab(cliIndex);
                        DscpTree->ClientList = (stClientInfo_t *) realloc (DscpTree->ClientList,
                                                DscpTree->MemorySlab * sizeof(stClientInfo_t));
                        if (DscpTree->ClientList == NULL)
                        {
                            WTC_LOG_ERROR("Realloc failure.");
                            return DscpTree;
                        }
                        errno_t rc = -1;
                        rc = memset_s(DscpTree->ClientList,
                                      DscpTree->MemorySlab * sizeof(stClientInfo_t),
                                      0, DscpTree->MemorySlab * sizeof(stClientInfo_t));
                        ERR_CHK(rc);
                    }

                    for(UINT i=0; i<cliIndex; i++)
                    {
                        UINT j;
                        for(j=0; j<dscpIndex; j++)
                        {
                            errno_t rc = -1;
                            INT ind = -1;
                            rc = strcmp_s(DscpTree->ClientList[j].Mac,
                                          strlen(DscpTree->ClientList[j].Mac),
                                          CliList->DSCP_Element[k].Client[i].mac, &ind);
                            ERR_CHK(rc);
                            //Update existing client
                            if ((rc == EOK) && (!ind))
                            {
                                DscpTree->ClientList[j].RxBytes =
                                                CliList->DSCP_Element[k].Client[i].rxBytes -
                                                DscpTree->ClientList[j].RxBytesTot;
                                DscpTree->ClientList[j].TxBytes =
                                                CliList->DSCP_Element[k].Client[i].txBytes -
                                                DscpTree->ClientList[j].TxBytesTot;
                                DscpTree->ClientList[j].RxBytesTot =
                                                CliList->DSCP_Element[k].Client[i].rxBytes;
                                DscpTree->ClientList[j].TxBytesTot =
                                                CliList->DSCP_Element[k].Client[i].txBytes;
                                DscpTree->ClientList[j].IsUpdated = TRUE;
                                DscpTree->IsUpdated = TRUE;
                                count++;
                                WTC_LOG_INFO("Mac = %s, rx = %lu, tx = %lu,"
                                             "rxTot = %lu, txTot = %lu, Count = %d",
                                                   DscpTree->ClientList[j].Mac,
                                                   DscpTree->ClientList[j].RxBytes,
                                                   DscpTree->ClientList[j].TxBytes,
                                                   DscpTree->ClientList[j].RxBytesTot,
                                                   DscpTree->ClientList[j].TxBytesTot, count);
                                break;
                            }
                        }
                        //New Client added
                        if (j == dscpIndex)
                        {
                            memcpy(DscpTree->ClientList[j].Mac,
                                   CliList->DSCP_Element[k].Client[i].mac,
                                   sizeof(CliList->DSCP_Element[k].Client[i].mac));
                            DscpTree->ClientList[j].RxBytes =
                                        CliList->DSCP_Element[k].Client[i].rxBytes;
                            DscpTree->ClientList[j].TxBytes =
                                        CliList->DSCP_Element[k].Client[i].txBytes;
                            DscpTree->ClientList[j].RxBytesTot =
                                        CliList->DSCP_Element[k].Client[i].rxBytes;
                            DscpTree->ClientList[j].TxBytesTot =
                                        CliList->DSCP_Element[k].Client[i].txBytes;
                            DscpTree->ClientList[j].IsUpdated = TRUE;
                            DscpTree->NumClients++;
                            DscpTree->IsUpdated = TRUE;
                            dscpIndex = DscpTree->NumClients;
                            WTC_LOG_INFO("j = %d, Dscp = %d, MAC = %s, RxBytes = %lu,"
                                         "TxBytes = %lu, RxBytesTot = %lu, TxBytesTot = %lu,"
                                         "Is_Updated = %d",
                                              j, DscpTree->Dscp,
                                              DscpTree->ClientList[j].Mac,
                                              DscpTree->ClientList[j].RxBytes,
                                              DscpTree->ClientList[j].TxBytes,
                                              DscpTree->ClientList[j].RxBytesTot,
                                              DscpTree->ClientList[j].TxBytesTot,
                                              DscpTree->IsUpdated);
                        }
                    }

                    if (dscpIndex > cliIndex)
                    {
                        DeleteDisabledClients(DscpTree);
                    }
                }
            }
        }
        DscpTree->Left = InsertClient(DscpTree->Left, CliList);
        DscpTree->Right = InsertClient(DscpTree->Right, CliList);
    }
    else if(DscpTree != NULL)
    {
        WTC_LOG_INFO("CliList is NULL");
    }
    return DscpTree;
}
