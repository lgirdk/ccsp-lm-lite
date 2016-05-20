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

#include <stdio.h>
#include <assert.h>
#include <avro.h>
#include <arpa/inet.h>
#include <semaphore.h>  /* Semaphore */
#include <uuid/uuid.h>
#include "ansc_platform.h"

#include "base64.h"
#include "webpa_interface.h"
#include "network_devices_status_avropack.h"
#include "ccsp_lmliteLog_wrapper.h"
#include "lm_main.h"

#define MAGIC_NUMBER      0x85
#define MAGIC_NUMBER_SIZE 1
#define SCHEMA_ID_LENGTH  32
#define WRITER_BUF_SIZE   1024 * 30 // 30K

//      "schemaTypeUUID" : "d9823986-8092-4ee9-b1f6-cf808486f186",
//      "schemaMD5Hash" : "8e2a859d6ef44610423f559296565635",

uint8_t HASH[16] = {0x8e, 0x2a, 0x85, 0x9d, 0x6e, 0xf4, 0x46, 0x10,
                    0x42, 0x3f, 0x55, 0x92, 0x96, 0x56, 0x56, 0x35
                   };

uint8_t UUID[16] = {0xd9, 0x82, 0x39, 0x86, 0x80, 0x92, 0x4e, 0xe9,
                    0xb1, 0xf6, 0xcf, 0x80, 0x84, 0x86, 0xf1, 0x86
                   };


static char *macStr = NULL;
static char CpemacStr[ 32 ];
BOOL schema_file_parsed = FALSE;
char *ndsschemabuffer = NULL;
char *nds_schemaidbuffer = "d9823986-8092-4ee9-b1f6-cf808486f186/8e2a859d6ef44610423f559296565635";
static size_t AvroSerializedSize;
static size_t OneAvroSerializedSize;
char AvroSerializedBuf[ WRITER_BUF_SIZE ];
extern LmObjectHosts lmHosts;
extern pthread_mutex_t LmHostObjectMutex;

// local data, load it with real data if necessary
char ReportSource[] = "LMLite";

char* GetNDStatusSchemaBuffer()
{
  return ndsschemabuffer;
}

int GetNDStatusSchemaBufferSize()
{
int len = 0;
if(ndsschemabuffer)
	len = strlen(ndsschemabuffer);
  
return len;
}

char* GetNDStatusSchemaIDBuffer()
{
  return nds_schemaidbuffer;
}

int GetNDStatusSchemaIDBufferSize()
{
int len = 0;
if(nds_schemaidbuffer)
        len = strlen(nds_schemaidbuffer);

return len;
}

int NumberofElementsinLinkedList(struct networkdevicestatusdata* head)
{
  int numelements = 0;
  struct networkdevicestatusdata* ptr  = head;
  while (ptr != NULL)
  {
    numelements++;
    ptr = ptr->next;
  }
  return numelements;
}


avro_writer_t prepare_writer_status()
{
  avro_writer_t writer;
  long lSize = 0;

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Avro prepares to serialize data\n"));

  if ( schema_file_parsed == FALSE )
  {
    FILE *fp;

    /* open schema file */
    fp = fopen ( NETWORK_DEVICE_STATUS_AVRO_FILENAME , "rb" );
    if ( !fp ) perror( NETWORK_DEVICE_STATUS_AVRO_FILENAME " doesn't exist."), exit(1);

    /* seek through file and get file size*/
    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );

    /*back to the start of the file*/
    rewind( fp );

    /* allocate memory for entire content */
    ndsschemabuffer = calloc( 1, lSize + 1 );

    if ( !ndsschemabuffer ) fclose(fp), fputs("memory alloc fails", stderr), exit(1);

    /* copy the file into the buffer */
    if ( 1 != fread( ndsschemabuffer , lSize, 1 , fp) )
      fclose(fp), free(ndsschemabuffer), fputs("entire read fails", stderr), exit(1);

    fclose(fp);

    schema_file_parsed = TRUE; // parse schema file once only
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Read Avro schema file ONCE, lSize = %ld, pbuffer = 0x%lx.\n", lSize + 1, (ulong)ndsschemabuffer ));
  }
  else
  {
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Stored lSize = %ld, pbuffer = 0x%lx.\n", lSize + 1, (ulong)ndsschemabuffer ));
  }

  memset(&AvroSerializedBuf[0], 0, sizeof(AvroSerializedBuf));

  AvroSerializedBuf[0] = MAGIC_NUMBER; /* fill MAGIC number = Empty, i.e. no Schema ID */

  memcpy( &AvroSerializedBuf[ MAGIC_NUMBER_SIZE ], UUID, sizeof(UUID));

  memcpy( &AvroSerializedBuf[ MAGIC_NUMBER_SIZE + sizeof(UUID) ], HASH, sizeof(HASH));

  writer = avro_writer_memory( (char*)&AvroSerializedBuf[MAGIC_NUMBER_SIZE + SCHEMA_ID_LENGTH],
                               sizeof(AvroSerializedBuf) - MAGIC_NUMBER_SIZE - SCHEMA_ID_LENGTH );

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

  return writer;
}


/* function call from lmlite with parameters */
void network_devices_status_report(struct networkdevicestatusdata *head)
{
  int i, j, k = 0;
  uint8_t* b64buffer =  NULL;
  size_t decodesize = 0;
  int numElements = 0;
  struct networkdevicestatusdata* ptr = head;
  avro_writer_t writer;
  char * serviceName = "lmlite";
  char * dest = "event:com.comcast.kestrel.reports.NetworkDevicesStatus";
  char * contentType = "avro/binary"; // contentType "application/json", "avro/binary"
  uuid_t transaction_id;
  char trans_id[37];

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

  numElements = NumberofElementsinLinkedList(head);

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, numElements = %d\n", numElements ));

  OneAvroSerializedSize = 0;

  /* goes thru total number of elements in link list */
  writer = prepare_writer_status();
  //schemas
  avro_schema_error_t  error = NULL;

  //Master report/datum
  avro_schema_t network_device_report_schema = NULL;
  avro_schema_from_json(ndsschemabuffer, strlen(ndsschemabuffer),
                        &network_device_report_schema, &error);

  //generate an avro class from our schema and get a pointer to the value interface
  avro_value_iface_t  *iface = avro_generic_class_from_schema(network_device_report_schema);

  //Reset out writer
  avro_writer_reset(writer);

  //Network Device Report
  avro_value_t  adr;
  avro_generic_value_new(iface, &adr);

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, GatewayNetworkDeviceTrafficReport\tType: %d\n", avro_value_get_type(&adr)));

  avro_value_t  adrField;

  //MAC
  /* Get CPE mac address, do it only pointer is NULL */
  if ( macStr == NULL )
  {
    macStr = getDeviceMac();

    strncpy( CpemacStr, macStr, sizeof(CpemacStr));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Received DeviceMac from Atom side: %s\n",macStr));
  }

  char CpeMacHoldingBuf[ 20 ] = {0};
  unsigned char CpeMacid[ 7 ] = {0};
  for (k = 0; k < 6; k++ )
  {
    /* copy 2 bytes */
    CpeMacHoldingBuf[ k * 2 ] = CpemacStr[ k * 2 ];
    CpeMacHoldingBuf[ k * 2 + 1 ] = CpemacStr[ k * 2 + 1 ];
    CpeMacid[ k ] = (unsigned char)strtol(&CpeMacHoldingBuf[ k * 2 ], NULL, 16);
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Mac address = %0x\n", CpeMacid[ k ] ));
  }
  avro_value_get_by_name(&adr, "report_header", &adrField, NULL);
  if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

  avro_value_get_by_name(&adrField, "cpe_id", &adrField, NULL);
  if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

  //Optional value for unions, mac address is an union
  avro_value_t optional;

  avro_value_get_by_name(&adrField, "mac_address", &adrField, NULL);
  avro_value_set_branch(&adrField, 1, &optional);
  avro_value_set_fixed(&optional, CpeMacid, 6);
  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, mac_address\tType: %d\n", avro_value_get_type(&adrField)));
  if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

  // timestamp - long
  avro_value_get_by_name(&adr, "report_header", &adrField, NULL);
  avro_value_get_by_name(&adrField, "timestamp", &adrField, NULL);

  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  avro_value_set_long(&adrField, /*ts.tv_sec */ 1000 );
  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, timestamp\tType: %d\n", avro_value_get_type(&adrField)));
  if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

  //Report source - string
  avro_value_get_by_name(&adr, "report_header", &adrField, NULL);
  avro_value_get_by_name(&adrField, "report_source", &adrField, NULL);
  avro_value_set_branch(&adrField, 1, &optional);
  avro_value_set_string(&optional, ReportSource);
  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, report_source\tType: %d\n", avro_value_get_type(&adrField)));
  if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

  //Data Field block

  avro_value_get_by_name(&adr, "data", &adrField, NULL);
  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, NetworkDeviceStatusReports - data\tType: %d\n", avro_value_get_type(&adrField)));
  if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

  //adrField now contains a reference to the AssociatedDeviceReportsArray
  //Device Report
  avro_value_t dr;

  //Current Device Report Field
  avro_value_t drField;

  //interference sources
  avro_value_t interferenceSource;

 while(ptr)
  {
    {

       CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Current Link List Ptr = [0x%lx], numElements = %d\n", (ulong)ptr, numElements ));
       CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, \tDevice entry #: %d\n", i + 1));

      //Append a DeviceReport item to array
      avro_value_append(&adrField, &dr, NULL);
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, \tDevice Status Report\tType: %d\n", avro_value_get_type(&dr)));

      //report_header block
      //device_mac
      avro_value_get_by_name(&dr, "report_header", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, report_header\tType: %d\n", avro_value_get_type(&drField)));
      avro_value_get_by_name(&drField, "device_mac", &drField, NULL);
      avro_value_set_fixed(&drField, ptr->device_mac, 6);
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, \t\tdevice_mac\tType: %d\n", avro_value_get_type(&drField)));
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

      //Timestamp
      avro_value_get_by_name(&dr, "report_header", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      avro_value_get_by_name(&drField, "timestamp", &drField, NULL);
      int64_t tstamp_av = (int64_t) ptr->timestamp.tv_sec * 1000000 + (int64_t) ptr->timestamp.tv_usec;
      tstamp_av = tstamp_av/1000;
      avro_value_set_long(&drField, tstamp_av);
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, \t\ttimestamp\tType: %d\n", avro_value_get_type(&drField)));
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

      // network_data block - this is an union
      // Interface_name
      avro_value_get_by_name(&dr, "network_data", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, network_data\tType: %d\n", avro_value_get_type(&drField)));
      avro_value_set_branch(&drField, 1, &optional);
      avro_value_get_by_name(&optional, "interface_name", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, interface_name\tType: %d\n", avro_value_get_type(&drField)));
      avro_value_set_branch(&drField, 1, &optional);
      avro_value_set_string(&optional, ptr->interface_name );
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

      // status
      avro_value_get_by_name(&dr, "network_data", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      avro_value_set_branch(&drField, 1, &optional);
      avro_value_get_by_name(&optional, "status", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, status\tType: %d\n", avro_value_get_type(&drField)));
      avro_value_set_branch(&drField, 1, &optional);
      if ( ptr->is_active )
          avro_value_set_enum(&optional, avro_schema_enum_get_by_name(avro_value_get_schema(&optional), "ONLINE"));
      else
          avro_value_set_enum(&optional, avro_schema_enum_get_by_name(avro_value_get_schema(&optional), "OFFLINE"));
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

    }
    ptr = ptr->next; // next link list

    /* check for writer size, if buffer is almost full, skip trailing linklist */
    avro_value_sizeof(&adr, &AvroSerializedSize);
    OneAvroSerializedSize = ( OneAvroSerializedSize == 0 ) ? AvroSerializedSize : OneAvroSerializedSize;

    if ( ( WRITER_BUF_SIZE - AvroSerializedSize ) < OneAvroSerializedSize )
    {
      CcspLMLiteTrace(("RDK_LOG_ERROR, AVRO write buffer is almost full, size = %d func %s, exit!\n", (int)AvroSerializedSize, __FUNCTION__ ));      break;
    }

  }

  //Thats the end of that
  avro_value_write(writer, &adr);

  avro_value_sizeof(&adr, &AvroSerializedSize);
  AvroSerializedSize += MAGIC_NUMBER_SIZE + SCHEMA_ID_LENGTH;
  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Serialized writer size %d\n", (int)AvroSerializedSize));

  //Free up memory
  avro_value_decref(&adr);
  avro_value_iface_decref(iface);
  avro_schema_decref(network_device_report_schema);
  avro_writer_free(writer);
  //free(buffer);

  /* b64 encoding */
  decodesize = b64_get_encoded_buffer_size( AvroSerializedSize );
  b64buffer = malloc(decodesize * sizeof(uint8_t));
  b64_encode( (uint8_t*)AvroSerializedBuf, AvroSerializedSize, b64buffer);

  if ( consoleDebugEnable )
  {
    fprintf( stderr, "\nAVro serialized data\n");
    for (k = 0; k < (int)AvroSerializedSize ; k++)
    {
      char buf[30];
      if ( ( k % 32 ) == 0 )
        fprintf( stderr, "\n");
      sprintf(buf, "%02X", (unsigned char)AvroSerializedBuf[k]);
      fprintf( stderr, "%c%c", buf[0], buf[1] );
    }

    fprintf( stderr, "\n\nB64 data\n");
    for (k = 0; k < (int)decodesize; k++)
    {
      if ( ( k % 32 ) == 0 )
        fprintf( stderr, "\n");
      fprintf( stderr, "%c", b64buffer[k]);
    }
    fprintf( stderr, "\n\n");
  }

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before ND WebPA SEND message call\n"));

  uuid_generate_random(transaction_id); 
  uuid_unparse(transaction_id, trans_id);

    // Send data from LMLite to webpa using CCSP bus interface
  sendWebpaMsg(serviceName, dest, trans_id, contentType, AvroSerializedBuf, AvroSerializedSize);

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, After ND WebPA SEND message call\n"));

  free(b64buffer);

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

}

