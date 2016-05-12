/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
   
#include <stdio.h>
#include <assert.h>
#include <avro.h>
#include <arpa/inet.h>
#include <semaphore.h>  /* Semaphore */
#include "ansc_platform.h"

#include "base64.h"
#include "webpa_interface.h"
#include "network_devices_traffic_avropack.h"
#include "ccsp_lmliteLog_wrapper.h"

#define MAGIC_NUMBER      0x85
#define MAGIC_NUMBER_SIZE 1
#define SCHEMA_ID_LENGTH  32
#define WRITER_BUF_SIZE   1024 * 30 // 30K

//      "schemaTypeUUID" : "8323ce6e-25e0-4d23-bdb3-51a541128261",
//      "schemaMD5Hash" : "2ecd240b79ad7c13de765fd891e692b7",

uint8_t HASH_NDT[16] = {0x2e, 0xcd, 0x24, 0x0b, 0x79, 0xad, 0x7c, 0x13,
                    0xde, 0x76, 0x5f, 0xd8, 0x91, 0xe6, 0x92, 0xb7
                   };

uint8_t UUID_NDT[16] = {0x83, 0x23, 0xce, 0x6e, 0x25, 0xe0, 0x4d, 0x23,
                    0xbd, 0xb3, 0x51, 0xa5, 0x41, 0x12, 0x82, 0x61
                   };


static char *macStr = NULL;
static char CpemacStr[ 32 ];
BOOL ndt_schema_file_parsed = FALSE;
char *ndtschemabuffer = NULL;
char *ndtschemaidbuffer = "8323ce6e-25e0-4d23-bdb3-51a541128261/2ecd240b79ad7c13de765fd891e692b7";
static size_t AvroSerializedSize;
static size_t OneAvroSerializedSize;
char AvroSerializedBuf[ WRITER_BUF_SIZE ];

// local data, load it with real data if necessary
char ReportSourceNDT[] = "LMLite";

char* GetNDTrafficSchemaBuffer()
{
  return ndtschemabuffer;
}

int GetNDTrafficSchemaBufferSize()
{
int len = 0;
if(ndtschemabuffer)
	len = strlen(ndtschemabuffer);
  
return len;
}

char* GetNDTrafficSchemaIDBuffer()
{
  return ndtschemaidbuffer;
}

int GetNDTrafficSchemaIDBufferSize()
{
int len = 0;
if(ndtschemaidbuffer)
        len = strlen(ndtschemaidbuffer);

return len;
}

int NumberofElementsinNDTLinkedList(struct networkdevicetrafficdata* head)
{
  int numelements = 0;
  struct networkdevicetrafficdata* ptr  = head;
  while (ptr != NULL)
  {
    numelements++;
    ptr = ptr->next;
  }
  return numelements;
}


avro_writer_t prepare_writer()
{
  avro_writer_t writer;
  long lSize = 0;

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Avro prepares to serialize data\n"));

  if ( ndt_schema_file_parsed == FALSE )
  {
    FILE *fp;

    /* open schema file */
    fp = fopen ( NETWORK_DEVICE_TRAFFIC_AVRO_FILENAME , "rb" );
    if ( !fp ) perror( NETWORK_DEVICE_TRAFFIC_AVRO_FILENAME " doesn't exist."), exit(1);

    /* seek through file and get file size*/
    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );

    /*back to the start of the file*/
    rewind( fp );

    /* allocate memory for entire content */
    ndtschemabuffer = calloc( 1, lSize + 1 );

    if ( !ndtschemabuffer ) fclose(fp), fputs("memory alloc fails", stderr), exit(1);

    /* copy the file into the buffer */
    if ( 1 != fread( ndtschemabuffer , lSize, 1 , fp) )
      fclose(fp), free(ndtschemabuffer), fputs("entire read fails", stderr), exit(1);

    fclose(fp);

    ndt_schema_file_parsed = TRUE; // parse schema file once only
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Read Avro schema file ONCE, lSize = %ld, pbuffer = 0x%lx.\n", lSize + 1, (ulong)ndtschemabuffer ));
  }
  else
  {
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Stored lSize = %ld, pbuffer = 0x%lx.\n", lSize + 1, (ulong)ndtschemabuffer ));
  }

  memset(&AvroSerializedBuf[0], 0, sizeof(AvroSerializedBuf));

  AvroSerializedBuf[0] = MAGIC_NUMBER; /* fill MAGIC number = Empty, i.e. no Schema ID */

  memcpy( &AvroSerializedBuf[ MAGIC_NUMBER_SIZE ], UUID_NDT, sizeof(UUID_NDT));

  memcpy( &AvroSerializedBuf[ MAGIC_NUMBER_SIZE + sizeof(UUID_NDT) ], HASH_NDT, sizeof(HASH_NDT));

  writer = avro_writer_memory( (char*)&AvroSerializedBuf[MAGIC_NUMBER_SIZE + SCHEMA_ID_LENGTH],
                               sizeof(AvroSerializedBuf) - MAGIC_NUMBER_SIZE - SCHEMA_ID_LENGTH );

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

  return writer;
}


/* function call from lmlite with parameters */
void network_devices_traffic_report(struct networkdevicetrafficdata *head)
{
  int i, j, k = 0;
  uint8_t* b64buffer =  NULL;
  size_t decodesize = 0;
  int numElements = 0;
  struct networkdevicetrafficdata* ptr = head;
  avro_writer_t writer;
  char * serviceName = "lmlite";
  char * dest = "event:com.comcast.kestrel.reports.NetworkDevicesTraffic";
  char * trans_id = "abcd"; // identifier for each message, if required. If NULL msgpack will fail
  char * contentType = "avro/binary"; // contentType "application/json", "avro/binary"

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : ENTER \n", __FUNCTION__ ));

  numElements = NumberofElementsinNDTLinkedList(head);

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, numElements = %d\n", numElements ));

  OneAvroSerializedSize = 0;

  /* goes thru total number of elements in link list */
  writer = prepare_writer();
  //schemas
  avro_schema_error_t  error = NULL;

  //Master report/datum
  avro_schema_t network_device_report_schema = NULL;
  avro_schema_from_json(ndtschemabuffer, strlen(ndtschemabuffer),
                        &network_device_report_schema, &error);

  //generate an avro class from our schema and get a pointer to the value interface
  avro_value_iface_t  *iface = avro_generic_class_from_schema(network_device_report_schema);

  //Reset out writer
  avro_writer_reset(writer);

  //Network Device Report
  avro_value_t  adr;
  avro_generic_value_new(iface, &adr);

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, GatewayNetworkDeviceStatusReport\tType: %d\n", avro_value_get_type(&adr)));

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
  avro_value_set_string(&optional, ReportSourceNDT);
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
      avro_value_get_by_name(&optional, "ip_address", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, ip_address\tType: %d\n", avro_value_get_type(&drField)));
      avro_value_set_branch(&drField, 1, &optional);
      avro_value_set_string(&optional, ptr->ip_address );
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

      // external_bytes_up
      avro_value_get_by_name(&dr, "network_data", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      avro_value_set_branch(&drField, 1, &optional);
      avro_value_get_by_name(&optional, "external_bytes_up", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, status\tType: %d\n", avro_value_get_type(&drField)));
      avro_value_set_branch(&adrField, 1, &optional);
      avro_value_set_long(&optional, ptr->external_bytes_up);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));

      // external_bytes_down
      avro_value_get_by_name(&dr, "network_data", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      avro_value_set_branch(&drField, 1, &optional);
      avro_value_get_by_name(&optional, "external_bytes_down", &drField, NULL);
      if ( CHK_AVRO_ERR ) CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, %s\n", avro_strerror()));
      CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, status\tType: %d\n", avro_value_get_type(&drField)));
      avro_value_set_branch(&adrField, 1, &optional);
      avro_value_set_long(&optional, ptr->external_bytes_down);
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

  // Send data from LMLite to webpa using CCSP bus interface
  sendWebpaMsg(serviceName, dest, trans_id, contentType, AvroSerializedBuf, AvroSerializedSize);

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, After ND WebPA SEND message call\n"));

  free(b64buffer);

  CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s : EXIT \n", __FUNCTION__ ));

}

