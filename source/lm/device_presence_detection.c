/*
* If not stated otherwise in this file or this component's LICENSE file the
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

#define _GNU_SOURCE
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include "ansc_platform.h"
#include <string.h> 
#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_RAW, INET_ADDRSTRLEN
#include <netinet/ip.h>       // IP_MAXPACKET (which is 65535)
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <net/if.h>           // struct ifreq
#include <net/ethernet.h>
#include <linux/if_ether.h>   // ETH_P_ARP = 0x0806
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <pthread.h> 
#include <errno.h>            // errno, perror()
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <mqueue.h>
#include "device_presence_detection.h"
#include "lm_util.h"
#include "syscfg/syscfg.h"
#include "safec_lib_common.h"
#include "secure_wrapper.h"

#define MAX_NUM_OF_DEVICE 200
#define MAX_SIZE    512

#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            return; \
        } \
    } while (0) \

#define MSG_TYPE_DNS_PRESENCE 7 
#define DNSMASQ_PRESENCE_QUEUE_NAME  "/presence_queue"

typedef struct NDSNotifyInfo
{
    int32_t enable;
    char interface[64];
}NDSNotifyInfo;

#define WR_VALUE _IOW('a','a',NDSNotifyInfo*)
#define RD_VALUE _IOR('a','b',NDSNotifyInfo*)

PLmDevicePresenceDetectionInfo pDetectionObject = NULL;
pthread_mutex_t PresenceDetectionMutex;
int Neighbourdiscovery_Update(BOOL enable)
{
    int ret = 0;
    char buf[64];
    int fd;
    NDSNotifyInfo input = { 0 };
    NDSNotifyInfo output = { 0 };
    errno_t rc = -1;
    
    printf("\nOpening Driver\n");
    fd = open("/dev/etx_device", O_RDWR);
    if(fd < 0) {
        printf("Cannot open device file...\n");
        return -1;
    }

    printf("Writing Value to Driver\n");
    input.enable = 0;
    if (enable)
    {
        input.enable = 1;
    }
    syscfg_get( NULL, "lan_ifname", buf, sizeof(buf));        
    rc = strcpy_s(input.interface, sizeof(input.interface), buf);
    if(rc != EOK)
    {
        ERR_CHK(rc);
        close(fd);
        return -1;
    }
    /*CID: 68224 Unchecked return value*/
    if (-1 == ioctl(fd, WR_VALUE, (NDSNotifyInfo*) &input))
    {
	    printf("%s ioctl write error\n", __FUNCTION__);
            close(fd);
	    return -1;
    }

    printf("Reading Value from Driver\n");
    /*CID: 68224 Unchecked return value*/
    if (-1 == ioctl(fd, RD_VALUE, (NDSNotifyInfo*) &output))
    {
	    printf("%s ioctl read error\n", __FUNCTION__);
            close(fd);
	    return -1;
    }
    printf("Value is %d\n", output.enable);

    printf("Closing Driver\n");
    close(fd);

    return ret;
}

int PresenceDetection_Init()
{
    int ret = 0;

    pDetectionObject =  AnscAllocateMemory(sizeof(LmDevicePresenceDetectionInfo));
    if (pDetectionObject)
    {        
        pDetectionObject->ppdevlist = AnscAllocateMemory(MAX_NUM_OF_DEVICE * sizeof(PLmPresenceDeviceInfo));
        if (!pDetectionObject->ppdevlist)
        {   AnscFreeMemory(pDetectionObject);
            pDetectionObject = NULL;
            return -1;
        }
        pthread_mutex_init(&PresenceDetectionMutex,0);
    }
    else
    {   
        return -1;
    }
    Neighbourdiscovery_Update(TRUE);
    return ret;        
}

int PresenceDetection_DeInit()
{
    int ret = 0;
    pthread_mutex_lock(&PresenceDetectionMutex);
    if (pDetectionObject)
    {
        int index = 0;
        if (pDetectionObject->ppdevlist)
        {
            for (index = 0; index < pDetectionObject->numOfDevice; ++index)
            {
                if (pDetectionObject->ppdevlist[index])
                {
                    AnscFreeMemory(pDetectionObject->ppdevlist[index]);
                    pDetectionObject->ppdevlist[index] = NULL;
                }
            }    
            AnscFreeMemory(pDetectionObject->ppdevlist);
            pDetectionObject->ppdevlist = NULL;
        } 
        AnscFreeMemory(pDetectionObject);
        pDetectionObject = NULL;
    }
    pthread_mutex_unlock(&PresenceDetectionMutex);
    Neighbourdiscovery_Update(FALSE);
    return ret;
}


PLmDevicePresenceDetectionInfo GetPresenceDetectionObject()
{
    return pDetectionObject;    
}

int Register_PresenceClbk(DEVICE_PRESENCE_DETECTION_FUNC clbkfunc)
{
    int ret_val = -1;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pthread_mutex_lock(&PresenceDetectionMutex);
    pobject = GetPresenceDetectionObject();
    if (pobject && clbkfunc)
    {
        pobject->clbk = clbkfunc;
        ret_val = 0;
    }
    pthread_mutex_unlock(&PresenceDetectionMutex);
    return ret_val;
}

int PresenceDetection_set_ipv4leaveinterval (unsigned int val)
{
    int ret_val = -1;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pthread_mutex_lock(&PresenceDetectionMutex);
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        pobject->ipv4_leave_detection_interval = val;
        ret_val = 0;
    }
    pthread_mutex_unlock(&PresenceDetectionMutex);
    return ret_val;
}

int PresenceDetection_set_ipv6leaveinterval (unsigned int val)
{
    int ret_val = -1;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pthread_mutex_lock(&PresenceDetectionMutex);
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        pobject->ipv6_leave_detection_interval = val;
        ret_val = 0;
    }
    pthread_mutex_unlock(&PresenceDetectionMutex);
    return ret_val;
}

int PresenceDetection_set_bkgndjoininterval (unsigned int val)
{
    int ret_val = -1;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pthread_mutex_lock(&PresenceDetectionMutex);
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        pobject->bkgnd_join_detection_interval = val;
        ret_val = 0;
    }
    pthread_mutex_unlock(&PresenceDetectionMutex);
    return ret_val;
}

int PresenceDetection_set_ipv4retrycount (unsigned int val)
{
    int ret_val = -1;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pthread_mutex_lock(&PresenceDetectionMutex);
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        pobject->ipv4_num_retries = val;
        ret_val = 0;
    }
    pthread_mutex_unlock(&PresenceDetectionMutex);
    return ret_val;
}

int PresenceDetection_set_ipv6retrycount (unsigned int val)
{
    int ret_val = -1;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pthread_mutex_lock(&PresenceDetectionMutex);
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        pobject->ipv6_num_retries = val;
        ret_val = 0;
    }    
    pthread_mutex_unlock(&PresenceDetectionMutex);
    return ret_val;
}

PLmPresenceDeviceInfo FindDeviceByPhysAddress(char * physAddress)
{
    int i = 0;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        for(i = 0; i<pobject->numOfDevice; i++)
        {
            if (pobject->ppdevlist && pobject->ppdevlist[i])
            {
                if (strcasecmp(pobject->ppdevlist[i]->mac, physAddress) == 0)
                {
                    return pobject->ppdevlist[i];
                }
            }
        }
    }
    return NULL;
}


int PresenceDetection_AddDevice(LmPresenceDeviceInfo *pinfo)
{
    int ret_val = -1;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    PLmPresenceDeviceInfo pDev = NULL;
    if (!pinfo)
        return ret_val;
    pthread_mutex_lock(&PresenceDetectionMutex);
    pDev = FindDeviceByPhysAddress(pinfo->mac);    
    if (pDev)
    {
        CcspTraceDebug(("RDKB_PRESENCE: Mac %s already exist !!! \n",pinfo->mac));
        if (pinfo->ipv4Active && (!pDev->ipv4Active))
        {
            pDev->ipv4Active = pinfo->ipv4Active;
            strncpy(pDev->ipv4,pinfo->ipv4,sizeof(pDev->ipv4));
        }
        if (pinfo->ipv6Active && (!pDev->ipv6Active))
        {
            pDev->ipv6Active = pinfo->ipv6Active;        
            strncpy(pDev->ipv6,pinfo->ipv6,sizeof(pDev->ipv6));
        }
        pthread_mutex_unlock(&PresenceDetectionMutex);
        return 0;
    }
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        if (pobject->numOfDevice < MAX_NUM_OF_DEVICE)
        {
            if (pobject->ppdevlist)
            {
                pDev =  AnscAllocateMemory(sizeof(LmPresenceDeviceInfo));
                if (pDev)
                {
                    memcpy (pDev,pinfo,sizeof(LmPresenceDeviceInfo));
                    pDev->ipv4_state = STATE_PRESENCE_DETECTION_NONE;
                    pDev->ipv6_state = STATE_PRESENCE_DETECTION_NONE;
                    pDev->ipv4_retry_count = 0;
                    pDev->ipv6_retry_count = 0;
                    pobject->ppdevlist[pobject->numOfDevice] = pDev;
                    CcspTraceWarning(("RDKB_PRESENCE:  Mac %s Added into detection list \n",pDev->mac));
                     ++pobject->numOfDevice;
                    ret_val = 0;                
                }
            }
        }
    }
    pthread_mutex_unlock(&PresenceDetectionMutex);
    return ret_val;
}

int PresenceDetection_RemoveDevice(char *mac)
{
    int ret_val = -1;
    int index = 0;
    int i = 0;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    PLmPresenceDeviceInfo pDev = NULL;
    if (!mac)
        return ret_val;
    pthread_mutex_lock(&PresenceDetectionMutex);
    pobject = GetPresenceDetectionObject();
    if (!pobject) {      
        /*CID: 135540 Missing unlock*/ 
        pthread_mutex_unlock(&PresenceDetectionMutex);
        return ret_val;
    }
    for(i = 0; i<pobject->numOfDevice; i++)
    {
        if (pobject->ppdevlist && pobject->ppdevlist[i])
        {
            if (strcasecmp(pobject->ppdevlist[i]->mac, mac) == 0)
            {
                pDev = pobject->ppdevlist[i];
                pobject->ppdevlist[i] = NULL;
                index = i;
                break;
            }
        }
    }
    if (!pDev)
    {
        CcspTraceWarning(("RDKB_PRESENCE:  Mac %s Not exist in detection list \n",mac));
        pthread_mutex_unlock(&PresenceDetectionMutex);
        return 0;
    }

    AnscFreeMemory(pDev);
    pDev = NULL;
    CcspTraceWarning(("RDKB_PRESENCE:  Mac %s Removed from monitor \n",mac));
    if (pobject && (pobject->numOfDevice > 0))
    {
        // reshuffle the list
        for(i = index; i<pobject->numOfDevice - 1; i++)
        {
            if (pobject->ppdevlist)
            {
                if (pobject->ppdevlist[i+1])
                {
                    pobject->ppdevlist[i] = pobject->ppdevlist[i+1];
                }
            }
        }
         pobject->ppdevlist[pobject->numOfDevice - 1] = NULL;
        --pobject->numOfDevice;
    }

    pthread_mutex_unlock(&PresenceDetectionMutex);
    return ret_val;
}

int sendIpv4ArpMessage(PLmDevicePresenceDetectionInfo pobject,BOOL bactiveclient)
{
    int i, status, frame_length, sd, bytes;
    char *interface, *target, *src_ip;
    arp_hdr arphdr;
    uint8_t *src_mac, *dst_mac, *ether_frame;
    struct addrinfo hints, *res;
    struct sockaddr_in *ipv4;
    struct sockaddr_ll device;
    struct ifreq ifr;
    char buf[64];
    errno_t rc = -1;

    if (pobject)
    {
        for(i = 0; i<pobject->numOfDevice; i++)
        {
            if (pobject->ppdevlist && pobject->ppdevlist[i])
            {
                PLmPresenceDeviceInfo obj = pobject->ppdevlist[i];
                if ((!obj->ipv4Active) || (obj->currentActive != bactiveclient))
                    continue;
                ++obj->ipv4_retry_count;
                if (obj->ipv4_retry_count > pobject->ipv4_num_retries)
                {
                    obj->ipv4_state = STATE_LEAVE_DETECTED;
                    if (obj->ipv6Active && (obj->ipv6_state != STATE_LEAVE_DETECTED))
                    {
                        continue;
                    }
                    obj->ipv4_retry_count = 0;
                    obj->currentActive = FALSE;
                    // trigger leave callback
                    if (pobject->clbk)
                    {
                        pobject->clbk(obj);
                    }
                    continue;
                }               

                /* In IPV4 only case, To identify accurate presence leave 
                 * reset IPV4 status and remove IPV4 entry from ARP.
                 * If device is in connected state for ipv4 case, ARP will updated again.
                 * otherwise this device will be in-active.
                 */
                if (obj->currentActive)
                {
                    if (obj->ipv4Active && (!obj->ipv6Active))
                    {
                        int ret =0;
                        char buf1[64];
                        syscfg_get(NULL, "lan_ifname", buf1, sizeof(buf1));
                        ret = v_secure_system("ip neigh del %s dev %s",obj->ipv4,buf1);
                        if(ret !=0)
                        {
                             CcspTraceDebug(("Failed in executing the command via v_secure_system ret: %d \n",ret));
                        }

                    }
                }
 
                // Allocate memory for various arrays.
                src_mac = allocate_ustrmem (6);
                dst_mac = allocate_ustrmem (6);
                ether_frame = allocate_ustrmem (IP_MAXPACKET);
                interface = allocate_strmem (40);
                target = allocate_strmem (40);
                src_ip = allocate_strmem (INET_ADDRSTRLEN);

                syscfg_get( NULL, "lan_ifname", buf, sizeof(buf));        
                // Interface to send packet through.
                rc = strcpy_s(interface, 40, buf);
                ERR_CHK(rc);

                // int cnt = 0;
                // for(cnt = 0; cnt < nPresenceDev;cnt++)
                // {
                // Submit request for a socket descriptor to look up interface.               
                if ((sd = socket (AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
                    perror ("socket() failed to get socket descriptor for using ioctl() ");
                    exit (EXIT_FAILURE);
                }

                // Use ioctl() to look up interface name and get its MAC address.
                memset (&ifr, 0, sizeof (ifr));
                snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
                if (ioctl (sd, SIOCGIFHWADDR, &ifr) < 0) {
                    perror ("ioctl() failed to get source MAC address ");
                    //return (EXIT_FAILURE);
                }
                close (sd);

                // Copy source MAC address.
                memcpy (src_mac, ifr.ifr_hwaddr.sa_data, 6 * sizeof (uint8_t));

                // Report source MAC address to stdout.
                printf ("MAC address for interface %s is ", interface);
                for (i=0; i<5; i++) {
                    printf ("%02x:", src_mac[i]);
                }
                printf ("%02x\n", src_mac[5]);

                // Find interface index from interface name and store index in
                // struct sockaddr_ll device, which will be used as an argument of sendto().
                memset (&device, 0, sizeof (device));
                if ((device.sll_ifindex = if_nametoindex (interface)) == 0) {
                    perror ("if_nametoindex() failed to obtain interface index ");
                    exit (EXIT_FAILURE);
                }
                printf ("Index for interface %s is %i\n", interface, device.sll_ifindex);

                // Set destination MAC address: broadcast address
                memset (dst_mac, 0xff, 6 * sizeof (uint8_t));

                // Source IPv4 address:  you need to fill this out
                syscfg_get( NULL, "lan_ipaddr", buf, sizeof(buf));        
                rc = strcpy_s (src_ip, INET_ADDRSTRLEN ,buf);
                ERR_CHK(rc);

                // Destination URL or IPv4 address (must be a link-local node): you need to fill this out
                //strcpy (target, "10.0.0.126");
                rc = strcpy_s(target, 40,obj->ipv4);
                ERR_CHK(rc);

                // Fill out hints for getaddrinfo().
                memset (&hints, 0, sizeof (struct addrinfo));
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_flags = hints.ai_flags | AI_CANONNAME;

                // Source IP address
                /*CID:67299 Argument cannot be negative*/
                if ((status = inet_pton (AF_INET, src_ip, &arphdr.sender_ip)) != 1) {
                    fprintf (stderr, "inet_pton() failed for source IP address.\nError message: %d - %s", status, strerror(errno));
                    exit (EXIT_FAILURE);
                }

                // Resolve target using getaddrinfo().
                if ((status = getaddrinfo (target, NULL, &hints, &res)) != 0) {
                    fprintf (stderr, "getaddrinfo() failed: %s\n", gai_strerror (status));
                    exit (EXIT_FAILURE);
                }
                ipv4 = (struct sockaddr_in *) res->ai_addr;
                memcpy (&arphdr.target_ip, &ipv4->sin_addr, 4 * sizeof (uint8_t));
                freeaddrinfo (res);

                // Fill out sockaddr_ll.
                device.sll_family = AF_PACKET;
                memcpy (device.sll_addr, src_mac, 6 * sizeof (uint8_t));
                device.sll_halen = 6;

                // ARP header

                // Hardware type (16 bits): 1 for ethernet
                arphdr.htype = htons (1);

                // Protocol type (16 bits): 2048 for IP
                arphdr.ptype = htons (ETH_P_IP);

                // Hardware address length (8 bits): 6 bytes for MAC address
                arphdr.hlen = 6;

                // Protocol address length (8 bits): 4 bytes for IPv4 address
                arphdr.plen = 4;

                // OpCode: 1 for ARP request
                arphdr.opcode = htons (ARPOP_REQUEST);

                // Sender hardware address (48 bits): MAC address
                memcpy (&arphdr.sender_mac, src_mac, 6 * sizeof (uint8_t));

                // Sender protocol address (32 bits)
                // See getaddrinfo() resolution of src_ip.

                // Target hardware address (48 bits): zero, since we don't know it yet.
                memset (&arphdr.target_mac, 0, 6 * sizeof (uint8_t));

                // Target protocol address (32 bits)
                // See getaddrinfo() resolution of target.

                // Fill out ethernet frame header.

                // Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (ARP header)
                frame_length = 6 + 6 + 2 + ARP_HDRLEN;

                // Destination and Source MAC addresses
                memcpy (ether_frame, dst_mac, 6 * sizeof (uint8_t));
                memcpy (ether_frame + 6, src_mac, 6 * sizeof (uint8_t));

                // Next is ethernet type code (ETH_P_ARP for ARP).
                // http://www.iana.org/assignments/ethernet-numbers
                ether_frame[12] = ETH_P_ARP / 256;
                ether_frame[13] = ETH_P_ARP % 256;

                // Next is ethernet frame data (ARP header).

                // ARP header
                memcpy (ether_frame + ETH_HDRLEN, &arphdr, ARP_HDRLEN * sizeof (uint8_t));

                // Submit request for a raw socket descriptor.
                if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
                    perror ("socket() failed ");
                    exit (EXIT_FAILURE);
                }

                // Send ethernet frame to socket.
                if ((bytes = sendto (sd, ether_frame, frame_length, 0, (struct sockaddr *) &device, sizeof (device))) <= 0) {
                    perror ("sendto() failed");
                    exit (EXIT_FAILURE);
                }

                // Close socket descriptor.
                close (sd);
                // }
                // Free allocated memory.
                free (src_mac);
                free (dst_mac);
                free (ether_frame);
                free (interface);
                free (target);
                free (src_ip);
            }

        }
    }
    return 0;
}
void *Send_arp_ipv4_thread (void *args)
{
    UNREFERENCED_PARAMETER(args);
    PLmDevicePresenceDetectionInfo pobject = NULL;
    unsigned int ActiveClientsecs = 0;
    unsigned int InActiveClientsecs = 0;
    pobject = GetPresenceDetectionObject();
    /*CID: 68372 Dereference after null check*/
    /*CID: 65919 Dereference before null check*/
    if (!pobject)
         return NULL;
    ++pobject->task_count;
    pthread_detach(pthread_self());
    while (pobject->taskState != STATE_DETECTION_TASK_STOP)
    {
            pthread_mutex_lock(&PresenceDetectionMutex);
            if (pobject->ipv4_leave_detection_interval)
            {
                if (ActiveClientsecs && (0 == (ActiveClientsecs % pobject->ipv4_leave_detection_interval)))
                {
                    sendIpv4ArpMessage(pobject,TRUE); // send message to Active client
                    ActiveClientsecs = 0;
                }
            } 
            else
            {
                ActiveClientsecs = 0;
            }  
        
            if (pobject->bkgnd_join_detection_interval)
            {
                if ( InActiveClientsecs  && (0 == (InActiveClientsecs % pobject->bkgnd_join_detection_interval)))
                {
                    sendIpv4ArpMessage(pobject,FALSE); // send message to InActive client
                    InActiveClientsecs = 0;
                }
            }
            else
            {
                InActiveClientsecs = 0;
            }            
            pthread_mutex_unlock(&PresenceDetectionMutex);
        
        sleep(1);        
        ++ActiveClientsecs;
        ++InActiveClientsecs;
    }
    if (pobject->task_count > 0)
    --pobject->task_count;
    return args;
}

void *ReceiveArp_Thread(void *args)
{
    UNREFERENCED_PARAMETER(args);
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pobject = GetPresenceDetectionObject();
    if (pobject)
    ++pobject->task_count;
    pthread_detach(pthread_self());
    while(pobject && (pobject->taskState != STATE_DETECTION_TASK_STOP))
    {
        FILE *arpCache = NULL; 
        char output[ARP_BUFFER_LEN];
        char buf[64];
        int ret = 0;

        syscfg_get(NULL, "lan_ifname", buf, sizeof(buf));
        ret = v_secure_system("arp -i %s -an > "PRESENCE_ARP_CACHE,buf);
        if(ret !=0)
        {
             CcspTraceDebug(("Failed in executing the command via v_secure_system ret: %d \n",ret));
        }

        arpCache = fopen(PRESENCE_ARP_CACHE, "r");
        if (!arpCache)
        {
            perror("Arp Cache: Failed to open file \"" PRESENCE_ARP_CACHE "\"");
            return NULL;
        }

        while(fgets(output, sizeof(output), arpCache)!=NULL)
        {
            pthread_mutex_lock(&PresenceDetectionMutex);
            Handle_RecieveArpCache(output);
            pthread_mutex_unlock(&PresenceDetectionMutex);
        }
        fclose(arpCache);
        sleep(10);
    }
    if (pobject && (pobject->task_count > 0))
    --pobject->task_count;
    return args;
}

int  getipaddressfromarp(char *inputline,char *output, int out_len)
{
    char *startip = NULL;
    if (!inputline || !output || out_len < 18)
        return -1;
    startip = strstr(inputline,"(");
    if (startip)
    {
        char *end = NULL;
        end = strstr(startip,")");
        if (end)
        {
            memset(output,0,out_len);
            if ((end - startip - 1) > 0)
            {
                memcpy(output,startip + 1,(end - startip - 1));
                return 0;
            }
        }
    }
    return -1;
}

int Handle_RecieveArpCache(char *line)
{
    int i = 0;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    if (!line)
        return -1;
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        for(i = 0; i<pobject->numOfDevice; i++)
        {
            if (pobject->ppdevlist && pobject->ppdevlist[i])
            {
                PLmPresenceDeviceInfo pobj = pobject->ppdevlist[i];                
                if(strcasestr(line, pobj->mac))
                {
                    PLmPresenceDeviceInfo pobj = pobject->ppdevlist[i];
                    int retval = 0;
                    char buf[IPV4_SIZE];
                    pobj->ipv4Active = TRUE;
                    pobj->currentActive = TRUE;
                    pobj->ipv4_retry_count = 0;
                    pobj->ipv4_state = STATE_JOIN_DETECTED_ARP;
                    retval = getipaddressfromarp(line,buf,sizeof(buf));
                    if (0 == retval)
                    {
                        if (strlen(buf) > 0)
                        /*CID:135467 Buffer not null terminated*/
                        strncpy(pobj->ipv4,buf,sizeof(pobj->ipv4)-1);
                        pobj->ipv4[sizeof(pobj->ipv4)-1] = '\0';
                    }
                    // trigger join callback            
                    if (pobject->clbk)
                    {
                        pobject->clbk(pobj);
                    }
                    break;                  
                }
            }
        }
    }
    return 0;

}

int CheckandupdatePresence(char *mac, int version, char *ipaddress,DeviceDetectionState state)
{
    int i = 0;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    if (!mac)
        return -1;
    CcspTraceDebug(("%s received mac= %s version %d state %d\n",__FUNCTION__,mac,version,state));
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        for(i = 0; i<pobject->numOfDevice; i++)
        {
            if (pobject->ppdevlist && pobject->ppdevlist[i])
            {
                PLmPresenceDeviceInfo pobj = pobject->ppdevlist[i];
                if (strcasecmp(pobj->mac, mac) == 0)
                {
                    switch (version)
                    {
                        case IPV4:
                        {
                            pobj->ipv4Active = TRUE;
                            pobj->currentActive = TRUE;
                            pobj->ipv4_retry_count = 0;
                            pobj->ipv4_state = state;
                            if (ipaddress)
                            {
                                /*CID: 135267 Buffer not null terminated*/
                                strncpy(pobj->ipv4,ipaddress,sizeof(pobj->ipv4)-1);
                                pobj->ipv4[sizeof(pobj->ipv4)-1] = '\0';
                            }
                        }
                        break;
                        case IPV6:
                        {
                            pobj->ipv6Active = TRUE;
                            pobj->currentActive = TRUE;
                            pobj->ipv6_retry_count = 0;
                            pobj->ipv6_state = state;
                            if (ipaddress)
                            {
                                 /*CID: 135267 Buffer not null terminated*/
                                strncpy(pobj->ipv6,ipaddress,sizeof(pobj->ipv6)-1);
                                pobj->ipv6[sizeof(pobj->ipv6)-1] = '\0';
                            }
                        }
                        break;
                        default:
                        break;
                    }   
                    // trigger join callback            
                    if (pobject->clbk)
                    {
                        pobject->clbk(pobj);
                    }
                    break;                  
               }
            }
        }
    }
    return 0;
}

void *ReceiveIpv4ClientStatus(void *args)
{
    UNREFERENCED_PARAMETER(args);
    mqd_t mq = -1;
    char buffer[MAX_SIZE + 1];
    PLmDevicePresenceDetectionInfo pobject = NULL;

    pobject = GetPresenceDetectionObject();
    pthread_detach(pthread_self());
    
    if (pobject)
    ++pobject->task_count;

    mq = mq_open(DNSMASQ_PRESENCE_QUEUE_NAME, O_RDONLY | O_NONBLOCK);

    do
    {
        ssize_t bytes_read;
        DnsmasqEventQData EventMsg;

        if (mq < 0)
        {
            mq = mq_open(DNSMASQ_PRESENCE_QUEUE_NAME, O_RDONLY | O_NONBLOCK);
        }
        else
        {
            /* receive the message */
            bytes_read = mq_receive(mq, buffer, MAX_SIZE, NULL);

            if (bytes_read > 0)
            {
                buffer[bytes_read] = '\0';
                memcpy(&EventMsg,buffer,sizeof(EventMsg));
                /* CID 340286 String not null terminated */
                EventMsg.mac[MAC_SIZE-1] = '\0';

                if(EventMsg.MsgType == MSG_TYPE_DNS_PRESENCE)
                {
                    pthread_mutex_lock(&PresenceDetectionMutex);
                    CheckandupdatePresence(EventMsg.mac,IPV4,NULL,STATE_JOIN_DETECTED_DNSMASQ);	 // Ip is not sent from dnsmasq
                    pthread_mutex_unlock(&PresenceDetectionMutex);

                }
            }
        }
        sleep(3);
    }while(pobject && (STATE_DETECTION_TASK_STOP != pobject->taskState));

     if (pobject && (pobject->task_count > 0))
    --pobject->task_count;
    if (mq != (mqd_t)-1)
    {
        int ret = mq_close(mq);
        if (ret == 0)
            mq_unlink(DNSMASQ_PRESENCE_QUEUE_NAME);
	else
	    printf("mq close failed");
    }
    return args;
}

void RecvHCPv4ClientConnects()
{
    int sd, new_socket, valread;
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char buffer[1024] = {0}; 
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pobject = GetPresenceDetectionObject();
    pthread_detach(pthread_self());
    //Opening socket connection
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) 
    { 
        printf("Failed to open socket descriptor\n"); 
        return; 
    } 
    // set reuse address flag
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                &opt, sizeof(opt)) < 0) 
    { 
        printf("Could not set reuse address option\n"); 
	close(sd);
        return; 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
    /*CID: 73139 Uninitialized scalar variable*/
    memset(&address.sin_zero, 0, sizeof(address.sin_zero));

    // bind the socket
    if (bind(sd, (struct sockaddr *)&address,  
                sizeof(address))<0) 
    { 
        printf("socket bind failed");
	close(sd);
        return; 
    } 
    if (listen(sd, 3) < 0) 
    { 
        perror("listen");
	close(sd);
        return; 
    } 

    printf("sd = %d\n",sd);
    if ((new_socket = accept(sd, (struct sockaddr *)&address,  
                    (socklen_t*)&addrlen))<0) 
    { 
        perror("accept");
	close(sd);
        return; 
    } 
    if (pobject)
	    ++pobject->task_count;

    printf ("\n %s waiting to read socket \n",__FUNCTION__);
    while(pobject && (STATE_DETECTION_TASK_STOP != pobject->taskState))
    {
	valread = read( new_socket , buffer, 1024); 
	if (valread < 0){
		printf("\n %s Can not read the socket %d\n",__FUNCTION__, new_socket); 
		close(new_socket);
		close(sd);
		return;
	}
	/* CID: 135473 String not null terminated*/ 
	/* CID: 164055 Out-of-bounds write */
	buffer[valread - 1] = '\0';
        printf("\n %s\n",buffer ); 
        printf("\n Hello message sent\n");
        if(strlen(buffer) != 0)
        {
            char* st = NULL;
            char* token = strtok_r(buffer, " ", &st);
            char* ip = strtok_r(NULL, " ", &st);
            if(token != NULL)
            {
                pthread_mutex_lock(&PresenceDetectionMutex);
                CheckandupdatePresence(token,IPV4, ip,STATE_JOIN_DETECTED_DNSMASQ);	
                pthread_mutex_unlock(&PresenceDetectionMutex);
            } 
        } 
    }
    close(new_socket);
    close(sd);
    if (pobject && (pobject->task_count > 0))
	    --pobject->task_count;

}

// Allocate memory for an array of chars.
char *
allocate_strmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (char *) malloc (len * sizeof (char));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (char));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_strmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Allocate memory for an array of unsigned chars.

uint8_t *
allocate_ustrmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (uint8_t *) malloc (len * sizeof (uint8_t));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (uint8_t));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
    exit (EXIT_FAILURE);
  }
}

int open_netlink(void)
{
    int sock;
    struct sockaddr_nl addr;
    int group = MYMGRP;

    sock = socket(AF_NETLINK, SOCK_RAW, MYPROTO);
    if (sock < 0) {
        printf("sock < 0.\n");
        return sock;
    }

    memset((void *) &addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    /* This doesn't work for some reason. See the setsockopt() below. */
    /* addr.nl_groups = MYMGRP; */

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("bind < 0.\n");
        close(sock);
        return -1;
    }

    /*
     * 270 is SOL_NETLINK. See
     * http://lxr.free-electrons.com/source/include/linux/socket.h?v=4.1#L314
     * and
     * http://stackoverflow.com/questions/17732044/
     */
    if (setsockopt(sock, 270, NETLINK_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        printf("setsockopt < 0\n");
        /*CID:73081 Resource leak*/
        close(sock);
        return -1;
    }

    return sock;
}


void read_event(int sock)
{
    struct sockaddr_nl nladdr;
    struct msghdr msg;
    struct iovec iov;
    int ret;
    int size = 65536;
    char *buffer = NULL;
    char *buffer1 = NULL;

    /*CID: 135612 Large stack use*/
    buffer = (char *) malloc(sizeof(char) * size);
    if(!buffer)
        return;

    buffer1 = (char *) malloc(sizeof(char) * size);

    if(!buffer1) {
       free(buffer);
       return;
    }    
 
    iov.iov_base = (void *) buffer;
    iov.iov_len = size;
    msg.msg_name = (void *) &(nladdr);
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    /* CID: 54845 Uninitialized scalar variable*/
    msg.msg_control         = NULL;
    msg.msg_controllen      = 0;
    msg.msg_flags           = 0;

    ret = recvmsg(sock, &msg, MSG_DONTWAIT);
    if (ret < 0)
    {
        CcspTraceDebug(("ret < 0.\n"));
    }
    else
    {
        char* st = NULL;
        char* token = NULL;
        char *ip = NULL;

        CcspTraceDebug(("Received message payload: %p\n", NLMSG_DATA((struct nlmsghdr *) buffer)));
        /* LIMITATION
         * Following strcpy() can't modified to safec strcpy_s() api
         * Because, safec has the limitation of copying only 4k ( RSIZE_MAX ) to destination pointer
         * And here, we have destination and source pointer size more than 4k, i.e 65536
         */
        /* CID 57216 Calling risky function */
        strncpy(buffer1, NLMSG_DATA((struct nlmsghdr *) buffer), size);
        buffer1[size-1] = '\0';
        CcspTraceDebug(("buffer1: %s\n", buffer1));
        token = strtok_r(buffer1, ",", &st);
        if(token != NULL)
        {
            token = strtok_r(NULL, ",", &st);  // Mac
            ip = strtok_r(NULL, ",", &st); // ipv6 address
            pthread_mutex_lock(&PresenceDetectionMutex);
            CheckandupdatePresence(token,IPV6,ip,STATE_JOIN_DETECTED_ND);	
	    pthread_mutex_unlock(&PresenceDetectionMutex);
	} 
    }
    free(buffer);
    free(buffer1);
}

void *RecvIPv6clientNotifications(void *args)
{
    UNREFERENCED_PARAMETER(args);
    int nls;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pobject = GetPresenceDetectionObject();
    pthread_detach(pthread_self());
    nls = open_netlink();
    if (nls < 0)
        return NULL;

    if (pobject)
    ++pobject->task_count;
    while (pobject && (pobject->taskState != STATE_DETECTION_TASK_STOP))
    {
        read_event(nls);
        sleep(3);
    }
    if (pobject && (pobject->task_count > 0))
    --pobject->task_count;

    close(nls);
    return args;
}

int Send_ipv6_neighbourdiscovery(PLmDevicePresenceDetectionInfo pobject,BOOL bactiveclient)
{
    int i = 0,ret = 0;
    char buf[64];
    if (pobject)
    {
        for(i = 0; i<pobject->numOfDevice; i++)
        {
            if (pobject->ppdevlist && pobject->ppdevlist[i])
            {
                PLmPresenceDeviceInfo pobj = pobject->ppdevlist[i];
                if ((!pobj->ipv6Active) || (pobj->currentActive != bactiveclient))
                    continue;
                ++pobj->ipv6_retry_count;
                if (pobj->ipv6_retry_count > pobject->ipv6_num_retries)
                {
                    pobj->ipv6_state = STATE_LEAVE_DETECTED;
                    pobj->ipv6_retry_count = 0;
                    /* Dual mode case, To identify accurate presence leave 
                     * reset IPV4 status and remove IPV4 entry from ARP.
                     * If device is in connected state for ipv4 case, ARP will updated again.
                     * otherwise considered this device is in-active.
                     */
                    if (pobj->ipv4Active && (pobj->ipv4_state != STATE_LEAVE_DETECTED))
                    {              
                        char buf[64];
                        pobj->ipv4_retry_count = 0;
                        syscfg_get(NULL, "lan_ifname", buf, sizeof(buf));
                        ret = v_secure_system("ip neigh del %s dev %s",pobj->ipv4,buf);
                        if(ret !=0)
                        {
                            CcspTraceDebug(("Failed in executing the command via v_secure_system ret: %d \n",ret));
                        }
                        continue;
                    }
                    pobj->currentActive = FALSE;
                    // trigger leave callback
                    if (pobject->clbk)
                    {
                        pobject->clbk(pobj);
                    }
                    continue;
                }
                syscfg_get( NULL, "lan_ifname", buf, sizeof(buf));
                CcspTraceDebug(("cmd = ndisc6 %s %s -r 1 -q", pobj->ipv6,buf));
                ret = v_secure_system("ndisc6 %s %s -r 1 -q", pobj->ipv6,buf);
                if(ret !=0)
                {
                     CcspTraceDebug(("Failed in executing the command via v_secure_system ret: %d \n",ret));
                }

            }
        }
    }
    return 0;

}

void *SendNS_Thread(void *args)
{
    UNREFERENCED_PARAMETER(args);
    PLmDevicePresenceDetectionInfo pobject = NULL;
    unsigned int ActiveClientsecs = 0;
    unsigned int InActiveClientsecs = 0;
    pobject = GetPresenceDetectionObject();
    /*CID: 71755 Dereference after null check*/
    /*CID: 57809 Dereference before null check*/
    if (!pobject)
        return NULL;
    ++pobject->task_count;
    pthread_detach(pthread_self());
	while(pobject->taskState != STATE_DETECTION_TASK_STOP)
	{
		if (pobject)
        {
            pthread_mutex_lock(&PresenceDetectionMutex);
            if (pobject->ipv6_leave_detection_interval)
            {
                if (ActiveClientsecs && (0 == (ActiveClientsecs % pobject->ipv6_leave_detection_interval)))
                {
                    Send_ipv6_neighbourdiscovery(pobject, TRUE);
                    ActiveClientsecs = 0;
                }
            } 
            else
            {
                ActiveClientsecs = 0;
            }  
        
            if (pobject->bkgnd_join_detection_interval)
            {
                if (InActiveClientsecs && (0 == (InActiveClientsecs % pobject->bkgnd_join_detection_interval)))
                {
                    Send_ipv6_neighbourdiscovery(pobject, FALSE);
                    InActiveClientsecs = 0;
                }
            }
            else
            {
                InActiveClientsecs = 0;
            }            
            pthread_mutex_unlock(&PresenceDetectionMutex);
        }
        
        sleep(1);        
        ++ActiveClientsecs;
        ++InActiveClientsecs;        
    }
    if (pobject && (pobject->task_count > 0))
    --pobject->task_count;
    return args;
}

void PresenceDetection_Stop()
{
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pobject = GetPresenceDetectionObject();
    printf("\n %s enter \n",__FUNCTION__);
    if (pobject)
    {
        printf("\n before stop thread count %d \n",pobject->task_count);
        pobject->taskState = STATE_DETECTION_TASK_STOP;
        while (0 != pobject->task_count)
        {
            sleep(1);
        }
        printf("\n after stop thread count %d \n",pobject->task_count);
    }
    printf("\n %s exit \n",__FUNCTION__);
}

void PresenceDetection_Start()
{
    int res = 0;
    pthread_t RecvHCPv4ClientConnects_ThreadID;
    PLmDevicePresenceDetectionInfo pobject = NULL;
    pobject = GetPresenceDetectionObject();
    if (pobject)
    {
        pobject->taskState = STATE_DETECTION_TASK_START;
    }

    printf("\n %s enter \n",__FUNCTION__);
    res = pthread_create(&RecvHCPv4ClientConnects_ThreadID, NULL, ReceiveIpv4ClientStatus, "ReceiveIpv4Client");
    if(res != 0) {
        printf("Create RecvHCPv4ClientConnects error %d\n", res);
    }

    pthread_t SendArp_ThreadID;
    res = pthread_create(&SendArp_ThreadID, NULL, Send_arp_ipv4_thread, "SendArp_Thread");
    if(res != 0) {
        printf("Create SendArp_Thread error %d\n", res);
    }

    pthread_t ReceiveArp_ThreadID;
    res = pthread_create(&ReceiveArp_ThreadID, NULL, ReceiveArp_Thread, "ReceiveArp_Thread");
    if(res != 0) {
        printf("Create ReceiveArp_Thread error %d\n", res);
	}
    pthread_t RecvIPv6clientNotifications_ThreadID;
    res = pthread_create(&RecvIPv6clientNotifications_ThreadID, NULL, RecvIPv6clientNotifications, "RecvIPv6clientNotifications_Thread");
    if(res != 0) {
        printf("Create RecvIPv6clientNotifications error %d\n", res);
    }
    pthread_t SendNS_ThreadID;
    res = pthread_create(&SendNS_ThreadID, NULL, SendNS_Thread, "SendNS_Thread");
    if(res != 0) {
        printf("Create SendNS_Thread error %d\n", res);
    }

    if (pobject)
    {
        pobject->taskState = STATE_DETECTION_TASK_STARTED;
    }
    printf("\n %s exit \n",__FUNCTION__);
}

