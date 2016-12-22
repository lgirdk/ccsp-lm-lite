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

#include "ssp_global.h"
#include "stdlib.h"
#include "ccsp_dm_api.h"
#include "webpa_interface.h"
#include "msgpack.h"
#include "base64.h"
#include "ccsp_lmliteLog_wrapper.h"
#include "lm_util.h"

#ifdef MLT_ENABLED
#include "rpl_malloc.h"
#include "mlt_malloc.h"
#endif

#define WEBPA_COMPONENT_NAME    "eRT.com.cisco.spvtg.ccsp.webpaagent"
#define WEBPA_DBUS_PATH         "/com/cisco/spvtg/ccsp/webpaagent"
#define WEBPA_PARAMETER_NAME    "Device.Webpa.PostData" 
#define MAX_PARAMETERNAME_LEN   512
#define CONTENT_TYPE            "content_type"
#define WEBPA_MSG_TYPE          "msg_type"
#define WEBPA_SOURCE            "source"
#define WEBPA_DESTINATION       "dest"
#define WEBPA_TRANSACTION_ID    "transaction_uuid"
#define WEBPA_PAYLOAD           "payload"
#define WEBPA_MAP_SIZE          6

extern ANSC_HANDLE bus_handle;
pthread_mutex_t webpa_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t device_mac_mutex = PTHREAD_MUTEX_INITIALIZER;

char deviceMAC[32]={'\0'}; 
char fullDeviceMAC[32]={'\0'}; 
static char * packStructure(char *serviceName, char *dest, char *trans_id, char *payload, char *contentType, unsigned int payload_len);
static void macToLower(char macValue[]);
//static char * packStructure(char *serviceName, char *dest, char *trans_id, char *payload, char *contentType, unsigned int payload_len);


int WebpaInterface_DiscoverComponent(char** pComponentName, char** pComponentPath )
{
    char CrName[256] = {0};
    int ret = 0;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    CrName[0] = 0;
    strcpy(CrName, "eRT.");
    strcat(CrName, CCSP_DBUS_INTERFACE_CR);

    componentStruct_t **components = NULL;
    int compNum = 0;
    int res = CcspBaseIf_discComponentSupportingNamespace (
            bus_handle,
            CrName,
            "Device.X_CISCO_COM_CableModem.MACAddress",
            "",
            &components,
            &compNum);
    if(res != CCSP_SUCCESS || compNum < 1){
        CcspTraceError(("WebpaInterface_DiscoverComponent find eRT PAM component error %d\n", res));
        ret = -1;
    }
    else{
        *pComponentName = LanManager_CloneString(components[0]->componentName);
        *pComponentPath = LanManager_CloneString(components[0]->dbusPath);
        CcspTraceInfo(("WebpaInterface_DiscoverComponent find eRT PAM component %s--%s\n", *pComponentName, *pComponentPath));
    }
    free_componentStruct_t(bus_handle, compNum, components);
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return ret;
}

void sendWebpaMsg(char *serviceName, char *dest, char *trans_id, char *contentType, char *payload, unsigned int payload_len)
{
    pthread_mutex_lock(&webpa_mutex);

    char* faultParam = NULL;
    int ret = -1;
    parameterValStruct_t val = {0};
    char * packedMsg = NULL;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, <======== Start of sendWebpaMsg =======>\n"));
    // Pack the message using msgpck WRP notification format and then using base64        
    packedMsg = packStructure(serviceName, dest, trans_id, payload, contentType,payload_len);              
    
/*    if(consoleDebugEnable)    
        {
            fprintf(stderr, "RDK_LOG_DEBUG, base64 encoded msgpack packed data containing %d bytes is : %s\n",strlen(packedMsg),packedMsg);
        }*/
    
    // set this packed message as value of WebPA Post parameter 
    val.parameterValue = packedMsg;
    val.type = ccsp_base64;
    val.parameterName = WEBPA_PARAMETER_NAME;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, val.parameterName %s, val.type %d\n",val.parameterName,val.type));
    ret = CcspBaseIf_setParameterValues(bus_handle,
                WEBPA_COMPONENT_NAME, WEBPA_DBUS_PATH, 0,
                0x0000000C, /* session id and write id */
                &val, 1, TRUE, /* no commit */
                &faultParam);

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, CcspBaseIf_setParameterValues ret %d\n",ret));
    if (ret != CCSP_SUCCESS && faultParam) 
    {
        CcspLMLiteTrace(("RDK_LOG_ERROR, ~~~~ Error:Failed to SetValue for param  '%s' ~~~~ ret : %d \n", faultParam, ret));
    }
    
    if(packedMsg != NULL)
    {
        free(packedMsg);
        packedMsg = NULL;
    }


    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,  <======== End of sendWebpaMsg =======>\n"));

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    pthread_mutex_unlock(&webpa_mutex);
}


static char * packStructure(char *serviceName, char *dest, char *trans_id, char *payload, char *contentType, unsigned int payload_len)
{           
    msgpack_sbuffer sbuf;
    msgpack_packer pk;  
    char* b64buffer =  NULL;
    size_t encodeSize = 0;
    char source[MAX_PARAMETERNAME_LEN/2] = {'\0'}; 
    int msg_type = 4;

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, deviceMAC *********:%s\n",deviceMAC));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, serviceName :%s\n",serviceName));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, dest :%s\n",dest));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, transaction_id :%s\n",trans_id));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, contentType :%s\n",contentType));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, payload_len :%d\n",payload_len));

    snprintf(source, sizeof(source), "mac:%s/%s", deviceMAC, serviceName);

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Received DeviceMac from Atom side: %s\n",deviceMAC));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Source derived is %s\n", source));
  
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, <======== Start of packStructure ======>\n"));
    
    // Start of msgpack encoding
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, -----------Start of msgpack encoding------------\n"));

    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
    msgpack_pack_map(&pk, WEBPA_MAP_SIZE);

    msgpack_pack_str(&pk, strlen(WEBPA_MSG_TYPE));
    msgpack_pack_str_body(&pk, WEBPA_MSG_TYPE,strlen(WEBPA_MSG_TYPE));
    msgpack_pack_int(&pk, msg_type);   
    
    msgpack_pack_str(&pk, strlen(WEBPA_SOURCE));
    msgpack_pack_str_body(&pk, WEBPA_SOURCE,strlen(WEBPA_SOURCE));
    msgpack_pack_str(&pk, strlen(source));
    msgpack_pack_str_body(&pk, source,strlen(source));
    
    msgpack_pack_str(&pk, strlen(WEBPA_DESTINATION));
    msgpack_pack_str_body(&pk, WEBPA_DESTINATION,strlen(WEBPA_DESTINATION));       
    msgpack_pack_str(&pk, strlen(dest));
    msgpack_pack_str_body(&pk, dest,strlen(dest));
    
    msgpack_pack_str(&pk, strlen(WEBPA_TRANSACTION_ID));
    msgpack_pack_str_body(&pk, WEBPA_TRANSACTION_ID,strlen(WEBPA_TRANSACTION_ID));
    msgpack_pack_str(&pk, strlen(trans_id));
    msgpack_pack_str_body(&pk, trans_id,strlen(trans_id));
     
    msgpack_pack_str(&pk, strlen(WEBPA_PAYLOAD));
    msgpack_pack_str_body(&pk, WEBPA_PAYLOAD,strlen(WEBPA_PAYLOAD));
       
    if(strcmp(contentType,"avro/binary") == 0)
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, msg->payload binary\n"));
        msgpack_pack_bin(&pk, payload_len);
        msgpack_pack_bin_body(&pk, payload, payload_len);
    }
    else // string: "contentType :application/json"
    {
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, msg->payload string\n"));
        msgpack_pack_str(&pk, strlen(payload));
        msgpack_pack_str_body(&pk, payload,strlen(payload));
    }
  
    msgpack_pack_str(&pk, strlen(CONTENT_TYPE));
    msgpack_pack_str_body(&pk, CONTENT_TYPE,strlen(CONTENT_TYPE));
    msgpack_pack_str(&pk, strlen(contentType));
    msgpack_pack_str_body(&pk, contentType,strlen(contentType));
    
    /*if(consoleDebugEnable)
        fprintf(stderr, "RDK_LOG_DEBUG,msgpack encoded data contains %d bytes and data is : %s\n",(int)sbuf.size,sbuf.data);*/

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,-----------End of msgpack encoding------------\n"));
    // End of msgpack encoding
    
    // Start of Base64 Encode
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,-----------Start of Base64 Encode ------------\n"));
    encodeSize = b64_get_encoded_buffer_size( sbuf.size );
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,encodeSize is %d\n", (int)encodeSize));
    b64buffer = malloc(encodeSize + 1); // one byte extra for terminating NULL byte
    b64_encode((const uint8_t *)sbuf.data, sbuf.size, (uint8_t *)b64buffer);
    b64buffer[encodeSize] = '\0' ;    
 
    
    /*if(consoleDebugEnable)
    {
    int i;
    fprintf(stderr, "RDK_LOG_DEBUG,\n\n b64 encoded data is : ");
    for(i = 0; i < encodeSize; i++)
        fprintf(stderr,"%c", b64buffer[i]);      

    fprintf(stderr,"\n\n");       
    }*/

    //CcspLMLiteTrace(("RDK_LOG_DEBUG,\nb64 encoded data length is %d\n",i));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,---------- End of Base64 Encode -------------\n"));
    // End of Base64 Encode
    
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,Destroying sbuf.....\n"));
    msgpack_sbuffer_destroy(&sbuf);
    
    //CcspLMLiteTrace(("RDK_LOG_DEBUG,Final Encoded data: %s\n",b64buffer));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,Final Encoded data length: %d\n",(int)strlen(b64buffer)));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,<======== End of packStructure ======>\n"));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return b64buffer;
}


char * getFullDeviceMac()
{
    if(strlen(fullDeviceMAC) == 0)
    {
        getDeviceMac();
    }

    return fullDeviceMAC;
}

char * getDeviceMac()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    while(!strlen(deviceMAC))
    {
        pthread_mutex_lock(&device_mac_mutex);
        int ret = -1, val_size =0,cnt =0;
        char *pComponentName = NULL, *pComponentPath = NULL;
	parameterValStruct_t **parameterval = NULL;
        char *getList[] = {"Device.X_CISCO_COM_CableModem.MACAddress"};
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before WebpaInterface_DiscoverComponent ret: %d\n",ret));

        if(pComponentPath == NULL || pComponentName == NULL)
        {
            if(-1 == WebpaInterface_DiscoverComponent(&pComponentName, &pComponentPath)){
                CcspTraceError(("%s ComponentPath or pComponentName is NULL\n", __FUNCTION__));
        		pthread_mutex_unlock(&device_mac_mutex);
                return NULL;
            }
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, WebpaInterface_DiscoverComponent ret: %d  ComponentPath %s ComponentName %s \n",ret, pComponentPath, pComponentName));
        }

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before GPV ret: %d\n",ret));
        ret = CcspBaseIf_getParameterValues(bus_handle,
                    pComponentName, pComponentPath,
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
            strcpy(fullDeviceMAC, parameterval[0]->parameterValue);
            macToLower(parameterval[0]->parameterValue);
            if(pComponentName)
            {
                AnscFreeMemory(pComponentName);
            }
            if(pComponentPath)
            {
                AnscFreeMemory(pComponentPath);
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
        pthread_mutex_unlock(&device_mac_mutex);
    
    }
        
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return deviceMAC;
}

void macToLower(char macValue[])
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    int i = 0;
    int j;
    char *token[32];
    char tmp[32];
    strncpy(tmp, macValue,sizeof(tmp));
    token[i] = strtok(tmp, ":");
    if(token[i]!=NULL)
    {
        strncat(deviceMAC, token[i],sizeof(deviceMAC)-1);
        deviceMAC[31]='\0';
        i++;
    }
    while ((token[i] = strtok(NULL, ":")) != NULL) 
    {
        strncat(deviceMAC, token[i],sizeof(deviceMAC)-1);
        deviceMAC[31]='\0';
        i++;
    }
    deviceMAC[31]='\0';
    for(j = 0; deviceMAC[j]; j++)
    {
        deviceMAC[j] = tolower(deviceMAC[j]);
    }
    
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Inside macToLower:: Device MAC: %s check\n",deviceMAC));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

}

