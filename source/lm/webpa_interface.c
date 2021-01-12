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
#define _GNU_SOURCE
#include "ssp_global.h"
#include "stdlib.h"
#include "ccsp_dm_api.h"
#include "webpa_interface.h"
#include "ccsp_lmliteLog_wrapper.h"
#include "lm_util.h"
#include <sysevent/sysevent.h>
#include <libparodus/libparodus.h>
#include "webpa_pd.h"
#include <math.h>
#include "syscfg/syscfg.h"
#include "ccsp_memory.h"

#ifdef MLT_ENABLED
#include "rpl_malloc.h"
#include "mlt_malloc.h"
#endif
#include "safec_lib_common.h"

#define MAX_PARAMETERNAME_LEN   512

#ifdef _SKY_HUB_COMMON_PRODUCT_REQ_
#include <utctx/utctx.h>
#include <utctx/utctx_api.h>
#include <utapi/utapi.h>
#include <utapi/utapi_util.h>
#endif

extern ANSC_HANDLE bus_handle;
pthread_mutex_t webpa_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t device_mac_mutex = PTHREAD_MUTEX_INITIALIZER;

char deviceMAC[32]={'\0'}; 
char fullDeviceMAC[32]={'\0'};
#define ETH_WAN_STATUS_PARAM "Device.Ethernet.X_RDKCENTRAL-COM_WAN.Enabled"
#define RDKB_ETHAGENT_COMPONENT_NAME                  "com.cisco.spvtg.ccsp.ethagent"
#define RDKB_ETHAGENT_DBUS_PATH                       "/com/cisco/spvtg/ccsp/ethagent"

libpd_instance_t client_instance;
static void *handle_parodus();
static int check_ethernet_wan_status();
int s_sysevent_connect(token_t *out_se_token);

int WebpaInterface_DiscoverComponent(char** pcomponentName, char** pcomponentPath )
{
    char CrName[256] = {0};
    int ret = 0;
    errno_t rc = -1;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    rc = sprintf_s(CrName, sizeof(CrName), "eRT.%s", CCSP_DBUS_INTERFACE_CR);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }
    componentStruct_t **components = NULL;
    int compNum = 0;
    int res = CcspBaseIf_discComponentSupportingNamespace (
            bus_handle,
            CrName,
#ifndef _XF3_PRODUCT_REQ_
#ifdef _SKY_HUB_COMMON_PRODUCT_REQ_
            "Device.DeviceInfo.X_COMCAST-COM_WAN_MAC",
#else
            "Device.X_CISCO_COM_CableModem.MACAddress",
#endif // _SKY_HUB_COMMON_PRODUCT_REQ_
#else
            "Device.DPoE.Mac_address",
#endif      
            "",
            &components,
            &compNum);
    if(res != CCSP_SUCCESS || compNum < 1){
        CcspTraceError(("WebpaInterface_DiscoverComponent find eRT PAM component error %d\n", res));
        ret = -1;
    }
    else{
        *pcomponentName = AnscCloneString(components[0]->componentName);
        *pcomponentPath = AnscCloneString(components[0]->dbusPath);
        CcspTraceInfo(("WebpaInterface_DiscoverComponent find eRT PAM component %s--%s\n", *pcomponentName, *pcomponentPath));
    }
    free_componentStruct_t(bus_handle, compNum, components);
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return ret;
}

void sendWebpaMsg(char *serviceName, char *dest, char *trans_id, char *contentType, char *payload, unsigned int payload_len)
{
    pthread_mutex_lock(&webpa_mutex);
    wrp_msg_t *wrp_msg ;
    int retry_count = 0, backoffRetryTime = 0, c = 2;
    int sendStatus = -1;
    char source[MAX_PARAMETERNAME_LEN/2] = {'\0'};

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, <======== Start of sendWebpaMsg =======>\n"));
	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, deviceMAC *********:%s\n",deviceMAC));
    if(serviceName!= NULL){
    	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, serviceName :%s\n",serviceName));
		snprintf(source, sizeof(source), "mac:%s/%s", deviceMAC, serviceName);
	}
	if(dest!= NULL){
    	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, dest :%s\n",dest));
	}
	if(trans_id!= NULL){
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, transaction_id :%s\n",trans_id));
	}
	if(contentType!= NULL){
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, contentType :%s\n",contentType));
    }
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, payload_len :%d\n",payload_len));

    

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Received DeviceMac from Atom side: %s\n",deviceMAC));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Source derived is %s\n", source));
    
    wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
    

    if(wrp_msg != NULL)
    {	
		memset(wrp_msg, 0, sizeof(wrp_msg_t));
        wrp_msg->msg_type = WRP_MSG_TYPE__EVENT;
        wrp_msg->u.event.payload = (void *)payload;
        wrp_msg->u.event.payload_size = payload_len;
        wrp_msg->u.event.source = source;
        wrp_msg->u.event.dest = dest;
        wrp_msg->u.event.content_type = contentType;

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->msg_type :%d\n",wrp_msg->msg_type));
        if(wrp_msg->u.event.payload!=NULL) 
        	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.payload :%s\n",(char *)(wrp_msg->u.event.payload)));
        	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.payload_size :%zu\n",(wrp_msg->u.event.payload_size)));
		if(wrp_msg->u.event.source != NULL)
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.source :%s\n",wrp_msg->u.event.source));
		if(wrp_msg->u.event.dest!=NULL)
        	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.dest :%s\n",wrp_msg->u.event.dest));
		if(wrp_msg->u.event.content_type!=NULL)
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.content_type :%s\n",wrp_msg->u.event.content_type));

        while(retry_count<=5)
        {
            backoffRetryTime = (int) pow(2, c) -1;

            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, retry_count : %d\n",retry_count));
            sendStatus = libparodus_send(client_instance, wrp_msg);
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, sendStatus is %d\n",sendStatus));
            if(sendStatus == 0)
            {
                retry_count = 0;
                CcspTraceInfo(("Sent message successfully to parodus\n"));
                break;
            }
            else
            {
                CcspTraceError(("Failed to send message: '%s', retrying ....\n",libparodus_strerror(sendStatus)));
                CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, backoffRetryTime %d seconds\n", backoffRetryTime));
                sleep(backoffRetryTime);
                c++;
                retry_count++;
            }
        }

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before freeing wrp_msg\n"));
        free(wrp_msg);
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, After freeing wrp_msg\n"));
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,  <======== End of sendWebpaMsg =======>\n"));

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    pthread_mutex_unlock(&webpa_mutex);
}

void initparodusTask()
{
	int err = 0;
	pthread_t parodusThreadId;
	
	err = pthread_create(&parodusThreadId, NULL, handle_parodus, NULL);
	if (err != 0) 
	{
		CcspLMLiteConsoleTrace(("RDK_LOG_ERROR, Error creating messages thread :[%s]\n", strerror(err)));
	}
	else
	{
		CcspLMLiteConsoleTrace(("RDK_LOG_INFO, handle_parodus thread created Successfully\n"));
	}
}

static void *handle_parodus()
{
    int backoffRetryTime = 0;
    int backoff_max_time = 9;
    int max_retry_sleep;
    //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
    int c =2;
    char *parodus_url = NULL;

    CcspLMLiteConsoleTrace(("RDK_LOG_INFO, ******** Start of handle_parodus ********\n"));

    pthread_detach(pthread_self());

    max_retry_sleep = (int) pow(2, backoff_max_time) -1;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, max_retry_sleep is %d\n", max_retry_sleep ));

        get_parodus_url(&parodus_url);
	if(parodus_url != NULL)
	{
	
		libpd_cfg_t cfg1 = {.service_name = "lmlite",
						.receive = false, .keepalive_timeout_secs = 0,
						.parodus_url = parodus_url,
						.client_url = NULL
					   };
		            
		CcspLMLiteConsoleTrace(("RDK_LOG_INFO, Configurations => service_name : %s parodus_url : %s client_url : %s\n", cfg1.service_name, cfg1.parodus_url, (cfg1.client_url) ? cfg1.client_url : "" ));

		CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Call parodus library init api \n"));

		while(1)
		{
		    if(backoffRetryTime < max_retry_sleep)
		    {
		        backoffRetryTime = (int) pow(2, c) -1;
		    }

		    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime));
		    int ret =libparodus_init (&client_instance, &cfg1);
		    CcspLMLiteConsoleTrace(("RDK_LOG_INFO, ret is %d\n",ret));
		    if(ret ==0)
		    {
		        CcspTraceWarning(("LMLite: Init for parodus Success..!!\n"));
		        break;
		    }
		    else
		    {
		        CcspTraceError(("LMLite: Init for parodus (url %s) failed: '%s'\n", parodus_url, libparodus_strerror(ret)));
                        /*REVIST CID:67436 Logically dead code- parodus_url cant be NULL*/
			get_parodus_url(&parodus_url);
			cfg1.parodus_url = parodus_url;
			sleep(backoffRetryTime);
		        c++;
		    }
		libparodus_shutdown(client_instance);
		   
		}
	}
    return 0;
}

const char *rdk_logger_module_fetch(void)
{
    return "LOG.RDK.LM";
}

char * getFullDeviceMac()
{
    if(strlen(fullDeviceMAC) == 0)
    {
        getDeviceMac();
    }

    return fullDeviceMAC;
}

static int check_ethernet_wan_status()
{
    int ret = -1;
    char isEthEnabled[8];

    if ((syscfg_get(NULL, "eth_wan_enabled", isEthEnabled, sizeof(isEthEnabled)) == 0) &&
        (strcmp(isEthEnabled, "true") == 0))
    {
        CcspTraceInfo(("Ethernet WAN is enabled\n"));
        ret = CCSP_SUCCESS;
    }

    return ret;
}

char * getDeviceMac()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

#if defined(_PLATFORM_RASPBERRYPI_)
    // Return without performing any operation as RPi Platform don't have Cable Modem and execution
    // of this function on RPI puts calling thread in infinite wait.
    return deviceMAC;
#endif
  
#ifdef _SKY_HUB_COMMON_PRODUCT_REQ_
    char wanPhyName[32] = {0};
    char out_value[32] = {0};
    char deviceMACVal[32] = {0};
    errno_t ret_code = -1;

    syscfg_init();

    if (syscfg_get(NULL, "wan_physical_ifname", out_value, sizeof(out_value)) == 0)
    {
        strncpy(wanPhyName, out_value, sizeof(wanPhyName));
        CcspTraceInfo(("%s %d - WanPhyName=%s \n", __FUNCTION__,__LINE__, wanPhyName));
    }
    else
    {
        strncpy(wanPhyName, "erouter0", sizeof(wanPhyName));
        CcspTraceInfo(("%s %d - WanPhyName=%s \n", __FUNCTION__,__LINE__, wanPhyName));
    }
    s_get_interface_mac(wanPhyName, deviceMACVal, sizeof(deviceMACVal));
    ret_code = STRCPY_S_NOCLOBBER(fullDeviceMAC, sizeof(fullDeviceMAC),deviceMACVal);
    ERR_CHK(ret_code);
    AnscMacToLower(deviceMAC, deviceMACVal, sizeof(deviceMAC));
    // Removing the \n at the end of mac
    if((strlen(deviceMAC) == 13) && (deviceMAC[12] == '\n'))
    {
        deviceMAC[12] = '\0';
        CcspTraceInfo(("%s %d - removed new line at the end of deviceMAC %s \n", __FUNCTION__,__LINE__, deviceMAC));
    }
    CcspTraceInfo(("%s %d -  deviceMAC is - %s \n", __FUNCTION__,__LINE__, deviceMAC));
    return deviceMAC;
#endif //_SKY_HUB_COMMON_PRODUCT_REQ_

    while(!strlen(deviceMAC))
    {
        pthread_mutex_lock(&device_mac_mutex);
        int ret = -1, val_size =0,cnt =0, fd = 0;
        char *pcomponentName = NULL, *pcomponentPath = NULL;
        parameterValStruct_t **parameterval = NULL;
        token_t  token;
        char deviceMACValue[32] = { '\0' };
        errno_t rc = -1;
#ifndef _XF3_PRODUCT_REQ_
        char *getList[] = {"Device.X_CISCO_COM_CableModem.MACAddress"};
#else
        char *getList[] = {"Device.DPoE.Mac_address"};
#endif
        
        if (strlen(deviceMAC))
        {
            pthread_mutex_unlock(&device_mac_mutex);
            break;
        }

        fd = s_sysevent_connect(&token);
        if(CCSP_SUCCESS == check_ethernet_wan_status() && sysevent_get(fd, token, "eth_wan_mac", deviceMACValue, sizeof(deviceMACValue)) == 0 && deviceMACValue[0] != '\0')
        {
            rc = STRCPY_S_NOCLOBBER(fullDeviceMAC, sizeof(fullDeviceMAC),deviceMACValue);
            ERR_CHK(rc);
            AnscMacToLower(deviceMAC, deviceMACValue, sizeof(deviceMAC));
            CcspTraceInfo(("deviceMAC is %s\n", deviceMAC));
        }
        else
        {
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before WebpaInterface_DiscoverComponent ret: %d\n",ret));

            if(pcomponentPath == NULL || pcomponentName == NULL)
            {
                if(-1 == WebpaInterface_DiscoverComponent(&pcomponentName, &pcomponentPath)){
                    CcspTraceError(("%s ComponentPath or pcomponentName is NULL\n", __FUNCTION__));
            		pthread_mutex_unlock(&device_mac_mutex);
                    return NULL;
                }
                CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, WebpaInterface_DiscoverComponent ret: %d  ComponentPath %s ComponentName %s \n",ret, pcomponentPath, pcomponentName));
            }

            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before GPV ret: %d\n",ret));
            ret = CcspBaseIf_getParameterValues(bus_handle,
                        pcomponentName, pcomponentPath,
                        getList,
                        1, &val_size, &parameterval);
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, After GPV ret: %d\n",ret));
            if(ret == CCSP_SUCCESS)
            {
                CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, val_size : %d\n",val_size));
                for (cnt = 0; cnt < val_size; cnt++)
                {
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, parameterval[%d]->parameterName : %s\n",cnt,parameterval[cnt]->parameterName));
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, parameterval[%d]->parameterValue : %s\n",cnt,parameterval[cnt]->parameterValue));
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, parameterval[%d]->type :%d\n",cnt,parameterval[cnt]->type));
                
                }
                CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Calling macToLower to get deviceMacId\n"));
                rc = STRCPY_S_NOCLOBBER(fullDeviceMAC, sizeof(fullDeviceMAC),parameterval[0]->parameterValue);
                ERR_CHK(rc);
                AnscMacToLower(deviceMAC, parameterval[0]->parameterValue, sizeof(deviceMAC));
                if(pcomponentName)
                {
                    AnscFreeMemory(pcomponentName);
                }
                if(pcomponentPath)
                {
                    AnscFreeMemory(pcomponentPath);
                }

            }
            else
            {
                CcspLMLiteTrace(("RDK_LOG_ERROR, Failed to get values for %s ret: %d\n",getList[0],ret));
                CcspTraceError(("RDK_LOG_ERROR, Failed to get values for %s ret: %d\n",getList[0],ret));
                sleep(10);
            }
         
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before free_parameterValStruct_t...\n"));
            free_parameterValStruct_t(bus_handle, val_size, parameterval);
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, After free_parameterValStruct_t...\n"));
        }   
        pthread_mutex_unlock(&device_mac_mutex);
    
    }
        
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return deviceMAC;
}
