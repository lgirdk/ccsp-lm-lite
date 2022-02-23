/*********************************************************************
 * Copyright 2017-2019 ARRIS Enterprises, LLC.
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
 **********************************************************************/
#if defined(DEVICE_GATEWAY_ASSOCIATION_FEATURE)
#include "cosa_managementserver_apis.h"
#include "lm_main.h"
#include <string.h>
extern LmObjectHosts lmHosts;
static ULONG countlines(char*);
/**********************************************************************
    function:
        CosaDmlGetHostPath
    description:
        This function is called to retrieve Host string from Hosts table
        based on the input MAC address.
    argument:   
                char*                       value,
                MAC Address
                char*                       hostPath,
                The Host string value from Hosts table;
                ULONG*                      hostPathSize
                The maximum length of Host string;
    return:     ANSC_STATUS_SUCCESS if succeeded;
                ANSC_STATUS_FAILURE if error.
**********************************************************************/
ANSC_STATUS CosaDmlGetHostPath(char *value, char *hostPath, ULONG hostPathSize)
{

    ULONG dmLen = 0;
    ULONG hostPathLen = 0;
    int i = 0;
    int retVal = ANSC_STATUS_SUCCESS;
    int array_size = 0;
    if ((hostPath != NULL) && (value != NULL))
    {
        /*  <TR-069 Device:2.11 Root Object definition>
        Comma-separated list (maximum list length 1024) of strings.
        Each list item MUST be the path name of a row in the Hosts.Host table.
        For example: Device.Hosts.Host.1,Device.Hosts.Host.5 */        
        array_size = lmHosts.numHost;
        if (array_size)
        { 
            for (i = 0; i < array_size; i++)
            {
                hostPathLen = strlen(hostPath);
                
                if ( AnscEqualString(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId],value,TRUE))
                {
                    dmLen = strlen(lmHosts.hostArray[i]->objectName);
                    
                    if ((hostPathLen + dmLen) < hostPathSize)
                    {
                        if (hostPathLen > 0)
                        {
                            _ansc_strcat(hostPath, ",");
                            hostPathLen += 1;
                        }
                        /* The objectName includes '.' at the end and it must be removed before mapping to Host parameter */
                    
                        strncat(hostPath,lmHosts.hostArray[i]->objectName,dmLen-1);
                        hostPathLen = hostPathLen + dmLen-1;
                    }
                    else
                    {
                        retVal= ANSC_STATUS_RESOURCES;
                        break;
                    }
                }
            }
            if ( retVal == ANSC_STATUS_SUCCESS )
            {
                hostPath[hostPathLen] = '\0';
            }           
        }
        else
        {
            return ANSC_STATUS_NOT_READY;
        }
    }
    else
    {
        retVal = ANSC_STATUS_FAILURE;
    }
    return retVal;

}
/**********************************************************************
    function:
        countlines
    description:
        This function is used to count the number of lines in the input file.
    argument:   
                char*                       filename,
                File path
    return:     0 if file processing failed
                lines in the input file.
**********************************************************************/
static ULONG countlines(char *filename)
{

    FILE *fp = NULL;
    char ch = '\0';
    ULONG lines = 0;
    fp = fopen(filename, "r");
    if (fp != NULL)
    {
        while (!feof(fp))
        {
            ch = fgetc(fp);
            if (ch == '\n')
            {
                lines++;
            }
        }
        fclose(fp);
    }
    return lines;

}
/**********************************************************************
    function:
        IsLeaseAvailable
    description:
        This function is checks of the lease availability of input mac.
    argument:   
                char*                       macaddr,
                MAC Address
    return:     0 if the lease expired
                1 if the lease is remaining.
**********************************************************************/
int IsLeaseAvailable(char* macaddr)
{

    int leaseAvailable = -1;
    int array_size = lmHosts.numHost;  
    int i = 0;
    
    if ( array_size && (macaddr != NULL) )
    {
        for (i = 0; i < array_size; i++)
        {     
            if ( AnscEqualString(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId],macaddr,TRUE) )
            {
                int leaseTimeRemaining = lmHosts.hostArray[i]->LeaseTime - time(NULL);
                if (leaseTimeRemaining > 0)
                {
                    leaseAvailable = 1;
                }
                else
                {
                    leaseAvailable = 0;
                }  
                break;
            }
        }
    }
    return leaseAvailable;
}
/**********************************************************************
    function:
        CosaDmlGetManageableDevices
    description:
        This function returns the current list of Managed devices
    argument:   
                char*                       filename,
                File path
                ULONG*                      tableEntryCount
                Number of Managed devices
    return:     NULL if file processing failed
                List of managed devices if processing is success
**********************************************************************/
PCOSA_DML_MANG_DEV CosaDmlGetManageableDevices(ULONG *tableEntryCount, char *filename)
{

    char macAddrStr[MANG_DEV_MAC_STR_LEN+1] = {0};
    char manufacturerOUI[MANG_DEV_MANUFACTURER_OUI_STR_LEN+1] = {0};
    char serialNumber[MANG_DEV_SERIAL_NUMBER_STR_LEN+1] = {0};
    char productClass[MANG_DEV_PRODUCT_CLASS_STR_LEN+1] = {0};
    char lineBuf[MAX_BUFFER_SIZE] = {0};
    PCOSA_DML_MANG_DEV  pMangDevTable = NULL;
    ULONG lines = 0;
    ULONG MangDevIdx = 0;
    int arguNum = 0; 
    
    lines = countlines(filename);
    if (lines)
    {
        pMangDevTable = AnscAllocateMemory(sizeof(COSA_DML_MANG_DEV) * lines);
    }
    
    if ((lines == 0) || (pMangDevTable == NULL))
    {
        return NULL;
    }
    FILE *fp = fopen(filename, "r");
    if (fp)
    {
        while (NULL != fgets(lineBuf, sizeof(lineBuf), fp))
        {
            arguNum = sscanf(lineBuf, "%17[^;];%6[^;];%64[^;];%64s ", macAddrStr, manufacturerOUI, serialNumber, productClass);
            /* ProductClass is optional. */
            if (((arguNum == PARSE_ARGU_NUM_4) || (arguNum == PARSE_ARGU_NUM_3)) &&
                (strlen(macAddrStr) == MANG_DEV_MAC_STR_LEN) && (strlen(manufacturerOUI) == MANG_DEV_MANUFACTURER_OUI_STR_LEN) &&
                (strlen(serialNumber) < MANG_DEV_SERIAL_NUMBER_STR_LEN) && (strlen(serialNumber) > 0))
            {
                if ( IsLeaseAvailable(macAddrStr) )
                {
                    _ansc_strncpy(pMangDevTable[MangDevIdx].ManufacturerOUI, manufacturerOUI, MANG_DEV_MANUFACTURER_OUI_STR_LEN);
                    _ansc_strncpy(pMangDevTable[MangDevIdx].SerialNumber, serialNumber, MANG_DEV_SERIAL_NUMBER_STR_LEN);
                    _ansc_strncpy(pMangDevTable[MangDevIdx].ProductClass, productClass, MANG_DEV_PRODUCT_CLASS_STR_LEN);
                    _ansc_strncpy(pMangDevTable[MangDevIdx].MacAddr, macAddrStr, MANG_DEV_MAC_STR_LEN);
                    MangDevIdx++;
                }
            }
            _ansc_memset(manufacturerOUI, 0, MANG_DEV_MANUFACTURER_OUI_STR_LEN+1);
            _ansc_memset(serialNumber, 0, MANG_DEV_SERIAL_NUMBER_STR_LEN+1);
            _ansc_memset(productClass, 0, MANG_DEV_PRODUCT_CLASS_STR_LEN+1);
            _ansc_memset(macAddrStr, 0, MANG_DEV_MAC_STR_LEN+1);
            _ansc_memset(lineBuf, 0, sizeof(lineBuf));
        }
        *tableEntryCount = MangDevIdx;
        fclose(fp);
    }
    
    return pMangDevTable;

}
/**********************************************************************
    function:
        buildDhcpVendorClientsFile
    description:
        This function creates a file with the list of IPv4 
        and IPv6 managed devices using the dhcp_vendor_clients_v6.txt
        and dhcp_vendor_clients.txt.
    argument:   
        None
    return:     
        void
**********************************************************************/
void buildDhcpVendorClientsFile()
{
    FILE *fpv4 = NULL;
    FILE *fp = NULL;
    char buffer[MAX_BUFFER_SIZE]="";
    fpv4 = fopen(DHCP_VENDOR_CLIENT_V4_PATH,"r");
    if ( fpv4 )
    {
        fp = fopen(DHCP_VENDOR_CLIENT_ALL_PATH,"w");
        if ( fp )
        {
            while ( fgets(buffer, sizeof(buffer), fpv4) != NULL )
            {
                fprintf(fp,"%s",buffer);
            }
            fclose(fp);
        }
        fclose(fpv4);
    }
    return;

}

#endif
