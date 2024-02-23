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
#include <sys/file.h>

extern LmObjectHosts lmHosts;
static unsigned int countlines(char*);
static void lock_clients_file(void);
static void unlock_clients_file(void);
#define DHCP_VENDOR_CLIENTS_LOCK "/tmp/.dhcp_vendor_clients_lock"
static int lock_fd;

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
                
                if (strcmp(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId], value) == 0)
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
static unsigned int countlines(char *filename)
{

    FILE *fp = NULL;
    char ch = '\0';
    unsigned int lines = 0;
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
            if (strcmp(lmHosts.hostArray[i]->pStringParaValue[LM_HOST_PhysAddressId], macaddr) == 0)
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
    PCOSA_DML_MANG_DEV pMangDevTable;
    unsigned int MangDevIdx = 0;
    unsigned int lines;
    FILE *fp;

    lines = countlines(filename);
    if (lines == 0)
    {
        *tableEntryCount = 0;
        return NULL;
    }

    pMangDevTable = AnscAllocateMemory(sizeof(COSA_DML_MANG_DEV) * lines);
    if (pMangDevTable == NULL)
    {
        *tableEntryCount = 0;
        return NULL;
    }

    fp = fopen(filename, "r");
    if (fp)
    {
        char lineBuf[17 + 1 + 6 + 1 + 64 + 1 + 64 + 1 + 1 + 16];

        while (fgets(lineBuf, sizeof(lineBuf), fp) != NULL)
        {
            char macAddrStr[MANG_DEV_MAC_STR_LEN + 1];
            char manufacturerOUI[MANG_DEV_MANUFACTURER_OUI_STR_LEN + 1];
            char serialNumber[MANG_DEV_SERIAL_NUMBER_STR_LEN + 1];
            char productClass[MANG_DEV_PRODUCT_CLASS_STR_LEN + 1];
            int arguNum;

            arguNum = sscanf(lineBuf, "%17[^;];%6[^;];%64[^;];%64s ", macAddrStr, manufacturerOUI, serialNumber, productClass);

            /* ProductClass is optional */
            if ((arguNum == 3) || (arguNum == 4))
            {
                size_t len_macAddrStr = strlen(macAddrStr);
                size_t len_manufacturerOUI = strlen(manufacturerOUI);
                size_t len_serialNumber = strlen(serialNumber);

                if ((len_macAddrStr == MANG_DEV_MAC_STR_LEN) &&
                    (len_manufacturerOUI == MANG_DEV_MANUFACTURER_OUI_STR_LEN) &&
                    (len_serialNumber > 0) && (len_serialNumber < MANG_DEV_SERIAL_NUMBER_STR_LEN))
                {
                    // TODO: This check is commented because LeaseTime is NOT currently updated in hostArray[] for DHCPv6 clients... hence IsLeaseAvailable(macAddrStr) check will return false for DHCPv6 LAN clients
                    // if (IsLeaseAvailable(macAddrStr))
                    {
                        memcpy(pMangDevTable[MangDevIdx].MacAddr, macAddrStr, len_macAddrStr + 1);
                        memcpy(pMangDevTable[MangDevIdx].ManufacturerOUI, manufacturerOUI, len_manufacturerOUI + 1);
                        memcpy(pMangDevTable[MangDevIdx].SerialNumber, serialNumber, len_serialNumber + 1);
                        snprintf(pMangDevTable[MangDevIdx].ProductClass, sizeof(pMangDevTable[MangDevIdx].ProductClass), "%s", (arguNum == 4) ? productClass : "");
                        MangDevIdx++;
                    }
                }
            }
        }

        fclose(fp);
    }

    *tableEntryCount = MangDevIdx;

    return pMangDevTable;
}

static void lock_clients_file(void)
{
  lock_fd = open(DHCP_VENDOR_CLIENTS_LOCK, O_RDONLY | O_CREAT, 0666);
  if (lock_fd < 0) {
    fprintf(stderr,"Can't open/create file %s",  errno, DHCP_VENDOR_CLIENTS_LOCK);
    return;
  }

  if (flock(lock_fd, LOCK_EX) < 0) {
    fprintf(stderr,"Error %d closing file %s'",  errno, DHCP_VENDOR_CLIENTS_LOCK);
    return;
  }
  return;
}

static void unlock_clients_file(void)
{
  if (flock(lock_fd, LOCK_UN) < 0)
  {
    fprintf(stderr,"Error %d open/create file %s",  errno, DHCP_VENDOR_CLIENTS_LOCK);
  }
  if (close(lock_fd) < 0)
  {
    fprintf(stderr,"Error %d closing file %s",  errno, DHCP_VENDOR_CLIENTS_LOCK);
  }
  return;
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
    FILE *fpv6 = NULL;
    FILE *fp = NULL;
    char buffer[MAX_BUFFER_SIZE]="";

    fp = fopen(DHCP_VENDOR_CLIENT_ALL_PATH,"w");
    if(fp)
    {
        lock_clients_file();
        fpv4 = fopen(DHCP_VENDOR_CLIENT_V4_PATH,"r");
        if (fpv4)
        {
            while ( fgets(buffer, sizeof(buffer), fpv4) != NULL )
            {
                fprintf(fp,"%s",buffer);
            }
            fclose(fpv4);
        }
        unlock_clients_file();

        fpv6 = fopen(DHCP_VENDOR_CLIENT_V6_PATH,"r");
        if (fpv6)
        {
            while ( fgets(buffer, sizeof(buffer), fpv6) != NULL )
            {
                fprintf(fp,"%s",buffer);
            }
            fclose(fpv6);
        }
        if(fp)
        fclose(fp);
    }
    return;
}

static BOOL parseXMLTag(char *buf, char *tag, char *tagValue)
{
    char *ptr = NULL;
    char tagLeft[MAX_BUFFER_SIZE] = {0};
    _ansc_strcat(tagLeft, "<");
    _ansc_strcat(tagLeft, tag);

    ptr = _ansc_strstr(buf, tagLeft);
    if( ptr == NULL )
    {
        return FALSE;
    }
    else
    {
        ptr = _ansc_strstr(ptr, ">");
       if( ptr == NULL )
        {
            return FALSE;
        }
        else
        {
              buf = ptr + 1;
        }
    }

    char tagRight[MAX_BUFFER_SIZE]= {0};
    sprintf(tagRight, "</%s>", tag);

    ptr = _ansc_strstr(buf, tagRight);
    if( ptr == NULL )
    {
        return FALSE;
    }
    else
    {
        _ansc_memcpy(tagValue , buf , strlen(buf) - strlen(ptr));
    }

    return TRUE;
}


/*
 * Generates DHCP_VENDOR_CLIENT_V6_PATH using tmp/dibbler/server-AddrMgr.xml for DHCPv6 clients
 */
void vendorClientV6XMLParser(char *xmlFile)
{
    char buf[MAX_BUFFER_SIZE] = {0};
    int cnt = 0;
    BOOL newClient = FALSE;
    BOOL gotAddr = FALSE;
    char cmd[MAX_BUFFER_SIZE] = {0};
    char respBuf[MAX_BUFFER_SIZE] = {0};
    char tagValue[MAX_BUFFER_SIZE] = {0};
    char vendor_mac[MANG_DEV_MAC_STR_LEN+1] = {0};
    char vendor_oui[MANG_DEV_MANUFACTURER_OUI_STR_LEN+1] = {0};
    char vendor_serial[MANG_DEV_SERIAL_NUMBER_STR_LEN+1] = {0};
    char vendor_class[MANG_DEV_PRODUCT_CLASS_STR_LEN+1] = {0};
    char *startPtr = NULL;
    char *endPtr = NULL;


    FILE *fp = fopen(xmlFile, "r");
    if(!fp)
    {
        return;
    }

    if (access(DHCP_VENDOR_CLIENT_V6_PATH, F_OK) == 0)
    {
        unlink(DHCP_VENDOR_CLIENT_V6_PATH);
    }

    while(!feof(fp))
    {
        _ansc_memset(buf, 0, MAX_BUFFER_SIZE);
        _ansc_memset(cmd, 0, MAX_BUFFER_SIZE);
        _ansc_memset(tagValue, 0, MAX_BUFFER_SIZE);
        if (fgets(buf, sizeof(buf), fp) != NULL)
        {

            if(_ansc_strstr(buf, TAG_STR_ADDCLIENT_START )!= 0 && !newClient ) //Look for <AddrClient>
            {
                newClient = TRUE;
                cnt++;
                continue;
            }
            if (newClient)
            {
                if (_ansc_strstr(buf, TAG_STR_ADDR_IA) != 0 && !gotAddr) //Look for AddrIA
                {
                    startPtr = _ansc_strstr(buf, "unicast=");
                    if (startPtr)
                    {
                        startPtr += strlen("unicast=") + 1;
                        endPtr = _ansc_strstr(startPtr, "\"");
                        if (endPtr)
                        {
                            *endPtr = '\0';
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        continue;
                    }
                    FILE *fp = NULL;

                    fp = v_secure_popen("r","ip neigh | grep %s | awk '{print $5}'", startPtr);
                    if(fp)
                    {
                        if(fgets(respBuf, sizeof(respBuf), fp)!=NULL)
                        {
                            respBuf[strlen(respBuf) - 1] = '\0';
                            strncpy(vendor_mac,respBuf,MANG_DEV_MAC_STR_LEN);
                        }
                        v_secure_pclose(fp);
                    }
                    gotAddr = TRUE;
                    continue;
                }
                if (parseXMLTag(buf, TAG_STR_MANUFACTUREROUI, tagValue) == TRUE)
                {
                    _ansc_strncpy(vendor_oui, tagValue, MANG_DEV_MANUFACTURER_OUI_STR_LEN);
                    continue;
                }

                if (parseXMLTag(buf, TAG_STR_SERIALNUMBER, tagValue) == TRUE)
                {
                    _ansc_strncpy(vendor_serial, tagValue, MANG_DEV_SERIAL_NUMBER_STR_LEN);
                    continue;
                }

                if (parseXMLTag(buf, TAG_STR_PRODUCTCLASS, tagValue) == TRUE)
                {
                    _ansc_strncpy(vendor_class, tagValue, MANG_DEV_PRODUCT_CLASS_STR_LEN);
                    continue;
                }

                if(_ansc_strstr(buf, TAG_STR_ADDCLIENT_END)!= 0 && newClient )
                {
                    newClient = FALSE;
                    gotAddr = FALSE;
                    /* product class is optional, it could be empty. */
                    if ((strlen(vendor_mac) == MANG_DEV_MAC_STR_LEN) &&
                       (strlen(vendor_oui) == MANG_DEV_MANUFACTURER_OUI_STR_LEN) &&
                       (strlen(vendor_serial) < MANG_DEV_SERIAL_NUMBER_STR_LEN) && (strlen(vendor_serial) > 0))
                    {
                        sprintf( cmd, "echo \"%s;%s;%s;%s\" >> %s", vendor_mac, vendor_oui, vendor_serial, vendor_class, DHCP_VENDOR_CLIENT_V6_PATH);
                        system(cmd);
                        //Below fails with error "redirection to variable is insecure"
                        //v_secure_system("echo %s;%s;%s;%s >> %s", vendor_mac, vendor_oui, vendor_serial, vendor_class, DHCP_VENDOR_CLIENT_V6_PATH);
                    }
                    _ansc_memset(vendor_mac, 0, MANG_DEV_MAC_STR_LEN+1);
                    _ansc_memset(vendor_oui, 0, MANG_DEV_MANUFACTURER_OUI_STR_LEN+1);
                    _ansc_memset(vendor_serial, 0, MANG_DEV_SERIAL_NUMBER_STR_LEN+1);
                    _ansc_memset(vendor_class, 0, MANG_DEV_PRODUCT_CLASS_STR_LEN+1);
                    continue;
                }
            }
        }
    }
    fclose(fp);

    return;
}
#endif
